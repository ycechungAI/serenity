/*
 * Copyright (c) 2020-2022, the SerenityOS developers.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/NumberFormat.h>

namespace AK {

// FIXME: Remove this hackery once printf() supports floats.
static DeprecatedString number_string_with_one_decimal(u64 number, u64 unit, char const* suffix)
{
    int decimal = (number % unit) * 10 / unit;
    return DeprecatedString::formatted("{}.{} {}", number / unit, decimal, suffix);
}

DeprecatedString human_readable_size(u64 size)
{
    if (size < 1 * KiB)
        return DeprecatedString::formatted("{} B", size);
    if (size < 1 * MiB)
        return number_string_with_one_decimal(size, KiB, "KiB");
    if (size < 1 * GiB)
        return number_string_with_one_decimal(size, MiB, "MiB");
    if (size < 1 * TiB)
        return number_string_with_one_decimal(size, GiB, "GiB");
    if (size < 1 * PiB)
        return number_string_with_one_decimal(size, TiB, "TiB");
    if (size < 1 * EiB)
        return number_string_with_one_decimal(size, PiB, "PiB");
    return number_string_with_one_decimal(size, EiB, "EiB");
}

DeprecatedString human_readable_size_long(u64 size)
{
    if (size < 1 * KiB)
        return DeprecatedString::formatted("{} bytes", size);
    else
        return DeprecatedString::formatted("{} ({} bytes)", human_readable_size(size), size);
}

DeprecatedString human_readable_time(i64 time_in_seconds)
{
    auto days = time_in_seconds / 86400;
    time_in_seconds = time_in_seconds % 86400;

    auto hours = time_in_seconds / 3600;
    time_in_seconds = time_in_seconds % 3600;

    auto minutes = time_in_seconds / 60;
    time_in_seconds = time_in_seconds % 60;

    StringBuilder builder;

    if (days > 0)
        builder.appendff("{} day{} ", days, days == 1 ? "" : "s");

    if (hours > 0)
        builder.appendff("{} hour{} ", hours, hours == 1 ? "" : "s");

    if (minutes > 0)
        builder.appendff("{} minute{} ", minutes, minutes == 1 ? "" : "s");

    builder.appendff("{} second{}", time_in_seconds, time_in_seconds == 1 ? "" : "s");

    return builder.to_deprecated_string();
}

DeprecatedString human_readable_digital_time(i64 time_in_seconds)
{
    auto hours = time_in_seconds / 3600;
    time_in_seconds = time_in_seconds % 3600;

    auto minutes = time_in_seconds / 60;
    time_in_seconds = time_in_seconds % 60;

    StringBuilder builder;

    if (hours > 0)
        builder.appendff("{:02}:", hours);
    builder.appendff("{:02}:", minutes);
    builder.appendff("{:02}", time_in_seconds);

    return builder.to_deprecated_string();
}

}
