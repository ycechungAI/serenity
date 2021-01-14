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

#pragma once

#include <AK/FileStream.h>
#include <AK/HashMap.h>
#include <AK/NonnullOwnPtr.h>
#include <AK/Optional.h>
#include <AK/RefCounted.h>
#include <AK/URL.h>
#include <ProtocolServer/Forward.h>

namespace ProtocolServer {

class Download {
public:
    virtual ~Download();

    i32 id() const { return m_id; }
    URL url() const { return m_url; }

    Optional<u32> status_code() const { return m_status_code; }
    Optional<u32> total_size() const { return m_total_size; }
    size_t downloaded_size() const { return m_downloaded_size; }
    const HashMap<String, String, CaseInsensitiveStringTraits>& response_headers() const { return m_response_headers; }

    void stop();
    virtual void set_certificate(String, String);

    // FIXME: Want Badge<Protocol>, but can't make one from HttpProtocol, etc.
    void set_download_fd(int fd) { m_download_fd = fd; }
    int download_fd() const { return m_download_fd; }

    void did_finish(bool success);
    void did_progress(Optional<u32> total_size, u32 downloaded_size);
    void set_status_code(u32 status_code) { m_status_code = status_code; }
    void did_request_certificates();
    void set_response_headers(const HashMap<String, String, CaseInsensitiveStringTraits>&);
    void set_downloaded_size(size_t size) { m_downloaded_size = size; }
    const OutputFileStream& output_stream() const { return *m_output_stream; }

protected:
    explicit Download(ClientConnection&, NonnullOwnPtr<OutputFileStream>&&);

private:
    ClientConnection& m_client;
    i32 m_id { 0 };
    int m_download_fd { -1 }; // Passed to client.
    URL m_url;
    Optional<u32> m_status_code;
    Optional<u32> m_total_size {};
    size_t m_downloaded_size { 0 };
    NonnullOwnPtr<OutputFileStream> m_output_stream;
    HashMap<String, String, CaseInsensitiveStringTraits> m_response_headers;
};

}
