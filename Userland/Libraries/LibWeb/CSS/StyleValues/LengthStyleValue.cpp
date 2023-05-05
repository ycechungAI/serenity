/*
 * Copyright (c) 2018-2020, Andreas Kling <kling@serenityos.org>
 * Copyright (c) 2021, Tobias Christiansen <tobyase@serenityos.org>
 * Copyright (c) 2021-2023, Sam Atkins <atkinssj@serenityos.org>
 * Copyright (c) 2022-2023, MacDue <macdue@dueutil.tech>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "LengthStyleValue.h"

namespace Web::CSS {

ErrorOr<ValueComparingNonnullRefPtr<LengthStyleValue>> LengthStyleValue::create(Length const& length)
{
    VERIFY(!length.is_auto());
    if (length.is_px()) {
        if (length.raw_value() == 0) {
            static auto value = TRY(adopt_nonnull_ref_or_enomem(new (nothrow) LengthStyleValue(CSS::Length::make_px(0))));
            return value;
        }
        if (length.raw_value() == 1) {
            static auto value = TRY(adopt_nonnull_ref_or_enomem(new (nothrow) LengthStyleValue(CSS::Length::make_px(1))));
            return value;
        }
    }
    return adopt_nonnull_ref_or_enomem(new (nothrow) LengthStyleValue(length));
}

ValueComparingNonnullRefPtr<StyleValue const> LengthStyleValue::absolutized(CSSPixelRect const& viewport_rect, Length::FontMetrics const& font_metrics, Length::FontMetrics const& root_font_metrics) const
{
    if (auto length = m_length.absolutize(viewport_rect, font_metrics, root_font_metrics); length.has_value())
        return LengthStyleValue::create(length.release_value()).release_value_but_fixme_should_propagate_errors();
    return *this;
}

}
