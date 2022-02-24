/*
 * Copyright (c) 2020-2021, Andreas Kling <kling@serenityos.org>
 * Copyright (c) 2021, Sam Atkins <atkinssj@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/Debug.h>
#include <AK/JsonObject.h>
#include <LibGfx/Bitmap.h>
#include <LibGfx/FontDatabase.h>
#include <LibGfx/SystemTheme.h>
#include <LibJS/Console.h>
#include <LibJS/Heap/Heap.h>
#include <LibJS/Interpreter.h>
#include <LibJS/Parser.h>
#include <LibWeb/Bindings/MainThreadVM.h>
#include <LibWeb/Cookie/ParsedCookie.h>
#include <LibWeb/DOM/Document.h>
#include <LibWeb/DOM/Window.h>
#include <LibWeb/Dump.h>
#include <LibWeb/HTML/BrowsingContext.h>
#include <LibWeb/HTML/Scripting/ClassicScript.h>
#include <LibWeb/HTML/Storage.h>
#include <LibWeb/Layout/InitialContainingBlock.h>
#include <LibWeb/Loader/ContentFilter.h>
#include <LibWeb/Loader/ResourceLoader.h>
#include <WebContent/ConnectionFromClient.h>
#include <WebContent/PageHost.h>
#include <WebContent/WebContentClientEndpoint.h>
#include <pthread.h>

namespace WebContent {

ConnectionFromClient::ConnectionFromClient(NonnullOwnPtr<Core::Stream::LocalSocket> socket)
    : IPC::ConnectionFromClient<WebContentClientEndpoint, WebContentServerEndpoint>(*this, move(socket), 1)
    , m_page_host(PageHost::create(*this))
{
    m_paint_flush_timer = Core::Timer::create_single_shot(0, [this] { flush_pending_paint_requests(); });
}

ConnectionFromClient::~ConnectionFromClient()
{
}

void ConnectionFromClient::die()
{
    Core::EventLoop::current().quit(0);
}

Web::Page& ConnectionFromClient::page()
{
    return m_page_host->page();
}

const Web::Page& ConnectionFromClient::page() const
{
    return m_page_host->page();
}

void ConnectionFromClient::update_system_theme(const Core::AnonymousBuffer& theme_buffer)
{
    Gfx::set_system_theme(theme_buffer);
    auto impl = Gfx::PaletteImpl::create_with_anonymous_buffer(theme_buffer);
    m_page_host->set_palette_impl(*impl);
}

void ConnectionFromClient::update_system_fonts(String const& default_font_query, String const& fixed_width_font_query)
{
    Gfx::FontDatabase::set_default_font_query(default_font_query);
    Gfx::FontDatabase::set_fixed_width_font_query(fixed_width_font_query);
}

void ConnectionFromClient::update_screen_rects(const Vector<Gfx::IntRect>& rects, u32 main_screen)
{
    m_page_host->set_screen_rects(rects, main_screen);
}

void ConnectionFromClient::load_url(const URL& url)
{
    dbgln_if(SPAM_DEBUG, "handle: WebContentServer::LoadURL: url={}", url);

    String process_name;
    if (url.host().is_empty())
        process_name = "WebContent";
    else
        process_name = String::formatted("WebContent: {}", url.host());

    pthread_setname_np(pthread_self(), process_name.characters());

    page().load(url);
}

void ConnectionFromClient::load_html(const String& html, const URL& url)
{
    dbgln_if(SPAM_DEBUG, "handle: WebContentServer::LoadHTML: html={}, url={}", html, url);
    page().load_html(html, url);
}

void ConnectionFromClient::set_viewport_rect(const Gfx::IntRect& rect)
{
    dbgln_if(SPAM_DEBUG, "handle: WebContentServer::SetViewportRect: rect={}", rect);
    m_page_host->set_viewport_rect(rect);
}

void ConnectionFromClient::add_backing_store(i32 backing_store_id, const Gfx::ShareableBitmap& bitmap)
{
    m_backing_stores.set(backing_store_id, *bitmap.bitmap());
}

void ConnectionFromClient::remove_backing_store(i32 backing_store_id)
{
    m_backing_stores.remove(backing_store_id);
}

void ConnectionFromClient::paint(const Gfx::IntRect& content_rect, i32 backing_store_id)
{
    for (auto& pending_paint : m_pending_paint_requests) {
        if (pending_paint.bitmap_id == backing_store_id) {
            pending_paint.content_rect = content_rect;
            return;
        }
    }

    auto it = m_backing_stores.find(backing_store_id);
    if (it == m_backing_stores.end()) {
        did_misbehave("Client requested paint with backing store ID");
        return;
    }

    auto& bitmap = *it->value;
    m_pending_paint_requests.append({ content_rect, bitmap, backing_store_id });
    m_paint_flush_timer->start();
}

void ConnectionFromClient::flush_pending_paint_requests()
{
    for (auto& pending_paint : m_pending_paint_requests) {
        m_page_host->paint(pending_paint.content_rect, *pending_paint.bitmap);
        async_did_paint(pending_paint.content_rect, pending_paint.bitmap_id);
    }
    m_pending_paint_requests.clear();
}

void ConnectionFromClient::mouse_down(const Gfx::IntPoint& position, unsigned int button, [[maybe_unused]] unsigned int buttons, unsigned int modifiers)
{
    page().handle_mousedown(position, button, modifiers);
}

void ConnectionFromClient::mouse_move(const Gfx::IntPoint& position, [[maybe_unused]] unsigned int button, unsigned int buttons, unsigned int modifiers)
{
    page().handle_mousemove(position, buttons, modifiers);
}

void ConnectionFromClient::mouse_up(const Gfx::IntPoint& position, unsigned int button, [[maybe_unused]] unsigned int buttons, unsigned int modifiers)
{
    page().handle_mouseup(position, button, modifiers);
}

void ConnectionFromClient::mouse_wheel(const Gfx::IntPoint& position, unsigned int button, [[maybe_unused]] unsigned int buttons, unsigned int modifiers, i32 wheel_delta_x, i32 wheel_delta_y)
{
    page().handle_mousewheel(position, button, modifiers, wheel_delta_x, wheel_delta_y);
}

void ConnectionFromClient::key_down(i32 key, unsigned int modifiers, u32 code_point)
{
    page().handle_keydown((KeyCode)key, modifiers, code_point);
}

void ConnectionFromClient::key_up(i32 key, unsigned int modifiers, u32 code_point)
{
    page().handle_keyup((KeyCode)key, modifiers, code_point);
}

void ConnectionFromClient::debug_request(const String& request, const String& argument)
{
    if (request == "dump-dom-tree") {
        if (auto* doc = page().top_level_browsing_context().active_document())
            Web::dump_tree(*doc);
    }

    if (request == "dump-layout-tree") {
        if (auto* doc = page().top_level_browsing_context().active_document()) {
            if (auto* icb = doc->layout_node())
                Web::dump_tree(*icb);
        }
    }

    if (request == "dump-stacking-context-tree") {
        if (auto* doc = page().top_level_browsing_context().active_document()) {
            if (auto* icb = doc->layout_node()) {
                if (auto* stacking_context = icb->stacking_context())
                    stacking_context->dump();
            }
        }
    }

    if (request == "dump-style-sheets") {
        if (auto* doc = page().top_level_browsing_context().active_document()) {
            for (auto& sheet : doc->style_sheets().sheets()) {
                Web::dump_sheet(sheet);
            }
        }
    }

    if (request == "collect-garbage") {
        Web::Bindings::main_thread_vm().heap().collect_garbage(JS::Heap::CollectionType::CollectGarbage, true);
    }

    if (request == "set-line-box-borders") {
        bool state = argument == "on";
        m_page_host->set_should_show_line_box_borders(state);
        page().top_level_browsing_context().set_needs_display(page().top_level_browsing_context().viewport_rect());
    }

    if (request == "clear-cache") {
        Web::ResourceLoader::the().clear_cache();
    }

    if (request == "spoof-user-agent") {
        Web::ResourceLoader::the().set_user_agent(argument);
    }

    if (request == "same-origin-policy") {
        m_page_host->page().set_same_origin_policy_enabled(argument == "on");
    }

    if (request == "dump-local-storage") {
        if (auto* doc = page().top_level_browsing_context().active_document())
            doc->window().local_storage()->dump();
    }
}

void ConnectionFromClient::get_source()
{
    if (auto* doc = page().top_level_browsing_context().active_document()) {
        async_did_get_source(doc->url(), doc->source());
    }
}

void ConnectionFromClient::inspect_dom_tree()
{
    if (auto* doc = page().top_level_browsing_context().active_document()) {
        async_did_get_dom_tree(doc->dump_dom_tree_as_json());
    }
}

Messages::WebContentServer::InspectDomNodeResponse ConnectionFromClient::inspect_dom_node(i32 node_id)
{
    auto& top_context = page().top_level_browsing_context();

    top_context.for_each_in_inclusive_subtree([&](auto& ctx) {
        if (ctx.active_document() != nullptr) {
            ctx.active_document()->set_inspected_node(nullptr);
        }
        return IterationDecision::Continue;
    });

    Web::DOM::Node* node = Web::DOM::Node::from_id(node_id);
    if (!node) {
        return { false, "", "", "" };
    }

    node->document().set_inspected_node(node);

    if (node->is_element()) {
        auto& element = verify_cast<Web::DOM::Element>(*node);
        if (!element.specified_css_values())
            return { false, "", "", "" };

        auto serialize_json = [](Web::CSS::StyleProperties const& properties) -> String {
            StringBuilder builder;

            auto serializer = MUST(JsonObjectSerializer<>::try_create(builder));
            properties.for_each_property([&](auto property_id, auto& value) {
                MUST(serializer.add(Web::CSS::string_from_property_id(property_id), value.to_string()));
            });
            MUST(serializer.finish());

            return builder.to_string();
        };

        auto serialize_custom_properties_json = [](Web::DOM::Element const& element) -> String {
            StringBuilder builder;
            auto serializer = MUST(JsonObjectSerializer<>::try_create(builder));
            HashTable<String> seen_properties;

            auto const* element_to_check = &element;
            while (element_to_check) {
                for (auto const& property : element_to_check->custom_properties()) {
                    if (!seen_properties.contains(property.key)) {
                        seen_properties.set(property.key);
                        MUST(serializer.add(property.key, property.value.value->to_string()));
                    }
                }

                element_to_check = element_to_check->parent_element();
            }

            MUST(serializer.finish());

            return builder.to_string();
        };

        String specified_values_json = serialize_json(*element.specified_css_values());
        String computed_values_json = serialize_json(element.computed_style());
        String custom_properties_json = serialize_custom_properties_json(element);
        return { true, specified_values_json, computed_values_json, custom_properties_json };
    }

    return { false, "", "", "" };
}

Messages::WebContentServer::GetHoveredNodeIdResponse ConnectionFromClient::get_hovered_node_id()
{
    if (auto* document = page().top_level_browsing_context().active_document()) {
        auto hovered_node = document->hovered_node();
        if (hovered_node)
            return hovered_node->id();
    }
    return (i32)0;
}

void ConnectionFromClient::initialize_js_console(Badge<PageHost>)
{
    auto* document = page().top_level_browsing_context().active_document();
    auto interpreter = document->interpreter().make_weak_ptr();
    if (m_interpreter.ptr() == interpreter.ptr())
        return;

    m_interpreter = interpreter;
    m_console_client = make<WebContentConsoleClient>(interpreter->global_object().console(), interpreter, *this);
    interpreter->global_object().console().set_client(*m_console_client.ptr());
}

void ConnectionFromClient::js_console_input(const String& js_source)
{
    if (m_console_client)
        m_console_client->handle_input(js_source);
}

void ConnectionFromClient::run_javascript(String const& js_source)
{
    auto* active_document = page().top_level_browsing_context().active_document();

    if (!active_document)
        return;

    // This is partially based on "execute a javascript: URL request" https://html.spec.whatwg.org/multipage/browsing-the-web.html#javascript-protocol

    // Let settings be browsingContext's active document's relevant settings object.
    auto& settings = active_document->relevant_settings_object();

    // Let baseURL be settings's API base URL.
    auto base_url = settings.api_base_url();

    // Let script be the result of creating a classic script given scriptSource, settings, baseURL, and the default classic script fetch options.
    // FIXME: This doesn't pass in "default classic script fetch options"
    // FIXME: What should the filename be here?
    auto script = Web::HTML::ClassicScript::create("(client connection run_javascript)", js_source, settings, move(base_url));

    // Let evaluationStatus be the result of running the classic script script.
    auto evaluation_status = script->run();

    if (evaluation_status.is_error())
        dbgln("Exception :(");
}

void ConnectionFromClient::js_console_request_messages(i32 start_index)
{
    if (m_console_client)
        m_console_client->send_messages(start_index);
}

Messages::WebContentServer::GetSelectedTextResponse ConnectionFromClient::get_selected_text()
{
    return page().focused_context().selected_text();
}

void ConnectionFromClient::select_all()
{
    page().focused_context().select_all();
    page().client().page_did_change_selection();
}

Messages::WebContentServer::DumpLayoutTreeResponse ConnectionFromClient::dump_layout_tree()
{
    auto* document = page().top_level_browsing_context().active_document();
    if (!document)
        return String { "(no DOM tree)" };
    auto* layout_root = document->layout_node();
    if (!layout_root)
        return String { "(no layout tree)" };
    StringBuilder builder;
    Web::dump_tree(builder, *layout_root);
    return builder.to_string();
}

void ConnectionFromClient::set_content_filters(Vector<String> const& filters)
{
    for (auto& filter : filters)
        Web::ContentFilter::the().add_pattern(filter);
}

void ConnectionFromClient::set_preferred_color_scheme(Web::CSS::PreferredColorScheme const& color_scheme)
{
    m_page_host->set_preferred_color_scheme(color_scheme);
}

void ConnectionFromClient::set_has_focus(bool has_focus)
{
    m_page_host->set_has_focus(has_focus);
}

}
