/*
 * Copyright (c) 2018-2020, Andreas Kling <kling@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/RefPtr.h>
#include <AK/WeakPtr.h>
#include <Kernel/Devices/BlockDevice.h>
#include <Kernel/Storage/Partition/DiskPartitionMetadata.h>

namespace Kernel {

class DiskPartition final : public BlockDevice {
public:
    static NonnullRefPtr<DiskPartition> create(BlockDevice&, unsigned, DiskPartitionMetadata);
    virtual ~DiskPartition();

    virtual void start_request(AsyncBlockDeviceRequest&) override;

    // ^BlockDevice
    virtual KResultOr<size_t> read(OpenFileDescription&, u64, UserOrKernelBuffer&, size_t) override;
    virtual bool can_read(const OpenFileDescription&, size_t) const override;
    virtual KResultOr<size_t> write(OpenFileDescription&, u64, const UserOrKernelBuffer&, size_t) override;
    virtual bool can_write(const OpenFileDescription&, size_t) const override;

    const DiskPartitionMetadata& metadata() const;

    // FIXME: We expose this constructor to make try_create_device helper to work
    DiskPartition(BlockDevice&, unsigned, DiskPartitionMetadata);

private:
    virtual StringView class_name() const override;

    WeakPtr<BlockDevice> m_device;
    DiskPartitionMetadata m_metadata;
};

}
