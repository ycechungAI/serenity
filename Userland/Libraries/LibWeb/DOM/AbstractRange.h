/*
 * Copyright (c) 2022, Luke Wilde <lukew@serenityos.org>
 * Copyright (c) 2022, Andreas Kling <kling@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <LibWeb/Bindings/PlatformObject.h>
#include <LibWeb/Forward.h>

namespace Web::DOM {

class AbstractRange : public Bindings::PlatformObject {
    JS_OBJECT(AbstractRange, Bindings::PlatformObject);

public:
    virtual ~AbstractRange() override;

    AbstractRange& impl() { return *this; }

    Node* start_container() { return m_start_container; }
    Node const* start_container() const { return m_start_container; }
    unsigned start_offset() const { return m_start_offset; }

    Node* end_container() { return m_end_container; }
    Node const* end_container() const { return m_end_container; }
    unsigned end_offset() const { return m_end_offset; }

    // https://dom.spec.whatwg.org/#range-collapsed
    bool collapsed() const
    {
        // A range is collapsed if its start node is its end node and its start offset is its end offset.
        return start_container() == end_container() && start_offset() == end_offset();
    }

protected:
    AbstractRange(Node& start_container, u32 start_offset, Node& end_container, u32 end_offset);

    NonnullRefPtr<Node> m_start_container;
    u32 m_start_offset;

    NonnullRefPtr<Node> m_end_container;
    u32 m_end_offset;
};

}

namespace Web::Bindings {
inline JS::Object* wrap(JS::Realm&, Web::DOM::AbstractRange& object) { return &object; }
using AbstractRangeWrapper = Web::DOM::AbstractRange;
}
