/*
 * Copyright (c) 2021, Andreas Kling <kling@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/RefCountForwarder.h>
#include <AK/StdLibExtras.h>
#include <LibWeb/Bindings/Wrappable.h>
#include <LibWeb/HTML/Window.h>

namespace Web::NavigationTiming {

class PerformanceTiming final
    : public RefCounted<PerformanceTiming>
    , public Bindings::Wrappable {
public:
    using WrapperType = Bindings::PerformanceTimingWrapper;
    using AllowOwnPtr = TrueType;

    explicit PerformanceTiming(HTML::Window&);
    ~PerformanceTiming();

    u64 navigation_start() { return 0; }
    u64 unload_event_start() { return 0; }
    u64 unload_event_end() { return 0; }
    u64 redirect_start() { return 0; }
    u64 redirect_end() { return 0; }
    u64 fetch_start() { return 0; }
    u64 domain_lookup_start() { return 0; }
    u64 domain_lookup_end() { return 0; }
    u64 connect_start() { return 0; }
    u64 connect_end() { return 0; }
    u64 secure_connection_start() { return 0; }
    u64 request_start() { return 0; }
    u64 response_start() { return 0; }
    u64 response_end() { return 0; }
    u64 dom_loading() { return 0; }
    u64 dom_interactive() { return 0; }
    u64 dom_content_loaded_event_start() { return 0; }
    u64 dom_content_loaded_event_end() { return 0; }
    u64 dom_complete() { return 0; }
    u64 load_event_start() { return 0; }
    u64 load_event_end() { return 0; }

private:
    JS::Handle<HTML::Window> m_window;
};

}
