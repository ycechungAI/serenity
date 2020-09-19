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

#include <AK/Badge.h>
#include <AK/CircularQueue.h>
#include <AK/WeakPtr.h>
#include <Kernel/FileSystem/File.h>
#include <Kernel/Lock.h>

namespace Kernel {

class Inode;

class InodeWatcher final : public File {
public:
    static NonnullRefPtr<InodeWatcher> create(Inode&);
    virtual ~InodeWatcher() override;

    struct Event {
        enum class Type {
            Invalid = 0,
            Modified,
            ChildAdded,
            ChildRemoved,
        };

        Type type { Type::Invalid };
        unsigned inode_index { 0 };
    };

    virtual bool can_read(const FileDescription&, size_t) const override;
    virtual bool can_write(const FileDescription&, size_t) const override;
    virtual KResultOr<size_t> read(FileDescription&, size_t, UserOrKernelBuffer&, size_t) override;
    virtual KResultOr<size_t> write(FileDescription&, size_t, const UserOrKernelBuffer&, size_t) override;
    virtual String absolute_path(const FileDescription&) const override;
    virtual const char* class_name() const override { return "InodeWatcher"; };

    void notify_inode_event(Badge<Inode>, Event::Type);
    void notify_child_added(Badge<Inode>, const InodeIdentifier& child_id);
    void notify_child_removed(Badge<Inode>, const InodeIdentifier& child_id);

private:
    explicit InodeWatcher(Inode&);

    Lock m_lock;
    WeakPtr<Inode> m_inode;
    CircularQueue<Event, 32> m_queue;
};

}
