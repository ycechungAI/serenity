/*
 * Copyright (c) 2022, Luke Wilde <lukew@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/Debug.h>
#include <LibGL/GLContext.h>
#include <LibWeb/HTML/HTMLCanvasElement.h>
#include <LibWeb/WebGL/WebGLRenderingContextBase.h>

namespace Web::WebGL {

WebGLRenderingContextBase::WebGLRenderingContextBase(HTML::HTMLCanvasElement& canvas_element, NonnullOwnPtr<GL::GLContext> context, WebGLContextAttributes context_creation_parameters, WebGLContextAttributes actual_context_parameters)
    : m_canvas_element(canvas_element)
    , m_context(move(context))
    , m_context_creation_parameters(move(context_creation_parameters))
    , m_actual_context_parameters(move(actual_context_parameters))
{
}

WebGLRenderingContextBase::~WebGLRenderingContextBase() = default;

void WebGLRenderingContextBase::present()
{
    if (!m_should_present)
        return;

    m_should_present = false;

    // "Before the drawing buffer is presented for compositing the implementation shall ensure that all rendering operations have been flushed to the drawing buffer."
    // FIXME: Is this the operation it means?
    m_context->gl_flush();

    m_context->present();

    // "By default, after compositing the contents of the drawing buffer shall be cleared to their default values, as shown in the table above.
    // This default behavior can be changed by setting the preserveDrawingBuffer attribute of the WebGLContextAttributes object.
    // If this flag is true, the contents of the drawing buffer shall be preserved until the author either clears or overwrites them."
    if (!m_context_creation_parameters.preserve_drawing_buffer) {
        auto current_clear_color = m_context->current_clear_color();
        auto current_clear_depth = m_context->current_clear_depth();
        auto current_clear_stencil = m_context->current_clear_stencil();

        // The implicit clear value for the color buffer is (0, 0, 0, 0)
        m_context->gl_clear_color(0, 0, 0, 0);

        // The implicit clear value for the depth buffer is 1.0.
        m_context->gl_clear_depth(1.0);

        // The implicit clear value for the stencil buffer is 0.
        m_context->gl_clear_stencil(0);

        m_context->gl_clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        // Restore the clear values.
        m_context->gl_clear_color(current_clear_color[0], current_clear_color[1], current_clear_color[2], current_clear_color[3]);
        m_context->gl_clear_depth(current_clear_depth);
        m_context->gl_clear_stencil(current_clear_stencil);
    }
}

void WebGLRenderingContextBase::needs_to_present()
{
    m_should_present = true;

    if (!m_canvas_element)
        return;
    if (!m_canvas_element->layout_node())
        return;
    m_canvas_element->layout_node()->set_needs_display();
}

Optional<Vector<String>> WebGLRenderingContextBase::get_supported_extensions() const
{
    if (m_context_lost)
        return Optional<Vector<String>> {};

    dbgln_if(WEBGL_CONTEXT_DEBUG, "WebGLRenderingContextBase::get_supported_extensions()");

    // FIXME: We don't currently support any extensions.
    return Vector<String> {};
}

JS::Object* WebGLRenderingContextBase::get_extension(String const& name) const
{
    if (m_context_lost)
        return nullptr;

    dbgln_if(WEBGL_CONTEXT_DEBUG, "WebGLRenderingContextBase::get_extension(name='{}')", name);

    // FIXME: We don't currently support any extensions.
    return nullptr;
}

void WebGLRenderingContextBase::active_texture(GLenum texture)
{
    if (m_context_lost)
        return;

    dbgln_if(WEBGL_CONTEXT_DEBUG, "WebGLRenderingContextBase::active_texture(texture=0x{:08x})", texture);
    m_context->gl_active_texture(texture);
}

void WebGLRenderingContextBase::clear(GLbitfield mask)
{
    if (m_context_lost)
        return;

    dbgln_if(WEBGL_CONTEXT_DEBUG, "WebGLRenderingContextBase::clear(mask=0x{:08x})", mask);
    m_context->gl_clear(mask);

    // FIXME: This should only be done if this is targeting the front buffer.
    needs_to_present();
}

void WebGLRenderingContextBase::clear_color(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
    if (m_context_lost)
        return;

    dbgln_if(WEBGL_CONTEXT_DEBUG, "WebGLRenderingContextBase::clear_color(red={}, green={}, blue={}, alpha={})", red, green, blue, alpha);
    m_context->gl_clear_color(red, green, blue, alpha);
}

void WebGLRenderingContextBase::clear_depth(GLclampf depth)
{
    if (m_context_lost)
        return;

    dbgln_if(WEBGL_CONTEXT_DEBUG, "WebGLRenderingContextBase::clear_depth(depth={})", depth);
    m_context->gl_clear_depth(depth);
}

void WebGLRenderingContextBase::clear_stencil(GLint s)
{
    if (m_context_lost)
        return;

    dbgln_if(WEBGL_CONTEXT_DEBUG, "WebGLRenderingContextBase::clear_stencil(s=0x{:08x})", s);
    m_context->gl_clear_stencil(s);
}

void WebGLRenderingContextBase::color_mask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)
{
    if (m_context_lost)
        return;

    dbgln_if(WEBGL_CONTEXT_DEBUG, "WebGLRenderingContextBase::color_mask(red={}, green={}, blue={}, alpha={})", red, green, blue, alpha);
    m_context->gl_color_mask(red, green, blue, alpha);
}

void WebGLRenderingContextBase::cull_face(GLenum mode)
{
    if (m_context_lost)
        return;

    dbgln_if(WEBGL_CONTEXT_DEBUG, "WebGLRenderingContextBase::cull_face(mode=0x{:08x})", mode);
    m_context->gl_cull_face(mode);
}

void WebGLRenderingContextBase::depth_func(GLenum func)
{
    if (m_context_lost)
        return;

    dbgln_if(WEBGL_CONTEXT_DEBUG, "WebGLRenderingContextBase::depth_func(func=0x{:08x})", func);
    m_context->gl_depth_func(func);
}

void WebGLRenderingContextBase::depth_mask(GLboolean mask)
{
    if (m_context_lost)
        return;

    dbgln_if(WEBGL_CONTEXT_DEBUG, "WebGLRenderingContextBase::depth_mask(mask={})", mask);
    m_context->gl_depth_mask(mask);
}

void WebGLRenderingContextBase::finish()
{
    if (m_context_lost)
        return;

    dbgln_if(WEBGL_CONTEXT_DEBUG, "WebGLRenderingContextBase::finish()");
    m_context->gl_finish();
}

void WebGLRenderingContextBase::flush()
{
    if (m_context_lost)
        return;

    dbgln_if(WEBGL_CONTEXT_DEBUG, "WebGLRenderingContextBase::flush()");
    m_context->gl_flush();
}

void WebGLRenderingContextBase::front_face(GLenum mode)
{
    if (m_context_lost)
        return;

    dbgln_if(WEBGL_CONTEXT_DEBUG, "WebGLRenderingContextBase::front_face(mode=0x{:08x})", mode);
    m_context->gl_front_face(mode);
}

void WebGLRenderingContextBase::polygon_offset(GLfloat factor, GLfloat units)
{
    if (m_context_lost)
        return;

    dbgln_if(WEBGL_CONTEXT_DEBUG, "WebGLRenderingContextBase::polygon_offset(factor={}, units={})", factor, units);
    m_context->gl_polygon_offset(factor, units);
}

void WebGLRenderingContextBase::scissor(GLint x, GLint y, GLsizei width, GLsizei height)
{
    if (m_context_lost)
        return;

    dbgln_if(WEBGL_CONTEXT_DEBUG, "WebGLRenderingContextBase::scissor(x={}, y={}, width={}, height={})", x, y, width, height);
    m_context->gl_scissor(x, y, width, height);
}

void WebGLRenderingContextBase::stencil_op(GLenum fail, GLenum zfail, GLenum zpass)
{
    if (m_context_lost)
        return;

    dbgln_if(WEBGL_CONTEXT_DEBUG, "WebGLRenderingContextBase::stencil_op(fail=0x{:08x}, zfail=0x{:08x}, zpass=0x{:08x})", fail, zfail, zpass);
    m_context->gl_stencil_op_separate(GL_FRONT_AND_BACK, fail, zfail, zpass);
}

void WebGLRenderingContextBase::stencil_op_separate(GLenum face, GLenum fail, GLenum zfail, GLenum zpass)
{
    if (m_context_lost)
        return;

    dbgln_if(WEBGL_CONTEXT_DEBUG, "WebGLRenderingContextBase::stencil_op_separate(face=0x{:08x}, fail=0x{:08x}, zfail=0x{:08x}, zpass=0x{:08x})", face, fail, zfail, zpass);
    m_context->gl_stencil_op_separate(face, fail, zfail, zpass);
}

void WebGLRenderingContextBase::viewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
    if (m_context_lost)
        return;

    dbgln_if(WEBGL_CONTEXT_DEBUG, "WebGLRenderingContextBase::viewport(x={}, y={}, width={}, height={})", x, y, width, height);
    m_context->gl_viewport(x, y, width, height);
}

}
