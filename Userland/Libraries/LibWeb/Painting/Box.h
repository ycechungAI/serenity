/*
 * Copyright (c) 2022, Andreas Kling <kling@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/NonnullOwnPtr.h>
#include <LibWeb/Layout/Box.h>
#include <LibWeb/Layout/LineBox.h>

namespace Web::Painting {

class Box {
public:
    static NonnullOwnPtr<Box> create(Layout::Box const& layout_box)
    {
        return adopt_own(*new Box(layout_box));
    }

    explicit Box(Layout::Box const& layout_box)
        : m_layout_box(layout_box)
    {
    }

    Layout::Box const& m_layout_box;

    auto& box_model() { return m_layout_box.box_model(); }
    auto const& box_model() const { return m_layout_box.box_model(); }

    struct OverflowData {
        Gfx::FloatRect scrollable_overflow_rect;
        Gfx::FloatPoint scroll_offset;
    };
    Optional<OverflowData> m_overflow_data;

    Gfx::FloatPoint m_offset;
    Gfx::FloatSize m_content_size;

    Vector<Layout::LineBox> const& line_boxes() const { return m_line_boxes; }
    void set_line_boxes(Vector<Layout::LineBox>&& line_boxes) { m_line_boxes = move(line_boxes); }

    Vector<Layout::LineBox> m_line_boxes;

    // Some boxes hang off of line box fragments. (inline-block, inline-table, replaced, etc)
    Optional<Layout::LineBoxFragmentCoordinate> m_containing_line_box_fragment;

    Gfx::FloatRect absolute_rect() const;
    Gfx::FloatPoint effective_offset() const;

    void set_offset(Gfx::FloatPoint const&);
    void set_offset(float x, float y) { set_offset({ x, y }); }

    Gfx::FloatSize const& content_size() const { return m_content_size; }
    void set_content_size(Gfx::FloatSize const&);
    void set_content_size(float width, float height) { set_content_size({ width, height }); }

    void set_content_width(float width) { set_content_size(width, content_height()); }
    void set_content_height(float height) { set_content_size(content_width(), height); }
    float content_width() const { return m_content_size.width(); }
    float content_height() const { return m_content_size.height(); }

    Gfx::FloatRect absolute_padding_box_rect() const
    {
        auto absolute_rect = this->absolute_rect();
        Gfx::FloatRect rect;
        rect.set_x(absolute_rect.x() - box_model().padding.left);
        rect.set_width(content_width() + box_model().padding.left + box_model().padding.right);
        rect.set_y(absolute_rect.y() - box_model().padding.top);
        rect.set_height(content_height() + box_model().padding.top + box_model().padding.bottom);
        return rect;
    }

    Gfx::FloatRect absolute_border_box_rect() const
    {
        auto padded_rect = this->absolute_padding_box_rect();
        Gfx::FloatRect rect;
        rect.set_x(padded_rect.x() - box_model().border.left);
        rect.set_width(padded_rect.width() + box_model().border.left + box_model().border.right);
        rect.set_y(padded_rect.y() - box_model().border.top);
        rect.set_height(padded_rect.height() + box_model().border.top + box_model().border.bottom);
        return rect;
    }

    float border_box_width() const
    {
        auto border_box = box_model().border_box();
        return content_width() + border_box.left + border_box.right;
    }

    float border_box_height() const
    {
        auto border_box = box_model().border_box();
        return content_height() + border_box.top + border_box.bottom;
    }

    float absolute_x() const { return absolute_rect().x(); }
    float absolute_y() const { return absolute_rect().y(); }
    Gfx::FloatPoint absolute_position() const { return absolute_rect().location(); }

    bool has_overflow() const { return m_overflow_data.has_value(); }

    Optional<Gfx::FloatRect> scrollable_overflow_rect() const
    {
        if (!m_overflow_data.has_value())
            return {};
        return m_overflow_data->scrollable_overflow_rect;
    }

    void set_overflow_data(Optional<OverflowData> data) { m_overflow_data = move(data); }
    void set_containing_line_box_fragment(Optional<Layout::LineBoxFragmentCoordinate>);

    template<typename Callback>
    void for_each_fragment(Callback callback) const
    {
        for (auto& line_box : line_boxes()) {
            for (auto& fragment : line_box.fragments()) {
                if (callback(fragment) == IterationDecision::Break)
                    return;
            }
        }
    }
};

}
