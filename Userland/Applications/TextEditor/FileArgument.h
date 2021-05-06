/*
 * Copyright (c) 2021, ry755 <ryanst755@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Optional.h>
#include <AK/String.h>

class FileArgument final {
public:
    FileArgument(String);
    ~FileArgument();

    String filename() { return m_filename; }
    Optional<size_t> line() { return m_line; }
    Optional<size_t> column() { return m_column; }

private:
    String m_filename;
    Optional<size_t> m_line;
    Optional<size_t> m_column;
};
