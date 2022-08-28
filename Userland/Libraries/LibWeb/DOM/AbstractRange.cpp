/*
 * Copyright (c) 2022, Andreas Kling <kling@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <LibWeb/Bindings/AbstractRangePrototype.h>
#include <LibWeb/DOM/AbstractRange.h>
#include <LibWeb/DOM/Document.h>
#include <LibWeb/HTML/Window.h>

namespace Web::DOM {

AbstractRange::AbstractRange(Node& start_container, u32 start_offset, Node& end_container, u32 end_offset)
    : Bindings::PlatformObject(start_container.document().window().ensure_web_prototype<Bindings::AbstractRangePrototype>("AbstractRange"))
    , m_start_container(start_container)
    , m_start_offset(start_offset)
    , m_end_container(end_container)
    , m_end_offset(end_offset)
{
}

AbstractRange::~AbstractRange() = default;

void AbstractRange::visit_edges(Cell::Visitor& visitor)
{
    Base::visit_edges(visitor);
    visitor.visit(m_start_container.ptr());
    visitor.visit(m_end_container.ptr());
}

}
