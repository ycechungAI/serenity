/*
 * Copyright (c) 2022, Stephan Unverwerth <s.unverwerth@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/Debug.h>
#include <LibGL/GLContext.h>

namespace GL {

GLuint GLContext::gl_create_shader(GLenum shader_type)
{
    // FIXME: Add support for GL_COMPUTE_SHADER, GL_TESS_CONTROL_SHADER, GL_TESS_EVALUATION_SHADER and GL_GEOMETRY_SHADER.
    RETURN_VALUE_WITH_ERROR_IF(shader_type != GL_VERTEX_SHADER
            && shader_type != GL_FRAGMENT_SHADER,
        GL_INVALID_ENUM,
        0);

    GLuint shader_name;
    m_shader_name_allocator.allocate(1, &shader_name);
    auto shader = Shader::create(shader_type);
    m_allocated_shaders.set(shader_name, shader);
    return shader_name;
}

void GLContext::gl_delete_shader(GLuint shader)
{
    // "A value of 0 for shader will be silently ignored." (https://registry.khronos.org/OpenGL-Refpages/gl4/html/glDeleteShader.xhtml)
    if (shader == 0)
        return;

    auto it = m_allocated_shaders.find(shader);
    RETURN_WITH_ERROR_IF(it == m_allocated_shaders.end(), GL_INVALID_VALUE);

    // FIXME: According to the spec, we should only flag the shader for deletion here and delete it once it is detached from all programs.
    m_allocated_shaders.remove(it);
    m_shader_name_allocator.free(shader);
}

void GLContext::gl_shader_source(GLuint shader, GLsizei count, GLchar const** string, GLint const* length)
{
    dbgln("gl_shader_source({}, {}, {#x}, {#x}) unimplemented ", shader, count, string, length);
    TODO();
}

void GLContext::gl_compile_shader(GLuint shader)
{
    dbgln("gl_compile_shader({}) unimplemented ", shader);
    TODO();
}

GLuint GLContext::gl_create_program()
{
    dbgln("gl_create_program() unimplemented ");
    TODO();
    return 0;
}

void GLContext::gl_delete_program(GLuint program)
{
    dbgln("gl_delete_program({}) unimplemented ", program);
    TODO();
}

void GLContext::gl_attach_shader(GLuint program, GLuint shader)
{
    dbgln("gl_attach_shader({}, {}) unimplemented ", program, shader);
    TODO();
}

void GLContext::gl_link_program(GLuint program)
{
    dbgln("gl_link_program({}) unimplemented ", program);
    TODO();
}

}
