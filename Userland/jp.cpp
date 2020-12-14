/*
 * Copyright (c) 2018-2020, Andreas Kling <kling@serenityos.org>
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

#include <AK/ByteBuffer.h>
#include <AK/JsonArray.h>
#include <AK/JsonObject.h>
#include <AK/JsonValue.h>
#include <AK/StringBuilder.h>
#include <LibCore/ArgsParser.h>
#include <LibCore/File.h>
#include <stdio.h>
#include <unistd.h>

static void print(const JsonValue& value, int indent = 0, bool use_color = true);
static void print_indent(int indent)
{
    for (int i = 0; i < indent; ++i)
        out("  ");
}

int main(int argc, char** argv)
{
    if (pledge("stdio rpath", nullptr) < 0) {
        perror("pledge");
        return 1;
    }

    const char* path = nullptr;

    Core::ArgsParser args_parser;
    args_parser.set_general_help("Pretty-print a JSON file with syntax-coloring and indentation.");
    args_parser.add_positional_argument(path, "Path to JSON file", "path");
    args_parser.parse(argc, argv);

    auto file = Core::File::construct(path);
    if (!file->open(Core::IODevice::ReadOnly)) {
        warnln("Couldn't open {} for reading: {}", path, file->error_string());
        return 1;
    }

    if (pledge("stdio", nullptr) < 0) {
        perror("pledge");
        return 1;
    }

    auto file_contents = file->read_all();
    auto json = JsonValue::from_string(file_contents);
    if (!json.has_value()) {
        warnln("Couldn't parse {} as JSON", path);
        return 1;
    }

    print(json.value(), 0, isatty(STDOUT_FILENO));
    outln();

    return 0;
}

void print(const JsonValue& value, int indent, bool use_color)
{
    if (value.is_object()) {
        outln("{{");
        value.as_object().for_each_member([&](auto& member_name, auto& member_value) {
            print_indent(indent + 1);
            if (use_color)
                out("\"\033[33;1m{}\033[0m\": ", member_name);
            else
                out("\"{}\": ", member_name);
            print(member_value, indent + 1, use_color);
            outln(",");
        });
        print_indent(indent);
        out("}}");
        return;
    }
    if (value.is_array()) {
        outln("[");
        value.as_array().for_each([&](auto& entry_value) {
            print_indent(indent + 1);
            print(entry_value, indent + 1, use_color);
            outln(",");
        });
        print_indent(indent);
        out("]");
        return;
    }
    if (use_color) {
        if (value.is_string())
            out("\033[31;1m");
        else if (value.is_number())
            out("\033[35;1m");
        else if (value.is_bool())
            out("\033[32;1m");
        else if (value.is_null())
            out("\033[34;1m");
    }
    if (value.is_string())
        out("\"");
    out("{}", value.to_string());
    if (value.is_string())
        out("\"");
    if (use_color)
        out("\033[0m");
}
