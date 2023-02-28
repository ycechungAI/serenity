/*
 * Copyright (c) 2023, Luke Wilde <lukew@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <LibWeb/Bindings/Intrinsics.h>
#include <LibWeb/HTML/MimeType.h>
#include <LibWeb/HTML/Scripting/Environments.h>
#include <LibWeb/HTML/Window.h>

namespace Web::HTML {

MimeType::MimeType(JS::Realm& realm, String const& type)
    : Bindings::PlatformObject(realm)
    , m_type(type)
{
}

MimeType::~MimeType() = default;

JS::ThrowCompletionOr<void> MimeType::initialize(JS::Realm& realm)
{
    MUST_OR_THROW_OOM(Base::initialize(realm));
    set_prototype(&Bindings::ensure_web_prototype<Bindings::MimeTypePrototype>(realm, "MimeType"));

    return {};
}

// https://html.spec.whatwg.org/multipage/system-state.html#concept-mimetype-type
String const& MimeType::type() const
{
    // The MimeType interface's type getter steps are to return this's type.
    return m_type;
}

// https://html.spec.whatwg.org/multipage/system-state.html#dom-mimetype-description
JS::ThrowCompletionOr<String> MimeType::description() const
{
    // The MimeType interface's description getter steps are to return "Portable Document Format".
    static String description_string = TRY_OR_THROW_OOM(vm(), String::from_utf8("Portable Document Format"sv));
    return description_string;
}

// https://html.spec.whatwg.org/multipage/system-state.html#dom-mimetype-suffixes
String const& MimeType::suffixes() const
{
    // The MimeType interface's suffixes getter steps are to return "pdf".
    static String suffixes_string = String::from_utf8_short_string("pdf"sv);
    return suffixes_string;
}

// https://html.spec.whatwg.org/multipage/system-state.html#dom-mimetype-enabledplugin
JS::NonnullGCPtr<Plugin> MimeType::enabled_plugin() const
{
    // The MimeType interface's enabledPlugin getter steps are to return this's relevant global object's PDF viewer plugin objects[0] (i.e., the generic "PDF Viewer" one).
    auto& window = verify_cast<HTML::Window>(HTML::relevant_global_object(*this));
    auto plugin_objects = window.pdf_viewer_plugin_objects();

    // NOTE: If a MimeType object was created, that means PDF viewer support is enabled, meaning there will be Plugin objects.
    VERIFY(!plugin_objects.is_empty());
    return plugin_objects.first();
}

}
