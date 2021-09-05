/*
 * Copyright (c) 2018-2020, Andreas Kling <kling@serenityos.org>
 * Copyright (c) 2021, sin-ack <sin-ack@protonmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <Kernel/API/InodeWatcherFlags.h>
#include <Kernel/FileSystem/Custody.h>
#include <Kernel/FileSystem/FileDescription.h>
#include <Kernel/FileSystem/InodeWatcher.h>
#include <Kernel/Process.h>

namespace Kernel {

KResultOr<FlatPtr> Process::sys$create_inode_watcher(u32 flags)
{
    VERIFY_PROCESS_BIG_LOCK_ACQUIRED(this)
    REQUIRE_PROMISE(rpath);

    auto fd_or_error = m_fds.allocate();
    if (fd_or_error.is_error())
        return fd_or_error.error();
    auto inode_watcher_fd = fd_or_error.release_value();

    auto watcher_or_error = InodeWatcher::try_create();
    if (watcher_or_error.is_error())
        return watcher_or_error.error();

    auto description_or_error = FileDescription::try_create(*watcher_or_error.value());
    if (description_or_error.is_error())
        return description_or_error.error();

    m_fds[inode_watcher_fd.fd].set(description_or_error.release_value());
    m_fds[inode_watcher_fd.fd].description()->set_readable(true);

    if (flags & static_cast<unsigned>(InodeWatcherFlags::Nonblock))
        m_fds[inode_watcher_fd.fd].description()->set_blocking(false);
    if (flags & static_cast<unsigned>(InodeWatcherFlags::CloseOnExec))
        m_fds[inode_watcher_fd.fd].set_flags(m_fds[inode_watcher_fd.fd].flags() | FD_CLOEXEC);

    return inode_watcher_fd.fd;
}

KResultOr<FlatPtr> Process::sys$inode_watcher_add_watch(Userspace<const Syscall::SC_inode_watcher_add_watch_params*> user_params)
{
    VERIFY_PROCESS_BIG_LOCK_ACQUIRED(this)
    REQUIRE_PROMISE(rpath);

    Syscall::SC_inode_watcher_add_watch_params params;
    TRY(copy_from_user(&params, user_params));

    auto description = fds().file_description(params.fd);
    if (!description)
        return EBADF;
    if (!description->is_inode_watcher())
        return EBADF;
    auto inode_watcher = description->inode_watcher();

    auto path = get_syscall_path_argument(params.user_path);
    if (path.is_error())
        return path.error();

    auto custody_or_error = VirtualFileSystem::the().resolve_path(path.value()->view(), current_directory());
    if (custody_or_error.is_error())
        return custody_or_error.error();

    auto& custody = custody_or_error.value();
    if (!custody->inode().fs().supports_watchers())
        return ENOTSUP;

    auto wd_or_error = inode_watcher->register_inode(custody->inode(), params.event_mask);
    if (wd_or_error.is_error())
        return wd_or_error.error();
    return wd_or_error.value();
}

KResultOr<FlatPtr> Process::sys$inode_watcher_remove_watch(int fd, int wd)
{
    VERIFY_PROCESS_BIG_LOCK_ACQUIRED(this)
    auto description = fds().file_description(fd);
    if (!description)
        return EBADF;
    if (!description->is_inode_watcher())
        return EBADF;
    auto inode_watcher = description->inode_watcher();

    auto result = inode_watcher->unregister_by_wd(wd);
    if (result.is_error())
        return result;

    return 0;
}

}
