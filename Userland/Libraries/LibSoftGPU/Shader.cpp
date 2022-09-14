/*
 * Copyright (c) 2022, Stephan Unverwerth <s.unverwerth@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <LibSoftGPU/Shader.h>

namespace SoftGPU {

Shader::Shader(void const* ownership_token)
    : GPU::Shader(ownership_token)
{
}

}
