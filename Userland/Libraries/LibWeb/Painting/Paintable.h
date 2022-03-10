/*
 * Copyright (c) 2022, Andreas Kling <kling@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/NonnullOwnPtr.h>
#include <LibWeb/Layout/Box.h>
#include <LibWeb/Layout/LineBox.h>
#include <LibWeb/Painting/StackingContext.h>

namespace Web::Painting {

class Paintable {
    AK_MAKE_NONMOVABLE(Paintable);
    AK_MAKE_NONCOPYABLE(Paintable);

public:
    virtual ~Paintable() = default;

    virtual void paint(PaintContext&, PaintPhase) const { }
    virtual void before_children_paint(PaintContext&, PaintPhase) const { }
    virtual void after_children_paint(PaintContext&, PaintPhase) const { }

    Layout::Node const& layout_node() const { return m_layout_node; }
    auto const& computed_values() const { return m_layout_node.computed_values(); }

protected:
    explicit Paintable(Layout::Node const& layout_node)
        : m_layout_node(layout_node)
    {
    }

private:
    Layout::Node const& m_layout_node;
};

class PaintableBox : public Paintable {
public:
    static NonnullOwnPtr<PaintableBox> create(Layout::Box const&);
    virtual ~PaintableBox();

    virtual void paint(PaintContext&, PaintPhase) const override;

    bool is_visible() const { return layout_box().is_visible(); }

    Layout::Box const& layout_box() const { return static_cast<Layout::Box const&>(Paintable::layout_node()); }

    auto const& box_model() const { return layout_box().box_model(); }

    struct OverflowData {
        Gfx::FloatRect scrollable_overflow_rect;
        Gfx::FloatPoint scroll_offset;
    };
    Optional<OverflowData> m_overflow_data;

    Gfx::FloatPoint m_offset;
    Gfx::FloatSize m_content_size;

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

    StackingContext* stacking_context() { return m_stacking_context; }
    StackingContext const* stacking_context() const { return m_stacking_context; }
    void set_stacking_context(NonnullOwnPtr<Painting::StackingContext> context) { m_stacking_context = move(context); }
    StackingContext* enclosing_stacking_context();

    DOM::Node const* dom_node() const { return layout_box().dom_node(); }
    DOM::Document const& document() const { return layout_box().document(); }

    virtual void before_children_paint(PaintContext&, PaintPhase) const override;
    virtual void after_children_paint(PaintContext&, PaintPhase) const override;

protected:
    explicit PaintableBox(Layout::Box const&);

    virtual void paint_border(PaintContext&) const;
    virtual void paint_background(PaintContext&) const;
    virtual void paint_box_shadow(PaintContext&) const;

private:
    Painting::BorderRadiusData normalized_border_radius_data() const;

    OwnPtr<Painting::StackingContext> m_stacking_context;
};

class PaintableWithLines : public PaintableBox {
public:
    static NonnullOwnPtr<PaintableWithLines> create(Layout::BlockContainer const& block_container)
    {
        return adopt_own(*new PaintableWithLines(block_container));
    }
    virtual ~PaintableWithLines() override;

    Vector<Layout::LineBox> const& line_boxes() const { return m_line_boxes; }
    void set_line_boxes(Vector<Layout::LineBox>&& line_boxes) { m_line_boxes = move(line_boxes); }

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

    virtual void paint(PaintContext&, PaintPhase) const override;

private:
    PaintableWithLines(Layout::BlockContainer const&);

    Vector<Layout::LineBox> m_line_boxes;
};

}
