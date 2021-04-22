/*
 * Copyright (c) 2020, the SerenityOS developers.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Format.h>

extern bool g_report_to_debug;

template<typename... Ts>
void reportln(const StringView& format, Ts... args)
{
    if (g_report_to_debug)
        AK::vdbgln(format, AK::VariadicFormatParams { args... });
    else
        warnln(format, args...);
}
