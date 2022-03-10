/*
 * Copyright (c) 2022, Andreas Kling <kling@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <LibWeb/Layout/FrameBox.h>
#include <LibWeb/Painting/Paintable.h>

namespace Web::Painting {

class NestedBrowsingContextPaintable final : public Paintable {
public:
    static NonnullOwnPtr<NestedBrowsingContextPaintable> create(Layout::FrameBox const&);

    virtual void paint(PaintContext&, PaintPhase) const override;

    Layout::FrameBox const& layout_box() const;

private:
    NestedBrowsingContextPaintable(Layout::FrameBox const&);
};

}
