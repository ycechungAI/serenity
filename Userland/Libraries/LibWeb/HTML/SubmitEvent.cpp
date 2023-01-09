/*
 * Copyright (c) 2022, Andreas Kling <kling@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <LibWeb/Bindings/Intrinsics.h>
#include <LibWeb/HTML/SubmitEvent.h>

namespace Web::HTML {

SubmitEvent* SubmitEvent::create(JS::Realm& realm, DeprecatedFlyString const& event_name, SubmitEventInit const& event_init)
{
    return realm.heap().allocate<SubmitEvent>(realm, realm, event_name, event_init);
}

SubmitEvent* SubmitEvent::construct_impl(JS::Realm& realm, DeprecatedFlyString const& event_name, SubmitEventInit const& event_init)
{
    return create(realm, event_name, event_init);
}

SubmitEvent::SubmitEvent(JS::Realm& realm, DeprecatedFlyString const& event_name, SubmitEventInit const& event_init)
    : DOM::Event(realm, event_name, event_init)
    , m_submitter(event_init.submitter)
{
    set_prototype(&Bindings::cached_web_prototype(realm, "SubmitEvent"));
}

SubmitEvent::~SubmitEvent() = default;

void SubmitEvent::visit_edges(Cell::Visitor& visitor)
{
    Base::visit_edges(visitor);
    visitor.visit(m_submitter.ptr());
}

}
