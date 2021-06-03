/*
 * Copyright (c) 2021, Liav A. <liavalb@hotmail.co.il>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <Kernel/Graphics/Console/FramebufferConsole.h>
#include <Kernel/TTY/ConsoleManagement.h>

namespace Kernel::Graphics {

constexpr unsigned char const font8x8_basic[128][8] = {
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // U+0000 (nul)
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // U+0001
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // U+0002
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // U+0003
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // U+0004
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // U+0005
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // U+0006
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // U+0007
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // U+0008
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // U+0009
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // U+000A
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // U+000B
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // U+000C
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // U+000D
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // U+000E
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // U+000F
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // U+0010
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // U+0011
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // U+0012
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // U+0013
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // U+0014
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // U+0015
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // U+0016
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // U+0017
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // U+0018
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // U+0019
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // U+001A
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // U+001B
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // U+001C
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // U+001D
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // U+001E
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // U+001F
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // U+0020 (space)
    { 0x18, 0x3C, 0x3C, 0x18, 0x18, 0x00, 0x18, 0x00 }, // U+0021 (!)
    { 0x36, 0x36, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // U+0022 (")
    { 0x36, 0x36, 0x7F, 0x36, 0x7F, 0x36, 0x36, 0x00 }, // U+0023 (#)
    { 0x0C, 0x3E, 0x03, 0x1E, 0x30, 0x1F, 0x0C, 0x00 }, // U+0024 ($)
    { 0x00, 0x63, 0x33, 0x18, 0x0C, 0x66, 0x63, 0x00 }, // U+0025 (%)
    { 0x1C, 0x36, 0x1C, 0x6E, 0x3B, 0x33, 0x6E, 0x00 }, // U+0026 (&)
    { 0x06, 0x06, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00 }, // U+0027 (')
    { 0x18, 0x0C, 0x06, 0x06, 0x06, 0x0C, 0x18, 0x00 }, // U+0028 (()
    { 0x06, 0x0C, 0x18, 0x18, 0x18, 0x0C, 0x06, 0x00 }, // U+0029 ())
    { 0x00, 0x66, 0x3C, 0xFF, 0x3C, 0x66, 0x00, 0x00 }, // U+002A (*)
    { 0x00, 0x0C, 0x0C, 0x3F, 0x0C, 0x0C, 0x00, 0x00 }, // U+002B (+)
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C, 0x06 }, // U+002C (,)
    { 0x00, 0x00, 0x00, 0x3F, 0x00, 0x00, 0x00, 0x00 }, // U+002D (-)
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C, 0x00 }, // U+002E (.)
    { 0x60, 0x30, 0x18, 0x0C, 0x06, 0x03, 0x01, 0x00 }, // U+002F (/)
    { 0x3E, 0x63, 0x73, 0x7B, 0x6F, 0x67, 0x3E, 0x00 }, // U+0030 (0)
    { 0x0C, 0x0E, 0x0C, 0x0C, 0x0C, 0x0C, 0x3F, 0x00 }, // U+0031 (1)
    { 0x1E, 0x33, 0x30, 0x1C, 0x06, 0x33, 0x3F, 0x00 }, // U+0032 (2)
    { 0x1E, 0x33, 0x30, 0x1C, 0x30, 0x33, 0x1E, 0x00 }, // U+0033 (3)
    { 0x38, 0x3C, 0x36, 0x33, 0x7F, 0x30, 0x78, 0x00 }, // U+0034 (4)
    { 0x3F, 0x03, 0x1F, 0x30, 0x30, 0x33, 0x1E, 0x00 }, // U+0035 (5)
    { 0x1C, 0x06, 0x03, 0x1F, 0x33, 0x33, 0x1E, 0x00 }, // U+0036 (6)
    { 0x3F, 0x33, 0x30, 0x18, 0x0C, 0x0C, 0x0C, 0x00 }, // U+0037 (7)
    { 0x1E, 0x33, 0x33, 0x1E, 0x33, 0x33, 0x1E, 0x00 }, // U+0038 (8)
    { 0x1E, 0x33, 0x33, 0x3E, 0x30, 0x18, 0x0E, 0x00 }, // U+0039 (9)
    { 0x00, 0x0C, 0x0C, 0x00, 0x00, 0x0C, 0x0C, 0x00 }, // U+003A (:)
    { 0x00, 0x0C, 0x0C, 0x00, 0x00, 0x0C, 0x0C, 0x06 }, // U+003B (;)
    { 0x18, 0x0C, 0x06, 0x03, 0x06, 0x0C, 0x18, 0x00 }, // U+003C (<)
    { 0x00, 0x00, 0x3F, 0x00, 0x00, 0x3F, 0x00, 0x00 }, // U+003D (=)
    { 0x06, 0x0C, 0x18, 0x30, 0x18, 0x0C, 0x06, 0x00 }, // U+003E (>)
    { 0x1E, 0x33, 0x30, 0x18, 0x0C, 0x00, 0x0C, 0x00 }, // U+003F (?)
    { 0x3E, 0x63, 0x7B, 0x7B, 0x7B, 0x03, 0x1E, 0x00 }, // U+0040 (@)
    { 0x0C, 0x1E, 0x33, 0x33, 0x3F, 0x33, 0x33, 0x00 }, // U+0041 (A)
    { 0x3F, 0x66, 0x66, 0x3E, 0x66, 0x66, 0x3F, 0x00 }, // U+0042 (B)
    { 0x3C, 0x66, 0x03, 0x03, 0x03, 0x66, 0x3C, 0x00 }, // U+0043 (C)
    { 0x1F, 0x36, 0x66, 0x66, 0x66, 0x36, 0x1F, 0x00 }, // U+0044 (D)
    { 0x7F, 0x46, 0x16, 0x1E, 0x16, 0x46, 0x7F, 0x00 }, // U+0045 (E)
    { 0x7F, 0x46, 0x16, 0x1E, 0x16, 0x06, 0x0F, 0x00 }, // U+0046 (F)
    { 0x3C, 0x66, 0x03, 0x03, 0x73, 0x66, 0x7C, 0x00 }, // U+0047 (G)
    { 0x33, 0x33, 0x33, 0x3F, 0x33, 0x33, 0x33, 0x00 }, // U+0048 (H)
    { 0x1E, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x1E, 0x00 }, // U+0049 (I)
    { 0x78, 0x30, 0x30, 0x30, 0x33, 0x33, 0x1E, 0x00 }, // U+004A (J)
    { 0x67, 0x66, 0x36, 0x1E, 0x36, 0x66, 0x67, 0x00 }, // U+004B (K)
    { 0x0F, 0x06, 0x06, 0x06, 0x46, 0x66, 0x7F, 0x00 }, // U+004C (L)
    { 0x63, 0x77, 0x7F, 0x7F, 0x6B, 0x63, 0x63, 0x00 }, // U+004D (M)
    { 0x63, 0x67, 0x6F, 0x7B, 0x73, 0x63, 0x63, 0x00 }, // U+004E (N)
    { 0x1C, 0x36, 0x63, 0x63, 0x63, 0x36, 0x1C, 0x00 }, // U+004F (O)
    { 0x3F, 0x66, 0x66, 0x3E, 0x06, 0x06, 0x0F, 0x00 }, // U+0050 (P)
    { 0x1E, 0x33, 0x33, 0x33, 0x3B, 0x1E, 0x38, 0x00 }, // U+0051 (Q)
    { 0x3F, 0x66, 0x66, 0x3E, 0x36, 0x66, 0x67, 0x00 }, // U+0052 (R)
    { 0x1E, 0x33, 0x07, 0x0E, 0x38, 0x33, 0x1E, 0x00 }, // U+0053 (S)
    { 0x3F, 0x2D, 0x0C, 0x0C, 0x0C, 0x0C, 0x1E, 0x00 }, // U+0054 (T)
    { 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x3F, 0x00 }, // U+0055 (U)
    { 0x33, 0x33, 0x33, 0x33, 0x33, 0x1E, 0x0C, 0x00 }, // U+0056 (V)
    { 0x63, 0x63, 0x63, 0x6B, 0x7F, 0x77, 0x63, 0x00 }, // U+0057 (W)
    { 0x63, 0x63, 0x36, 0x1C, 0x1C, 0x36, 0x63, 0x00 }, // U+0058 (X)
    { 0x33, 0x33, 0x33, 0x1E, 0x0C, 0x0C, 0x1E, 0x00 }, // U+0059 (Y)
    { 0x7F, 0x63, 0x31, 0x18, 0x4C, 0x66, 0x7F, 0x00 }, // U+005A (Z)
    { 0x1E, 0x06, 0x06, 0x06, 0x06, 0x06, 0x1E, 0x00 }, // U+005B ([)
    { 0x03, 0x06, 0x0C, 0x18, 0x30, 0x60, 0x40, 0x00 }, // U+005C (\)
    { 0x1E, 0x18, 0x18, 0x18, 0x18, 0x18, 0x1E, 0x00 }, // U+005D (])
    { 0x08, 0x1C, 0x36, 0x63, 0x00, 0x00, 0x00, 0x00 }, // U+005E (^)
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF }, // U+005F (_)
    { 0x0C, 0x0C, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00 }, // U+0060 (`)
    { 0x00, 0x00, 0x1E, 0x30, 0x3E, 0x33, 0x6E, 0x00 }, // U+0061 (a)
    { 0x07, 0x06, 0x06, 0x3E, 0x66, 0x66, 0x3B, 0x00 }, // U+0062 (b)
    { 0x00, 0x00, 0x1E, 0x33, 0x03, 0x33, 0x1E, 0x00 }, // U+0063 (c)
    { 0x38, 0x30, 0x30, 0x3e, 0x33, 0x33, 0x6E, 0x00 }, // U+0064 (d)
    { 0x00, 0x00, 0x1E, 0x33, 0x3f, 0x03, 0x1E, 0x00 }, // U+0065 (e)
    { 0x1C, 0x36, 0x06, 0x0f, 0x06, 0x06, 0x0F, 0x00 }, // U+0066 (f)
    { 0x00, 0x00, 0x6E, 0x33, 0x33, 0x3E, 0x30, 0x1F }, // U+0067 (g)
    { 0x07, 0x06, 0x36, 0x6E, 0x66, 0x66, 0x67, 0x00 }, // U+0068 (h)
    { 0x0C, 0x00, 0x0E, 0x0C, 0x0C, 0x0C, 0x1E, 0x00 }, // U+0069 (i)
    { 0x30, 0x00, 0x30, 0x30, 0x30, 0x33, 0x33, 0x1E }, // U+006A (j)
    { 0x07, 0x06, 0x66, 0x36, 0x1E, 0x36, 0x67, 0x00 }, // U+006B (k)
    { 0x0E, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x1E, 0x00 }, // U+006C (l)
    { 0x00, 0x00, 0x33, 0x7F, 0x7F, 0x6B, 0x63, 0x00 }, // U+006D (m)
    { 0x00, 0x00, 0x1F, 0x33, 0x33, 0x33, 0x33, 0x00 }, // U+006E (n)
    { 0x00, 0x00, 0x1E, 0x33, 0x33, 0x33, 0x1E, 0x00 }, // U+006F (o)
    { 0x00, 0x00, 0x3B, 0x66, 0x66, 0x3E, 0x06, 0x0F }, // U+0070 (p)
    { 0x00, 0x00, 0x6E, 0x33, 0x33, 0x3E, 0x30, 0x78 }, // U+0071 (q)
    { 0x00, 0x00, 0x3B, 0x6E, 0x66, 0x06, 0x0F, 0x00 }, // U+0072 (r)
    { 0x00, 0x00, 0x3E, 0x03, 0x1E, 0x30, 0x1F, 0x00 }, // U+0073 (s)
    { 0x08, 0x0C, 0x3E, 0x0C, 0x0C, 0x2C, 0x18, 0x00 }, // U+0074 (t)
    { 0x00, 0x00, 0x33, 0x33, 0x33, 0x33, 0x6E, 0x00 }, // U+0075 (u)
    { 0x00, 0x00, 0x33, 0x33, 0x33, 0x1E, 0x0C, 0x00 }, // U+0076 (v)
    { 0x00, 0x00, 0x63, 0x6B, 0x7F, 0x7F, 0x36, 0x00 }, // U+0077 (w)
    { 0x00, 0x00, 0x63, 0x36, 0x1C, 0x36, 0x63, 0x00 }, // U+0078 (x)
    { 0x00, 0x00, 0x33, 0x33, 0x33, 0x3E, 0x30, 0x1F }, // U+0079 (y)
    { 0x00, 0x00, 0x3F, 0x19, 0x0C, 0x26, 0x3F, 0x00 }, // U+007A (z)
    { 0x38, 0x0C, 0x0C, 0x07, 0x0C, 0x0C, 0x38, 0x00 }, // U+007B ({)
    { 0x18, 0x18, 0x18, 0x00, 0x18, 0x18, 0x18, 0x00 }, // U+007C (|)
    { 0x07, 0x0C, 0x0C, 0x38, 0x0C, 0x0C, 0x07, 0x00 }, // U+007D (})
    { 0x6E, 0x3B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // U+007E (~)
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }  // U+007F
};

// FIXME: This assumes 32 bit BGR (Blue-Green-Red) palette
enum BGRColor : u32 {
    Black = 0,
    Blue = 0x0000FF,
    Green = 0x00FF00,
    Cyan = 0x0000FFFF,
    Red = 0xFF0000,
    Magenta = 0x00FF00FF,
    Brown = 0x00964B00,
    LightGray = 0x00D3D3D3,
    DarkGray = 0x00A9A9A9,
    BrightBlue = 0x0ADD8E6,
    BrightGreen = 0x0090EE90,
    BrightCyan = 0x00E0FFFF,
    BrightRed = 0x00D70A53,
    BrightMagenta = 0x00F984E5,
    Yellow = 0x00FFE135,
    White = 0x00FFFFFF,
};

static inline BGRColor convert_standard_color_to_bgr_color(Console::Color color)
{
    switch (color) {
    case Console::Color::Black:
        return BGRColor::Black;
    case Console::Color::Red:
        return BGRColor::Red;
    case Console::Color::Brown:
        return BGRColor::Brown;
    case Console::Color::Blue:
        return BGRColor::Blue;
    case Console::Color::Magenta:
        return BGRColor::Magenta;
    case Console::Color::Green:
        return BGRColor::Green;
    case Console::Color::Cyan:
        return BGRColor::Cyan;
    case Console::Color::LightGray:
        return BGRColor::LightGray;
    case Console::Color::DarkGray:
        return BGRColor::DarkGray;
    case Console::Color::BrightRed:
        return BGRColor::BrightRed;
    case Console::Color::BrightGreen:
        return BGRColor::BrightGreen;
    case Console::Color::Yellow:
        return BGRColor::Yellow;
    case Console::Color::BrightBlue:
        return BGRColor::BrightBlue;
    case Console::Color::BrightMagenta:
        return BGRColor::BrightMagenta;
    case Console::Color::BrightCyan:
        return BGRColor::BrightCyan;
    case Console::Color::White:
        return BGRColor::White;
    default:
        VERIFY_NOT_REACHED();
    }
}

NonnullRefPtr<FramebufferConsole> FramebufferConsole::initialize(PhysicalAddress framebuffer_address, size_t width, size_t height, size_t pitch)
{
    return adopt_ref(*new FramebufferConsole(framebuffer_address, width, height, pitch));
}

void FramebufferConsole::set_resolution(size_t width, size_t height, size_t pitch)
{
    m_width = width;
    m_height = height;
    m_pitch = pitch;

    dbgln("Framebuffer Console: taking {} bytes", page_round_up(pitch * height));
    m_framebuffer_region = MM.allocate_kernel_region(m_framebuffer_address, page_round_up(pitch * height), "Framebuffer Console", Region::Access::Read | Region::Access::Write, Region::Cacheable::Yes);
    VERIFY(m_framebuffer_region);

    // Just to start cleanly, we clean the entire framebuffer
    memset(m_framebuffer_region->vaddr().as_ptr(), 0, pitch * height);

    ConsoleManagement::the().resolution_was_changed();
}

FramebufferConsole::FramebufferConsole(PhysicalAddress framebuffer_address, size_t width, size_t height, size_t pitch)
    : Console(width, height)
    , m_framebuffer_address(framebuffer_address)
    , m_pitch(pitch)
{
    set_resolution(width, height, pitch);
}

size_t FramebufferConsole::bytes_per_base_glyph() const
{
    // FIXME: We assume we have 32 bit bpp framebuffer.
    return 8 * 32;
}
size_t FramebufferConsole::chars_per_line() const
{
    return width() / bytes_per_base_glyph();
}

void FramebufferConsole::set_cursor(size_t, size_t)
{
}
void FramebufferConsole::hide_cursor()
{
}
void FramebufferConsole::show_cursor()
{
}

void FramebufferConsole::clear(size_t x, size_t y, size_t length) const
{
    ScopedSpinLock lock(m_lock);
    if (x == 0 && length == max_column()) {
        // if we need to clear the entire row, just clean it with quick memset :)
        auto* offset_in_framebuffer = (u32*)m_framebuffer_region->vaddr().offset(x * sizeof(u32) * 8).offset(y * 8 * sizeof(u32) * width()).as_ptr();
        for (size_t current_x = 0; current_x < 8; current_x++) {
            memset(offset_in_framebuffer, 0, width() * sizeof(u32));
            offset_in_framebuffer = (u32*)((u8*)offset_in_framebuffer + width() * 4);
        }
        return;
    }
    for (size_t index = 0; index < length; index++) {
        if (x >= max_column()) {
            x = 0;
            y++;
            if (y >= max_row())
                y = 0;
        }
        clear_glyph(x, y);
    }
}

void FramebufferConsole::clear_glyph(size_t x, size_t y) const
{
    VERIFY(m_lock.is_locked());
    auto* offset_in_framebuffer = (u32*)m_framebuffer_region->vaddr().offset(x * sizeof(u32) * 8).offset(y * 8 * sizeof(u32) * width()).as_ptr();
    for (size_t current_x = 0; current_x < 8; current_x++) {
        memset(offset_in_framebuffer, 0, 8 * sizeof(u32));
        offset_in_framebuffer = (u32*)((u8*)offset_in_framebuffer + width() * sizeof(u32));
    }
}

void FramebufferConsole::enable()
{
    ScopedSpinLock lock(m_lock);
    memset(m_framebuffer_region->vaddr().as_ptr(), 0, height() * width() * sizeof(u32));
    m_enabled.store(true);
}
void FramebufferConsole::disable()
{
    ScopedSpinLock lock(m_lock);
    m_enabled.store(false);
}

void FramebufferConsole::write(size_t x, size_t y, char ch, Color background, Color foreground, bool critical) const
{
    ScopedSpinLock lock(m_lock);
    if (!m_enabled.load())
        return;

    // If we are in critical printing mode, we need to handle new lines here
    // because there's no other responsible object to do that in the print call path
    if (critical && (ch == '\r' || ch == '\n')) {
        m_x = 0;
        m_y += 1;
        if (m_y >= max_row())
            m_y = 0;
        return;
    }
    if ((int)ch < 0x20 || (int)ch == 0x7f) {
        // FIXME: There's no point in printing empty glyphs...
        // Maybe try to add these special glyphs and print them.
        return;
    }
    clear_glyph(x, y);
    auto* offset_in_framebuffer = (u32*)m_framebuffer_region->vaddr().offset(x * sizeof(u32) * 8).offset(y * 8 * sizeof(u32) * width()).as_ptr();
    int current_bitpixels = 0;
    int current_bitpixel = 0;
    auto bitmap = font8x8_basic[(int)ch];
    bool set;
    BGRColor foreground_color = convert_standard_color_to_bgr_color(foreground);
    BGRColor background_color = convert_standard_color_to_bgr_color(background);
    for (current_bitpixels = 0; current_bitpixels < 8; current_bitpixels++) {
        for (current_bitpixel = 0; current_bitpixel < 8; current_bitpixel++) {
            set = bitmap[current_bitpixels] & (1 << current_bitpixel);
            if (set) {
                offset_in_framebuffer[current_bitpixel] = foreground_color;
            } else {
                offset_in_framebuffer[current_bitpixel] = background_color;
            }
        }
        offset_in_framebuffer = (u32*)((u8*)offset_in_framebuffer + width() * 4);
    }
    m_x = x + 1;
    if (m_x >= max_column()) {
        m_x = 0;
        m_y = y + 1;
        if (m_y >= max_row())
            m_y = 0;
    }
}

void FramebufferConsole::write(size_t x, size_t y, char ch, bool critical) const
{
    write(x, y, ch, m_default_background_color, m_default_foreground_color, critical);
}

void FramebufferConsole::write(char ch, bool critical) const
{
    write(m_x, m_y, ch, m_default_background_color, m_default_foreground_color, critical);
}

}
