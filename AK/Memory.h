/*
 * Copyright (c) 2020, Andreas Kling <kling@serenityos.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include <AK/Types.h>

#if defined(KERNEL) || defined(BOOTSTRAPPER)
#    include <LibBareMetal/StdLib.h>
#else
#    include <stdlib.h>
#    include <string.h>
#endif

#if defined(__serenity__) && !defined(KERNEL) && !defined(BOOTSTRAPPER)
extern "C" void* mmx_memcpy(void* to, const void* from, size_t);
#endif

ALWAYS_INLINE void fast_u32_copy(u32* dest, const u32* src, size_t count)
{
#if defined(__serenity__) && !defined(KERNEL) && !defined(BOOTSTRAPPER)
    if (count >= 256) {
        mmx_memcpy(dest, src, count * sizeof(count));
        return;
    }
#endif
    asm volatile(
        "rep movsl\n"
        : "=S"(src), "=D"(dest), "=c"(count)
        : "S"(src), "D"(dest), "c"(count)
        : "memory");
}

ALWAYS_INLINE void fast_u32_fill(u32* dest, u32 value, size_t count)
{
    asm volatile(
        "rep stosl\n"
        : "=D"(dest), "=c"(count)
        : "D"(dest), "c"(count), "a"(value)
        : "memory");
}
