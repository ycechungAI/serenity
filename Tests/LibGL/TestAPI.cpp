/*
 * Copyright (c) 2022, Luke Wilde <lukew@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <LibGL/GL/gl.h>
#include <LibGL/GLContext.h>
#include <LibTest/TestCase.h>

static NonnullOwnPtr<GL::GLContext> create_testing_context()
{
    auto bitmap = MUST(Gfx::Bitmap::try_create(Gfx::BitmapFormat::BGRx8888, { 1, 1 }));
    auto context = GL::create_context(*bitmap);
    GL::make_context_current(context);
    return context;
}

TEST_CASE(0001_gl_gen_textures_does_not_return_the_same_texture_name_twice_unless_deleted)
{
    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glGenTextures.xhtml
    // "Texture names returned by a call to glGenTextures are not returned by subsequent calls, unless they are first deleted with glDeleteTextures."
    auto context = create_testing_context();

    GLuint texture1 = 0;
    GLuint texture2 = 0;

    glGenTextures(1, &texture1);

    // glDeleteTextures previously did not check that the texture name was allocated by glGenTextures before adding it to the free texture name list.
    // This means that if you delete a texture twice in a row, the name will appear twice in the free texture list, making glGenTextures return the
    // same texture name twice in a row.
    glDeleteTextures(1, &texture1);
    glDeleteTextures(1, &texture1);

    texture1 = 0;

    glGenTextures(1, &texture1);
    glGenTextures(1, &texture2);

    EXPECT_NE(texture1, texture2);
}
