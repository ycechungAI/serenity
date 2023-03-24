/*
 * Copyright (c) 2018-2020, Andreas Kling <kling@serenityos.org>
 * Copyright (c) 2021, Tobias Christiansen <tobyase@serenityos.org>
 * Copyright (c) 2021-2023, Sam Atkins <atkinssj@serenityos.org>
 * Copyright (c) 2022-2023, MacDue <macdue@dueutil.tech>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "LinearGradientStyleValue.h"
#include <LibWeb/CSS/Serialize.h>

namespace Web::CSS {

// FIXME: Temporary until AbstractImageStyleValue.h exists. (And the Serialize.h include above.)
static ErrorOr<void> serialize_color_stop_list(StringBuilder& builder, auto const& color_stop_list)
{
    bool first = true;
    for (auto const& element : color_stop_list) {
        if (!first)
            TRY(builder.try_append(", "sv));

        if (element.transition_hint.has_value())
            TRY(builder.try_appendff("{}, "sv, TRY(element.transition_hint->value.to_string())));

        TRY(serialize_a_srgb_value(builder, element.color_stop.color));
        for (auto position : Array { &element.color_stop.position, &element.color_stop.second_position }) {
            if (position->has_value())
                TRY(builder.try_appendff(" {}"sv, TRY((*position)->to_string())));
        }
        first = false;
    }
    return {};
}

ErrorOr<String> LinearGradientStyleValue::to_string() const
{
    StringBuilder builder;
    auto side_or_corner_to_string = [](SideOrCorner value) {
        switch (value) {
        case SideOrCorner::Top:
            return "top"sv;
        case SideOrCorner::Bottom:
            return "bottom"sv;
        case SideOrCorner::Left:
            return "left"sv;
        case SideOrCorner::Right:
            return "right"sv;
        case SideOrCorner::TopLeft:
            return "top left"sv;
        case SideOrCorner::TopRight:
            return "top right"sv;
        case SideOrCorner::BottomLeft:
            return "bottom left"sv;
        case SideOrCorner::BottomRight:
            return "bottom right"sv;
        default:
            VERIFY_NOT_REACHED();
        }
    };

    if (m_properties.gradient_type == GradientType::WebKit)
        TRY(builder.try_append("-webkit-"sv));
    if (is_repeating())
        TRY(builder.try_append("repeating-"sv));
    TRY(builder.try_append("linear-gradient("sv));
    TRY(m_properties.direction.visit(
        [&](SideOrCorner side_or_corner) -> ErrorOr<void> {
            return builder.try_appendff("{}{}, "sv, m_properties.gradient_type == GradientType::Standard ? "to "sv : ""sv, side_or_corner_to_string(side_or_corner));
        },
        [&](Angle const& angle) -> ErrorOr<void> {
            return builder.try_appendff("{}, "sv, TRY(angle.to_string()));
        }));

    TRY(serialize_color_stop_list(builder, m_properties.color_stop_list));
    TRY(builder.try_append(")"sv));
    return builder.to_string();
}

bool LinearGradientStyleValue::equals(StyleValue const& other_) const
{
    if (type() != other_.type())
        return false;
    auto& other = other_.as_linear_gradient();
    return m_properties == other.m_properties;
}

float LinearGradientStyleValue::angle_degrees(CSSPixelSize gradient_size) const
{
    auto corner_angle_degrees = [&] {
        return static_cast<float>(atan2(gradient_size.height().value(), gradient_size.width().value())) * 180 / AK::Pi<float>;
    };
    return m_properties.direction.visit(
        [&](SideOrCorner side_or_corner) {
            auto angle = [&] {
                switch (side_or_corner) {
                case SideOrCorner::Top:
                    return 0.0f;
                case SideOrCorner::Bottom:
                    return 180.0f;
                case SideOrCorner::Left:
                    return 270.0f;
                case SideOrCorner::Right:
                    return 90.0f;
                case SideOrCorner::TopRight:
                    return corner_angle_degrees();
                case SideOrCorner::BottomLeft:
                    return corner_angle_degrees() + 180.0f;
                case SideOrCorner::TopLeft:
                    return -corner_angle_degrees();
                case SideOrCorner::BottomRight:
                    return -(corner_angle_degrees() + 180.0f);
                default:
                    VERIFY_NOT_REACHED();
                }
            }();
            // Note: For unknowable reasons the angles are opposite on the -webkit- version
            if (m_properties.gradient_type == GradientType::WebKit)
                return angle + 180.0f;
            return angle;
        },
        [&](Angle const& angle) {
            return angle.to_degrees();
        });
}

void LinearGradientStyleValue::resolve_for_size(Layout::Node const& node, CSSPixelSize size) const
{
    if (m_resolved.has_value() && m_resolved->size == size)
        return;
    m_resolved = ResolvedData { Painting::resolve_linear_gradient_data(node, size, *this), size };
}

void LinearGradientStyleValue::paint(PaintContext& context, DevicePixelRect const& dest_rect, CSS::ImageRendering) const
{
    VERIFY(m_resolved.has_value());
    Painting::paint_linear_gradient(context, dest_rect, m_resolved->data);
}

}
