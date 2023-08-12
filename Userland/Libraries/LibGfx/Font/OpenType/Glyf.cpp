/*
 * Copyright (c) 2020, Srimanta Barua <srimanta.barua1@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <LibGfx/AntiAliasingPainter.h>
#include <LibGfx/Font/OpenType/Glyf.h>
#include <LibGfx/Painter.h>
#include <LibGfx/Path.h>
#include <LibGfx/Point.h>

namespace OpenType {

extern u16 be_u16(u8 const* ptr);
extern u32 be_u32(u8 const* ptr);
extern i16 be_i16(u8 const* ptr);
extern float be_fword(u8 const* ptr);

enum class SimpleGlyfFlags {
    // From spec.
    OnCurve = 0x01,
    XShortVector = 0x02,
    YShortVector = 0x04,
    RepeatFlag = 0x08,
    XIsSameOrPositiveXShortVector = 0x10,
    YIsSameOrPositiveYShortVector = 0x20,
    // Combinations
    XMask = 0x12,
    YMask = 0x24,
    XLongVector = 0x00,
    YLongVector = 0x00,
    XNegativeShortVector = 0x02,
    YNegativeShortVector = 0x04,
    XPositiveShortVector = 0x12,
    YPositiveShortVector = 0x24,
};

class PointIterator {
public:
    struct Item {
        bool on_curve;
        Gfx::FloatPoint point;
    };

    PointIterator(ReadonlyBytes slice, u16 num_points, u32 flags_offset, u32 x_offset, u32 y_offset, Gfx::AffineTransform affine)
        : m_slice(slice)
        , m_points_remaining(num_points)
        , m_flags_offset(flags_offset)
        , m_x_offset(x_offset)
        , m_y_offset(y_offset)
        , m_affine(affine)
    {
    }

    Optional<Item> next()
    {
        if (m_points_remaining == 0) {
            return {};
        }
        if (m_flags_remaining > 0) {
            m_flags_remaining--;
        } else {
            m_flag = m_slice[m_flags_offset++];
            if (m_flag & (u8)SimpleGlyfFlags::RepeatFlag) {
                m_flags_remaining = m_slice[m_flags_offset++];
            }
        }
        switch (m_flag & (u8)SimpleGlyfFlags::XMask) {
        case (u8)SimpleGlyfFlags::XLongVector:
            m_last_point.set_x(m_last_point.x() + be_i16(m_slice.offset_pointer(m_x_offset)));
            m_x_offset += 2;
            break;
        case (u8)SimpleGlyfFlags::XNegativeShortVector:
            m_last_point.set_x(m_last_point.x() - m_slice[m_x_offset++]);
            break;
        case (u8)SimpleGlyfFlags::XPositiveShortVector:
            m_last_point.set_x(m_last_point.x() + m_slice[m_x_offset++]);
            break;
        default:
            break;
        }
        switch (m_flag & (u8)SimpleGlyfFlags::YMask) {
        case (u8)SimpleGlyfFlags::YLongVector:
            m_last_point.set_y(m_last_point.y() + be_i16(m_slice.offset_pointer(m_y_offset)));
            m_y_offset += 2;
            break;
        case (u8)SimpleGlyfFlags::YNegativeShortVector:
            m_last_point.set_y(m_last_point.y() - m_slice[m_y_offset++]);
            break;
        case (u8)SimpleGlyfFlags::YPositiveShortVector:
            m_last_point.set_y(m_last_point.y() + m_slice[m_y_offset++]);
            break;
        default:
            break;
        }
        m_points_remaining--;
        Item ret = {
            .on_curve = (m_flag & (u8)SimpleGlyfFlags::OnCurve) != 0,
            .point = m_affine.map(m_last_point),
        };
        return ret;
    }

private:
    ReadonlyBytes m_slice;
    u16 m_points_remaining;
    u8 m_flag { 0 };
    Gfx::FloatPoint m_last_point = { 0.0f, 0.0f };
    u32 m_flags_remaining = { 0 };
    u32 m_flags_offset;
    u32 m_x_offset;
    u32 m_y_offset;
    Gfx::AffineTransform m_affine;
};

Optional<Glyf::Glyph::ComponentIterator::Item> Glyf::Glyph::ComponentIterator::next()
{
    if (!m_has_more) {
        return {};
    }
    u16 flags = be_u16(m_slice.offset_pointer(m_offset));
    m_offset += 2;
    u16 glyph_id = be_u16(m_slice.offset_pointer(m_offset));
    m_offset += 2;
    i16 arg1 = 0, arg2 = 0;
    if (flags & (u16)CompositeFlags::Arg1AndArg2AreWords) {
        arg1 = be_i16(m_slice.offset_pointer(m_offset));
        m_offset += 2;
        arg2 = be_i16(m_slice.offset_pointer(m_offset));
        m_offset += 2;
    } else {
        arg1 = (i8)m_slice[m_offset++];
        arg2 = (i8)m_slice[m_offset++];
    }
    float a = 1.0, b = 0.0, c = 0.0, d = 1.0, e = 0.0, f = 0.0;
    if (flags & (u16)CompositeFlags::WeHaveATwoByTwo) {
        a = be_fword(m_slice.offset_pointer(m_offset));
        m_offset += 2;
        b = be_fword(m_slice.offset_pointer(m_offset));
        m_offset += 2;
        c = be_fword(m_slice.offset_pointer(m_offset));
        m_offset += 2;
        d = be_fword(m_slice.offset_pointer(m_offset));
        m_offset += 2;
    } else if (flags & (u16)CompositeFlags::WeHaveAnXAndYScale) {
        a = be_fword(m_slice.offset_pointer(m_offset));
        m_offset += 2;
        d = be_fword(m_slice.offset_pointer(m_offset));
        m_offset += 2;
    } else if (flags & (u16)CompositeFlags::WeHaveAScale) {
        a = be_fword(m_slice.offset_pointer(m_offset));
        m_offset += 2;
        d = a;
    }
    // FIXME: Handle UseMyMetrics, ScaledComponentOffset, UnscaledComponentOffset, non-ArgsAreXYValues
    if (flags & (u16)CompositeFlags::ArgsAreXYValues) {
        e = arg1;
        f = arg2;
    } else {
        // FIXME: Implement this. There's no TODO() here since many fonts work just fine without this.
    }
    if (flags & (u16)CompositeFlags::UseMyMetrics) {
        // FIXME: Implement this. There's no TODO() here since many fonts work just fine without this.
    }
    if (flags & (u16)CompositeFlags::ScaledComponentOffset) {
        // FIXME: Implement this. There's no TODO() here since many fonts work just fine without this.
    }
    if (flags & (u16)CompositeFlags::UnscaledComponentOffset) {
        // FIXME: Implement this. There's no TODO() here since many fonts work just fine without this.
    }
    m_has_more = (flags & (u16)CompositeFlags::MoreComponents);
    return Item {
        .glyph_id = glyph_id,
        .affine = Gfx::AffineTransform(a, b, c, d, e, f),
    };
}

Optional<Loca> Loca::from_slice(ReadonlyBytes slice, u32 num_glyphs, IndexToLocFormat index_to_loc_format)
{
    switch (index_to_loc_format) {
    case IndexToLocFormat::Offset16:
        if (slice.size() < num_glyphs * 2) {
            return {};
        }
        break;
    case IndexToLocFormat::Offset32:
        if (slice.size() < num_glyphs * 4) {
            return {};
        }
        break;
    }
    return Loca(slice, num_glyphs, index_to_loc_format);
}

u32 Loca::get_glyph_offset(u32 glyph_id) const
{
    // NOTE: The value of n is numGlyphs + 1.
    VERIFY(glyph_id <= m_num_glyphs);
    switch (m_index_to_loc_format) {
    case IndexToLocFormat::Offset16:
        return ((u32)be_u16(m_slice.offset_pointer(glyph_id * 2))) * 2;
    case IndexToLocFormat::Offset32:
        return be_u32(m_slice.offset_pointer(glyph_id * 4));
    default:
        VERIFY_NOT_REACHED();
    }
}

static void get_ttglyph_offsets(ReadonlyBytes slice, u32 num_points, u32 flags_offset, u32* x_offset, u32* y_offset)
{
    u32 flags_size = 0;
    u32 x_size = 0;
    u32 repeat_count;
    while (num_points > 0) {
        u8 flag = slice[flags_offset + flags_size];
        if (flag & (u8)SimpleGlyfFlags::RepeatFlag) {
            flags_size++;
            repeat_count = slice[flags_offset + flags_size] + 1;
        } else {
            repeat_count = 1;
        }
        flags_size++;
        switch (flag & (u8)SimpleGlyfFlags::XMask) {
        case (u8)SimpleGlyfFlags::XLongVector:
            x_size += repeat_count * 2;
            break;
        case (u8)SimpleGlyfFlags::XNegativeShortVector:
        case (u8)SimpleGlyfFlags::XPositiveShortVector:
            x_size += repeat_count;
            break;
        default:
            break;
        }
        num_points -= repeat_count;
    }
    *x_offset = flags_offset + flags_size;
    *y_offset = *x_offset + x_size;
}

ReadonlyBytes Glyf::Glyph::program() const
{
    auto instructions_start = m_num_contours * 2;
    u16 num_instructions = be_u16(m_slice.offset_pointer(instructions_start));
    return m_slice.slice(instructions_start + 2, num_instructions);
}

void Glyf::Glyph::rasterize_impl(Gfx::Painter& painter, Gfx::AffineTransform const& transform) const
{
    // Get offset for flags, x, and y.
    u16 num_points = be_u16(m_slice.offset_pointer((m_num_contours - 1) * 2)) + 1;
    u16 num_instructions = be_u16(m_slice.offset_pointer(m_num_contours * 2));
    u32 flags_offset = m_num_contours * 2 + 2 + num_instructions;
    u32 x_offset = 0;
    u32 y_offset = 0;
    get_ttglyph_offsets(m_slice, num_points, flags_offset, &x_offset, &y_offset);

    // Prepare to render glyph.
    Gfx::Path path;
    PointIterator point_iterator(m_slice, num_points, flags_offset, x_offset, y_offset, transform);

    u32 current_point_index = 0;
    for (u16 contour_index = 0; contour_index < m_num_contours; contour_index++) {
        u32 current_contour_last_point_index = be_u16(m_slice.offset_pointer(contour_index * 2));
        Optional<Gfx::FloatPoint> start_off_curve_point;
        Optional<Gfx::FloatPoint> start_on_curve_point;
        Optional<Gfx::FloatPoint> unprocessed_off_curve_point;
        while (current_point_index <= current_contour_last_point_index) {
            auto current_point = point_iterator.next();
            current_point_index++;
            if (!current_point.has_value())
                break;

            if (current_point->on_curve) {
                if (!start_on_curve_point.has_value()) {
                    start_on_curve_point = current_point->point;
                    path.move_to(current_point->point);
                }

                if (unprocessed_off_curve_point.has_value()) {
                    path.quadratic_bezier_curve_to(unprocessed_off_curve_point.value(), current_point->point);
                    unprocessed_off_curve_point = {};
                } else {
                    path.line_to(current_point->point);
                }
            } else {
                if (!start_on_curve_point.has_value() && !start_off_curve_point.has_value()) {
                    // If "off curve" point comes first it needs to be saved to use while closing the path
                    start_off_curve_point = current_point->point;
                }

                if (unprocessed_off_curve_point.has_value()) {
                    // Two subsequent "off curve" points create implied "on-curve" point lying between them
                    auto implied_point = (unprocessed_off_curve_point.value() + current_point->point) * 0.5f;
                    if (!start_on_curve_point.has_value()) {
                        start_on_curve_point = implied_point;
                        path.move_to(implied_point);
                    }
                    path.quadratic_bezier_curve_to(unprocessed_off_curve_point.value(), implied_point);
                }
                unprocessed_off_curve_point = current_point->point;
            }
        }

        if (start_off_curve_point.has_value()) {
            // Close the path creating "implied" point if both first and last points were "off curve"
            if (unprocessed_off_curve_point.has_value()) {
                auto implied_point = (start_off_curve_point.value() + unprocessed_off_curve_point.value()) * 0.5f;
                path.quadratic_bezier_curve_to(unprocessed_off_curve_point.value(), implied_point);
            }

            // Add bezier curve from new "implied point" to first "on curve" point in the path
            path.quadratic_bezier_curve_to(start_off_curve_point.value(), start_on_curve_point.value());
        } else if (unprocessed_off_curve_point.has_value()) {
            // Add bezier curve to first "on curve" point using last "off curve" point
            path.quadratic_bezier_curve_to(unprocessed_off_curve_point.value(), start_on_curve_point.value());
        } else {
            path.line_to(start_on_curve_point.value());
        }
    }

    constexpr auto base_color = Color::White;
    Gfx::AntiAliasingPainter aa_painter { painter };
    aa_painter.fill_path(path, base_color);
}

RefPtr<Gfx::Bitmap> Glyf::Glyph::rasterize_simple(i16 font_ascender, i16 font_descender, float x_scale, float y_scale, Gfx::GlyphSubpixelOffset subpixel_offset) const
{
    u32 width = (u32)(ceilf((m_xmax - m_xmin) * x_scale)) + 2;
    u32 height = (u32)(ceilf((font_ascender - font_descender) * y_scale)) + 2;
    auto bitmap = Gfx::Bitmap::create(Gfx::BitmapFormat::BGRA8888, { width, height }).release_value_but_fixme_should_propagate_errors();
    auto affine = Gfx::AffineTransform()
                      .translate(subpixel_offset.to_float_point())
                      .scale(x_scale, -y_scale)
                      .translate(-m_xmin, -font_ascender);
    Gfx::Painter painter { bitmap };
    rasterize_impl(painter, affine);
    return bitmap;
}

Optional<Glyf::Glyph> Glyf::glyph(u32 offset) const
{
    if (offset + sizeof(GlyphHeader) > m_slice.size())
        return {};
    VERIFY(m_slice.size() >= offset + sizeof(GlyphHeader));
    auto const& glyph_header = *bit_cast<GlyphHeader const*>(m_slice.offset_pointer(offset));
    i16 num_contours = glyph_header.number_of_contours;
    i16 xmin = glyph_header.x_min;
    i16 ymin = glyph_header.y_min;
    i16 xmax = glyph_header.x_max;
    i16 ymax = glyph_header.y_max;
    auto slice = m_slice.slice(offset + sizeof(GlyphHeader));
    return Glyph(slice, xmin, ymin, xmax, ymax, num_contours);
}

}
