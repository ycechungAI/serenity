/*
 * Copyright (c) 2018-2022, Andreas Kling <kling@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <LibGfx/AntiAliasingPainter.h>
#include <LibWeb/DOM/Document.h>
#include <LibWeb/Layout/BlockContainer.h>
#include <LibWeb/Painting/BackgroundPainting.h>
#include <LibWeb/Painting/InlinePaintable.h>

namespace Web::Painting {

JS::NonnullGCPtr<InlinePaintable> InlinePaintable::create(Layout::InlineNode const& layout_node)
{
    return layout_node.heap().allocate_without_realm<InlinePaintable>(layout_node);
}

InlinePaintable::InlinePaintable(Layout::InlineNode const& layout_node)
    : Paintable(layout_node)
{
}

Layout::InlineNode const& InlinePaintable::layout_node() const
{
    return static_cast<Layout::InlineNode const&>(Paintable::layout_node());
}

void InlinePaintable::paint(PaintContext& context, PaintPhase phase) const
{
    auto& painter = context.recording_painter();

    if (phase == PaintPhase::Background) {
        auto containing_block_position_in_absolute_coordinates = containing_block()->paintable_box()->absolute_position();

        for_each_fragment([&](auto const& fragment, bool is_first_fragment, bool is_last_fragment) {
            CSSPixelRect absolute_fragment_rect { containing_block_position_in_absolute_coordinates.translated(fragment.offset()), fragment.size() };

            if (is_first_fragment) {
                auto extra_start_width = box_model().padding.left;
                absolute_fragment_rect.translate_by(-extra_start_width, 0);
                absolute_fragment_rect.set_width(absolute_fragment_rect.width() + extra_start_width);
            }

            if (is_last_fragment) {
                auto extra_end_width = box_model().padding.right;
                absolute_fragment_rect.set_width(absolute_fragment_rect.width() + extra_end_width);
            }

            auto const& border_radii_data = fragment.border_radii_data();
            paint_background(context, layout_node(), absolute_fragment_rect, computed_values().background_color(), computed_values().image_rendering(), &computed_values().background_layers(), border_radii_data);

            if (!box_shadow_data().is_empty()) {
                auto borders_data = BordersData {
                    .top = computed_values().border_top(),
                    .right = computed_values().border_right(),
                    .bottom = computed_values().border_bottom(),
                    .left = computed_values().border_left(),
                };
                auto absolute_fragment_rect_bordered = absolute_fragment_rect.inflated(
                    borders_data.top.width, borders_data.right.width,
                    borders_data.bottom.width, borders_data.left.width);
                paint_box_shadow(context, absolute_fragment_rect_bordered, absolute_fragment_rect,
                    borders_data, border_radii_data, box_shadow_data());
            }

            return IterationDecision::Continue;
        });
    }

    auto paint_border_or_outline = [&](Optional<BordersData> outline_data = {}, CSSPixels outline_offset = 0) {
        auto borders_data = BordersData {
            .top = computed_values().border_top(),
            .right = computed_values().border_right(),
            .bottom = computed_values().border_bottom(),
            .left = computed_values().border_left(),
        };

        auto containing_block_position_in_absolute_coordinates = containing_block()->paintable_box()->absolute_position();

        for_each_fragment([&](auto const& fragment, bool is_first_fragment, bool is_last_fragment) {
            CSSPixelRect absolute_fragment_rect { containing_block_position_in_absolute_coordinates.translated(fragment.offset()), fragment.size() };

            if (is_first_fragment) {
                auto extra_start_width = box_model().padding.left;
                absolute_fragment_rect.translate_by(-extra_start_width, 0);
                absolute_fragment_rect.set_width(absolute_fragment_rect.width() + extra_start_width);
            }

            if (is_last_fragment) {
                auto extra_end_width = box_model().padding.right;
                absolute_fragment_rect.set_width(absolute_fragment_rect.width() + extra_end_width);
            }

            auto borders_rect = absolute_fragment_rect.inflated(borders_data.top.width, borders_data.right.width, borders_data.bottom.width, borders_data.left.width);
            auto border_radii_data = fragment.border_radii_data();

            if (outline_data.has_value()) {
                auto outline_offset_x = outline_offset;
                auto outline_offset_y = outline_offset;
                // "Both the height and the width of the outside of the shape drawn by the outline should not
                // become smaller than twice the computed value of the outline-width property to make sure
                // that an outline can be rendered even with large negative values."
                // https://www.w3.org/TR/css-ui-4/#outline-offset
                // So, if the horizontal outline offset is > half the borders_rect's width then we set it to that.
                // (And the same for y)
                if ((borders_rect.width() / 2) + outline_offset_x < 0)
                    outline_offset_x = -borders_rect.width() / 2;
                if ((borders_rect.height() / 2) + outline_offset_y < 0)
                    outline_offset_y = -borders_rect.height() / 2;

                border_radii_data.inflate(outline_data->top.width + outline_offset_y, outline_data->right.width + outline_offset_x, outline_data->bottom.width + outline_offset_y, outline_data->left.width + outline_offset_x);
                borders_rect.inflate(outline_data->top.width + outline_offset_y, outline_data->right.width + outline_offset_x, outline_data->bottom.width + outline_offset_y, outline_data->left.width + outline_offset_x);
                context.recording_painter().paint_borders(context.rounded_device_rect(borders_rect), border_radii_data.as_corners(context), outline_data->to_device_pixels(context));
            } else {
                context.recording_painter().paint_borders(context.rounded_device_rect(borders_rect), border_radii_data.as_corners(context), borders_data.to_device_pixels(context));
            }

            return IterationDecision::Continue;
        });
    };

    if (phase == PaintPhase::Border) {
        paint_border_or_outline();
    }

    if (phase == PaintPhase::Outline) {
        auto outline_width = computed_values().outline_width().to_px(layout_node());
        auto maybe_outline_data = borders_data_for_outline(layout_node(), computed_values().outline_color(), computed_values().outline_style(), outline_width);
        if (maybe_outline_data.has_value()) {
            paint_border_or_outline(maybe_outline_data.value(), computed_values().outline_offset().to_px(layout_node()));
        }
    }

    if (phase == PaintPhase::Foreground) {
        for_each_fragment([&](auto const& fragment, bool, bool) {
            if (is<Layout::TextNode>(fragment.layout_node()))
                paint_text_fragment(context, static_cast<Layout::TextNode const&>(fragment.layout_node()), fragment, phase);
        });
    }

    if (phase == PaintPhase::Overlay && layout_node().document().inspected_layout_node() == &layout_node()) {
        // FIXME: This paints a double-thick border between adjacent fragments, where ideally there
        //        would be none. Once we implement non-rectangular outlines for the `outline` CSS
        //        property, we can use that here instead.
        for_each_fragment([&](auto const& fragment, bool, bool) {
            painter.draw_rect(context.enclosing_device_rect(fragment.absolute_rect()).template to_type<int>(), Color::Magenta);
            return IterationDecision::Continue;
        });
    }
}

template<typename Callback>
void InlinePaintable::for_each_fragment(Callback callback) const
{
    for (size_t i = 0; i < m_fragments.size(); ++i) {
        auto const& fragment = m_fragments[i];
        callback(fragment, i == 0, i == m_fragments.size() - 1);
    }
}

Optional<HitTestResult> InlinePaintable::hit_test(CSSPixelPoint position, HitTestType type) const
{
    for (auto& fragment : m_fragments) {
        if (fragment.paintable().stacking_context())
            continue;
        auto fragment_absolute_rect = fragment.absolute_rect();
        if (fragment_absolute_rect.contains(position)) {
            if (auto result = fragment.paintable().hit_test(position, type); result.has_value())
                return result;
            return HitTestResult { const_cast<Paintable&>(fragment.paintable()),
                fragment.text_index_at(position.x()) };
        }
    }

    Optional<HitTestResult> hit_test_result;
    for_each_child([&](Paintable const& child) {
        if (child.stacking_context())
            return IterationDecision::Continue;
        if (auto result = child.hit_test(position, type); result.has_value()) {
            hit_test_result = result;
            return IterationDecision::Break;
        }
        return IterationDecision::Continue;
    });

    return hit_test_result;
}

CSSPixelRect InlinePaintable::bounding_rect() const
{
    auto top = CSSPixels::max();
    auto left = CSSPixels::max();
    auto right = CSSPixels::min();
    auto bottom = CSSPixels::min();
    auto has_fragments = false;
    for_each_fragment([&](auto const& fragment, bool, bool) {
        has_fragments = true;
        auto fragment_absolute_rect = fragment.absolute_rect();
        if (fragment_absolute_rect.top() < top)
            top = fragment_absolute_rect.top();
        if (fragment_absolute_rect.left() < left)
            left = fragment_absolute_rect.left();
        if (fragment_absolute_rect.right() > right)
            right = fragment_absolute_rect.right();
        if (fragment_absolute_rect.bottom() > bottom)
            bottom = fragment_absolute_rect.bottom();
    });

    if (!has_fragments) {
        // FIXME: This is adhoc, and we should return rect of empty fragment instead.
        auto containing_block_position_in_absolute_coordinates = containing_block()->paintable_box()->absolute_position();
        return { containing_block_position_in_absolute_coordinates, { 0, 0 } };
    }
    return { left, top, right - left, bottom - top };
}

}
