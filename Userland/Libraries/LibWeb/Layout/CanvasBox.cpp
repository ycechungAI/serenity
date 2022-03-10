/*
 * Copyright (c) 2020-2022, Andreas Kling <kling@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <LibGfx/Painter.h>
#include <LibWeb/Layout/CanvasBox.h>
#include <LibWeb/Painting/Box.h>

namespace Web::Layout {

CanvasBox::CanvasBox(DOM::Document& document, HTML::HTMLCanvasElement& element, NonnullRefPtr<CSS::StyleProperties> style)
    : ReplacedBox(document, element, move(style))
{
}

CanvasBox::~CanvasBox()
{
}

void CanvasBox::prepare_for_replaced_layout()
{
    set_intrinsic_width(dom_node().width());
    set_intrinsic_height(dom_node().height());
}

void CanvasBox::paint(PaintContext& context, Painting::PaintPhase phase)
{
    if (!is_visible())
        return;

    ReplacedBox::paint(context, phase);

    if (phase == Painting::PaintPhase::Foreground) {
        // FIXME: This should be done at a different level. Also rect() does not include padding etc!
        if (!context.viewport_rect().intersects(enclosing_int_rect(m_paint_box->absolute_rect())))
            return;

        if (dom_node().bitmap())
            context.painter().draw_scaled_bitmap(rounded_int_rect(m_paint_box->absolute_rect()), *dom_node().bitmap(), dom_node().bitmap()->rect(), 1.0f, to_gfx_scaling_mode(computed_values().image_rendering()));
    }
}

}
