/*
 * Copyright (c) 2024, the SerenityOS developers.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <LibGfx/ImageFormats/JPEGLoader.h>
#include <LibGfx/ImageFormats/JPEGWriter.h>
#include <LibMain/Main.h>

struct Fixpoint {
    Gfx::Color fixpoint;
    int number_of_iterations {};
};

static ErrorOr<Fixpoint> compute_fixpoint(Gfx::Color start_color)
{
    auto bitmap = TRY(Gfx::Bitmap::create(Gfx::BitmapFormat::BGRx8888, { 8, 8 }));
    bitmap->fill(start_color);

    int number_of_iterations = 1;
    Color last_color = start_color;
    while (true) {
        AllocatingMemoryStream stream;
        TRY(Gfx::JPEGWriter::encode(stream, *bitmap));
        auto data = TRY(stream.read_until_eof());
        auto plugin_decoder = TRY(Gfx::JPEGImageDecoderPlugin::create(data));
        auto frame = TRY(plugin_decoder->frame(0));

        Color current_color = frame.image->get_pixel(4, 4);
        if (current_color == last_color)
            break;

        ++number_of_iterations;
        last_color = current_color;
        bitmap = *frame.image;
    }
    return Fixpoint { last_color, number_of_iterations };
}

static ErrorOr<void> test(Gfx::Color color)
{
    auto fixpoint = TRY(compute_fixpoint(color));
    outln("color {} converges to {} after saving {} times", color, fixpoint.fixpoint, fixpoint.number_of_iterations);
    return {};
}

ErrorOr<int> serenity_main(Main::Arguments)
{
    TRY(test(Gfx::Color::Red));
    TRY(test(Gfx::Color::Green));
    TRY(test(Gfx::Color::Blue));

    TRY(test(Gfx::Color::MidRed));
    TRY(test(Gfx::Color::MidGreen));
    TRY(test(Gfx::Color::MidBlue));

    TRY(test(Gfx::Color::Cyan));
    TRY(test(Gfx::Color::Magenta));
    TRY(test(Gfx::Color::Yellow));

    TRY(test(Gfx::Color::Black));
    TRY(test(Gfx::Color::White));

    return 0;
}
