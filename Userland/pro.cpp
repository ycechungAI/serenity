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

#include <AK/URL.h>
#include <AK/SharedBuffer.h>
#include <LibCore/EventLoop.h>
#include <LibProtocol/Client.h>
#include <LibProtocol/Download.h>
#include <stdio.h>

int main(int argc, char** argv)
{
    if (argc != 2) {
        printf("usage: %s <url>\n", argv[0]);
        return 0;
    }

    String url_string = argv[1];
    URL url(url_string);
    if (!url.is_valid()) {
        fprintf(stderr, "'%s' is not a valid URL\n", url_string.characters());
        return 1;
    }

    Core::EventLoop loop;
    auto protocol_client = Protocol::Client::construct();

    auto download = protocol_client->start_download(url.to_string());
    if (!download) {
        fprintf(stderr, "Failed to start download for '%s'\n", url_string.characters());
        return 1;
    }
    download->on_progress = [](u32 total_size, u32 downloaded_size) {
        dbgprintf("download progress: %u / %u\n", downloaded_size, total_size);
    };
    download->on_finish = [&](bool success, auto& payload, auto) {
        if (success)
            write(STDOUT_FILENO, payload.data(), payload.size());
        else
            fprintf(stderr, "Download failed :(\n");
        loop.quit(0);
    };
    dbgprintf("started download with id %d\n", download->id());

    return loop.exec();
}
