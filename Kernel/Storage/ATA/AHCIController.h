/*
 * Copyright (c) 2021-2022, Liav A. <liavalb@hotmail.co.il>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/OwnPtr.h>
#include <AK/RefPtr.h>
#include <AK/Types.h>
#include <Kernel/Sections.h>
#include <Kernel/Storage/ATA/AHCI.h>
#include <Kernel/Storage/ATA/ATAController.h>
#include <Kernel/Storage/StorageDevice.h>

namespace Kernel {

class AsyncBlockDeviceRequest;
class AHCIPortHandler;
class AHCIPort;
class AHCIController final : public ATAController
    , public PCI::Device {
    friend class AHCIPortHandler;

public:
    static NonnullRefPtr<AHCIController> initialize(PCI::DeviceIdentifier const& pci_device_identifier);
    virtual ~AHCIController() override;

    virtual RefPtr<StorageDevice> device(u32 index) const override;
    virtual bool reset() override;
    virtual bool shutdown() override;
    virtual size_t devices_count() const override;
    virtual void start_request(ATADevice const&, AsyncBlockDeviceRequest&) override;
    virtual void complete_current_request(AsyncDeviceRequest::RequestResult) override;

    PhysicalAddress get_identify_metadata_physical_region(Badge<AHCIPort>, u32 port_index) const;
    void handle_interrupt_for_port(Badge<AHCIPortHandler>, u32 port_index) const;

private:
    void disable_global_interrupts() const;
    void enable_global_interrupts() const;

    explicit AHCIController(PCI::DeviceIdentifier const&);
    void initialize_hba(PCI::DeviceIdentifier const&);

    AHCI::HBADefinedCapabilities capabilities() const;
    RefPtr<StorageDevice> device_by_port(u32 index) const;

    volatile AHCI::PortRegisters& port(size_t port_number) const;
    NonnullOwnPtr<Memory::Region> default_hba_region() const;
    volatile AHCI::HBA& hba() const;

    NonnullRefPtrVector<Memory::PhysicalPage> m_identify_metadata_pages;
    Array<RefPtr<AHCIPort>, 32> m_ports;
    NonnullOwnPtr<Memory::Region> m_hba_region;
    AHCI::HBADefinedCapabilities m_hba_capabilities;

    // FIXME: There could be multiple IRQ (MSI) handlers for AHCI. Find a way to use all of them.
    OwnPtr<AHCIPortHandler> m_irq_handler;
};
}
