/*
 * Copyright (c) 2019-2020, Sergey Bugaev <bugaevc@serenityos.org>
 * Copyright (c) 2022-2023, Liav A. <liavalb@hotmail.co.il>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Kernel/FileSystem/FileSystem.h>
#include <Kernel/FileSystem/Inode.h>
#include <Kernel/Forward.h>

namespace Kernel {

class RAMFS final : public FileSystem {
    friend class RAMFSInode;

public:
    virtual ~RAMFS() override;
    static ErrorOr<NonnullRefPtr<FileSystem>> try_create(ReadonlyBytes);
    virtual ErrorOr<void> initialize() override;

    virtual StringView class_name() const override { return "RAMFS"sv; }

    virtual bool supports_watchers() const override { return true; }

    virtual Inode& root_inode() override;

    virtual u8 internal_file_type_to_directory_entry_type(DirectoryEntryView const& entry) const override;

    enum class FileType : u8 {
        Directory,
        Character,
        Block,
        Regular,
        FIFO,
        Link,
        Socket,
        Unknown,
    };

private:
    RAMFS();

    RefPtr<RAMFSInode> m_root_inode;

    // NOTE: We start by assigning InodeIndex of 2, because 0 is invalid and 1
    // is reserved for the root directory inode.
    unsigned m_next_inode_index { 2 };
    unsigned next_inode_index();
};

}
