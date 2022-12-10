/*
 * Copyright (c) 2020-2022, the SerenityOS developers.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/DeprecatedString.h>

namespace AK {

DeprecatedString human_readable_size(u64 size);
DeprecatedString human_readable_size_long(u64 size);
DeprecatedString human_readable_time(i64 time_in_seconds);
DeprecatedString human_readable_digital_time(i64 time_in_seconds);

}

#if USING_AK_GLOBALLY
using AK::human_readable_digital_time;
using AK::human_readable_size;
using AK::human_readable_size_long;
using AK::human_readable_time;
#endif
