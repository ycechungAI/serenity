/*
 * Copyright (c) 2022, the SerenityOS developers.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <LibGfx/Painter.h>
#include <LibGfx/StylePainter.h>
#include <LibWeb/Layout/Progress.h>
#include <LibWeb/Painting/ProgressPaintable.h>

namespace Web::Layout {

Progress::Progress(DOM::Document& document, HTML::HTMLProgressElement& element, NonnullRefPtr<CSS::StyleProperties> style)
    : LabelableNode(document, element, move(style))
{
    set_intrinsic_height(12);
}

Progress::~Progress()
{
}

OwnPtr<Painting::Paintable> Progress::create_paintable() const
{
    return Painting::ProgressPaintable::create(*this);
}

}
