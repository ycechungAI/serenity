/*
 * Copyright (c) 2018-2020, Andreas Kling <kling@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

//
// Parallel ATA (PATA) controller driver
//
// This driver describes a logical PATA Channel. Each channel can connect up to 2
// IDE Hard Disk Drives. The drives themselves can be either the master drive (hd0)
// or the slave drive (hd1).
//
// More information about the ATA spec for PATA can be found here:
//      ftp://ftp.seagate.com/acrobat/reference/111-1c.pdf
//

#pragma once

#include <AK/Error.h>
#include <Kernel/Arch/x86/IO.h>
#include <Kernel/Devices/Device.h>
#include <Kernel/Interrupts/IRQHandler.h>
#include <Kernel/Library/LockRefPtr.h>
#include <Kernel/Locking/Mutex.h>
#include <Kernel/Memory/PhysicalPage.h>
#include <Kernel/PhysicalAddress.h>
#include <Kernel/Random.h>
#include <Kernel/Storage/ATA/ATADevice.h>
#include <Kernel/Storage/ATA/ATAPort.h>
#include <Kernel/Storage/StorageDevice.h>
#include <Kernel/WaitQueue.h>

namespace Kernel {

class AsyncBlockDeviceRequest;

class IDEController;
class PCIIDEController;
#if ARCH(I386) || ARCH(X86_64)
class ISAIDEController;
#endif
class IDEChannel
    : public ATAPort
    , public IRQHandler {
    friend class IDEController;

public:
    enum class ChannelType : u8 {
        Primary,
        Secondary
    };

    enum class DeviceType : u8 {
        Master,
        Slave,
    };

    struct IOAddressGroup {
        IOAddressGroup(IOAddress io_base, IOAddress control_base, IOAddress bus_master_base)
            : m_io_base(io_base)
            , m_control_base(control_base)
            , m_bus_master_base(bus_master_base)
        {
        }

        IOAddressGroup(IOAddress io_base, IOAddress control_base, Optional<IOAddress> bus_master_base)
            : m_io_base(io_base)
            , m_control_base(control_base)
            , m_bus_master_base(bus_master_base)
        {
        }

        IOAddressGroup(IOAddress io_base, IOAddress control_base)
            : m_io_base(io_base)
            , m_control_base(control_base)
            , m_bus_master_base()
        {
        }

        IOAddressGroup(IOAddressGroup const& other, IOAddress bus_master_base)
            : m_io_base(other.io_base())
            , m_control_base(other.control_base())
            , m_bus_master_base(bus_master_base)
        {
        }

        IOAddressGroup(IOAddressGroup const&) = default;

        // Disable default implementations that would use surprising integer promotion.
        bool operator==(IOAddressGroup const&) const = delete;
        bool operator<=(IOAddressGroup const&) const = delete;
        bool operator>=(IOAddressGroup const&) const = delete;
        bool operator<(IOAddressGroup const&) const = delete;
        bool operator>(IOAddressGroup const&) const = delete;

        IOAddress io_base() const { return m_io_base; };
        IOAddress control_base() const { return m_control_base; }
        Optional<IOAddress> bus_master_base() const { return m_bus_master_base; }

    private:
        IOAddress m_io_base;
        IOAddress m_control_base;
        Optional<IOAddress> m_bus_master_base;
    };

public:
    static NonnullLockRefPtr<IDEChannel> create(IDEController const&, IOAddressGroup, ChannelType type);
    static NonnullLockRefPtr<IDEChannel> create(IDEController const&, u8 irq, IOAddressGroup, ChannelType type);

    virtual ~IDEChannel() override;

    virtual StringView purpose() const override { return "PATA Channel"sv; }

    ErrorOr<void> allocate_resources_for_pci_ide_controller(Badge<PCIIDEController>, bool force_pio);
#if ARCH(I386) || ARCH(X86_64)
    ErrorOr<void> allocate_resources_for_isa_ide_controller(Badge<ISAIDEController>);
#endif

private:
    static constexpr size_t m_logical_sector_size = 512;
    ErrorOr<void> allocate_resources(bool force_pio);
    StringView channel_type_string() const;

    virtual ErrorOr<void> disable() override { TODO(); }
    virtual ErrorOr<void> power_on() override { TODO(); }

    virtual ErrorOr<void> port_phy_reset() override;
    bool select_device_and_wait_until_not_busy(DeviceType, size_t milliseconds_timeout);

    virtual bool pio_capable() const override { return true; }
    virtual bool dma_capable() const override { return m_dma_enabled; }

    virtual size_t max_possible_devices_connected() const override { return 2; }

    virtual ErrorOr<void> stop_busmastering() override;
    virtual ErrorOr<void> start_busmastering(TransactionDirection) override;
    virtual ErrorOr<void> force_busmastering_status_clean() override;
    virtual ErrorOr<u8> busmastering_status() override;
    virtual ErrorOr<void> prepare_transaction_with_busmastering(TransactionDirection, PhysicalAddress prdt_buffer) override;
    virtual ErrorOr<void> initiate_transaction(TransactionDirection) override;

    virtual ErrorOr<u8> task_file_status() override;
    virtual ErrorOr<u8> task_file_error() override;

    virtual ErrorOr<void> wait_if_busy_until_timeout(size_t timeout_in_milliseconds) override;

    virtual ErrorOr<void> device_select(size_t device_index) override;
    virtual ErrorOr<bool> detect_presence_on_selected_device() override;

    virtual ErrorOr<void> enable_interrupts() override;
    virtual ErrorOr<void> disable_interrupts() override;

    virtual ErrorOr<void> force_clear_interrupts() override;
    virtual ErrorOr<void> load_taskfile_into_registers(TaskFile const&, LBAMode lba_mode, size_t completion_timeout_in_milliseconds) override;

    virtual ErrorOr<void> read_pio_data_to_buffer(UserOrKernelBuffer&, size_t block_offset, size_t words_count) override;
    virtual ErrorOr<void> write_pio_data_from_buffer(UserOrKernelBuffer const&, size_t block_offset, size_t words_count) override;

    IDEChannel(IDEController const&, IOAddressGroup, ChannelType type, NonnullOwnPtr<KBuffer> ata_identify_data_buffer);
    IDEChannel(IDEController const&, u8 irq, IOAddressGroup, ChannelType type, NonnullOwnPtr<KBuffer> ata_identify_data_buffer);
    //^ IRQHandler
    virtual bool handle_irq(RegisterState const&) override;

    // Data members
    ChannelType m_channel_type { ChannelType::Primary };

    bool m_dma_enabled { false };
    bool m_interrupts_enabled { true };

    IOAddressGroup m_io_group;
};
}
