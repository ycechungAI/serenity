/*
 * Copyright (c) 2021-2022, Andreas Kling <kling@serenityos.org>
 * Copyright (c) 2021, Luke Wilde <lukew@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <LibWeb/Bindings/Intrinsics.h>
#include <LibWeb/DOM/Document.h>
#include <LibWeb/DOM/Element.h>
#include <LibWeb/DOM/HTMLCollection.h>
#include <LibWeb/DOM/ParentNode.h>
#include <LibWeb/Namespace.h>

namespace Web::DOM {

JS_DEFINE_ALLOCATOR(HTMLCollection);

JS::NonnullGCPtr<HTMLCollection> HTMLCollection::create(ParentNode& root, Scope scope, Function<bool(Element const&)> filter)
{
    return root.heap().allocate<HTMLCollection>(root.realm(), root, scope, move(filter));
}

HTMLCollection::HTMLCollection(ParentNode& root, Scope scope, Function<bool(Element const&)> filter)
    : PlatformObject(root.realm())
    , m_root(root)
    , m_filter(move(filter))
    , m_scope(scope)
{
    m_legacy_platform_object_flags = LegacyPlatformObjectFlags {
        .supports_indexed_properties = true,
        .supports_named_properties = true,
        .has_legacy_unenumerable_named_properties_interface_extended_attribute = true,
    };
}

HTMLCollection::~HTMLCollection() = default;

void HTMLCollection::initialize(JS::Realm& realm)
{
    Base::initialize(realm);
    WEB_SET_PROTOTYPE_FOR_INTERFACE(HTMLCollection);
}

void HTMLCollection::visit_edges(Cell::Visitor& visitor)
{
    Base::visit_edges(visitor);
    visitor.visit(m_root);
    for (auto& element : m_cached_elements)
        visitor.visit(element);
}

JS::MarkedVector<Element*> HTMLCollection::collect_matching_elements() const
{
    if (m_cached_dom_tree_version != root()->document().dom_tree_version()) {
        m_cached_elements.clear();
        if (m_scope == Scope::Descendants) {
            m_root->for_each_in_subtree_of_type<Element>([&](auto& element) {
                if (m_filter(element))
                    m_cached_elements.append(element);
                return IterationDecision::Continue;
            });
        } else {
            m_root->for_each_child_of_type<Element>([&](auto& element) {
                if (m_filter(element))
                    m_cached_elements.append(element);
                return IterationDecision::Continue;
            });
        }
        m_cached_dom_tree_version = root()->document().dom_tree_version();
    }

    JS::MarkedVector<Element*> elements(heap());
    for (auto& element : m_cached_elements)
        elements.append(element);
    return elements;
}

// https://dom.spec.whatwg.org/#dom-htmlcollection-length
size_t HTMLCollection::length() const
{
    // The length getter steps are to return the number of nodes represented by the collection.
    return collect_matching_elements().size();
}

// https://dom.spec.whatwg.org/#dom-htmlcollection-item
Element* HTMLCollection::item(size_t index) const
{
    // The item(index) method steps are to return the indexth element in the collection. If there is no indexth element in the collection, then the method must return null.
    auto elements = collect_matching_elements();
    if (index >= elements.size())
        return nullptr;
    return elements[index];
}

// https://dom.spec.whatwg.org/#dom-htmlcollection-nameditem-key
Element* HTMLCollection::named_item(FlyString const& name) const
{
    // 1. If key is the empty string, return null.
    if (name.is_empty())
        return nullptr;
    auto elements = collect_matching_elements();
    // 2. Return the first element in the collection for which at least one of the following is true:
    //      - it has an ID which is key;
    if (auto it = elements.find_if([&](auto& entry) { return entry->id().has_value() && entry->id().value() == name; }); it != elements.end())
        return *it;
    //      - it is in the HTML namespace and has a name attribute whose value is key;
    if (auto it = elements.find_if([&](auto& entry) { return entry->namespace_uri() == Namespace::HTML && entry->name() == name; }); it != elements.end())
        return *it;
    //    or null if there is no such element.
    return nullptr;
}

// https://dom.spec.whatwg.org/#ref-for-dfn-supported-property-names
Vector<FlyString> HTMLCollection::supported_property_names() const
{
    // 1. Let result be an empty list.
    Vector<FlyString> result;

    // 2. For each element represented by the collection, in tree order:
    auto elements = collect_matching_elements();

    for (auto& element : elements) {
        // 1. If element has an ID which is not in result, append element’s ID to result.
        if (auto const& id = element->id(); id.has_value()) {
            if (!result.contains_slow(id.value()))
                result.append(id.value());
        }

        // 2. If element is in the HTML namespace and has a name attribute whose value is neither the empty string nor is in result, append element’s name attribute value to result.
        if (element->namespace_uri() == Namespace::HTML && element->name().has_value()) {
            auto name = element->name().value();
            if (!name.is_empty() && !result.contains_slow(name))
                result.append(move(name));
        }
    }

    // 3. Return result.
    return result;
}

// https://dom.spec.whatwg.org/#ref-for-dfn-supported-property-indices%E2%91%A1
bool HTMLCollection::is_supported_property_index(u32 index) const
{
    // The object’s supported property indices are the numbers in the range zero to one less than the number of elements represented by the collection.
    // If there are no such elements, then there are no supported property indices.
    auto elements = collect_matching_elements();
    if (elements.is_empty())
        return false;

    return index < elements.size();
}

WebIDL::ExceptionOr<JS::Value> HTMLCollection::item_value(size_t index) const
{
    auto* element = item(index);
    if (!element)
        return JS::js_undefined();
    return const_cast<Element*>(element);
}

WebIDL::ExceptionOr<JS::Value> HTMLCollection::named_item_value(FlyString const& index) const
{
    auto* element = named_item(index);
    if (!element)
        return JS::js_undefined();
    return const_cast<Element*>(element);
}
}
