/*
 * Copyright (c) 2021, Nico Weber <thakis@chromium.org>
 * Copyright (c) 2021, Marcin Undak <mcinek@gmail.com>
 * Copyright (c) 2021, Jesse Buhagiar <jooster669@gmail.com>
 * Copyright (c) 2022, the SerenityOS developers.
 * Copyright (c) 2022, Filiph Sandström <filiph.sandstrom@filfatstudios.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/Format.h>
#include <AK/Types.h>

#include <Kernel/Arch/InterruptManagement.h>
#include <Kernel/Arch/Interrupts.h>
#include <Kernel/Arch/Processor.h>
#include <Kernel/Arch/aarch64/BootPPMParser.h>
#include <Kernel/Arch/aarch64/CPU.h>
#include <Kernel/Arch/aarch64/RPi/Framebuffer.h>
#include <Kernel/Arch/aarch64/RPi/Mailbox.h>
#include <Kernel/Arch/aarch64/RPi/Timer.h>
#include <Kernel/Arch/aarch64/RPi/UART.h>
#include <Kernel/Arch/aarch64/Registers.h>
#include <Kernel/Arch/aarch64/TrapFrame.h>
#include <Kernel/CommandLine.h>
#include <Kernel/Devices/DeviceManagement.h>
#include <Kernel/Graphics/Console/BootFramebufferConsole.h>
#include <Kernel/KSyms.h>
#include <Kernel/Panic.h>

extern "C" void exception_common(Kernel::TrapFrame const* const trap_frame);
extern "C" void exception_common(Kernel::TrapFrame const* const trap_frame)
{
    constexpr bool print_stack_frame = true;

    if constexpr (print_stack_frame) {
        dbgln("Exception Generated by processor!");

        for (auto reg = 0; reg < 31; reg++) {
            dbgln("x{}: {:x}", reg, trap_frame->x[reg]);
        }

        // Special registers
        dbgln("spsr_el1: {:x}", trap_frame->spsr_el1);
        dbgln("elr_el1: {:x}", trap_frame->elr_el1);
        dbgln("tpidr_el1: {:x}", trap_frame->tpidr_el1);
        dbgln("sp_el0: {:x}", trap_frame->sp_el0);

        auto esr_el1 = Kernel::Aarch64::ESR_EL1::read();
        dbgln("esr_el1: EC({:#b}) IL({:#b}) ISS({:#b}) ISS2({:#b})", esr_el1.EC, esr_el1.IL, esr_el1.ISS, esr_el1.ISS2);
    }

    Kernel::Processor::halt();
}

typedef void (*ctor_func_t)();
extern ctor_func_t start_heap_ctors[];
extern ctor_func_t end_heap_ctors[];
extern ctor_func_t start_ctors[];
extern ctor_func_t end_ctors[];

// FIXME: Share this with the Intel Prekernel.
extern size_t __stack_chk_guard;
size_t __stack_chk_guard;
extern "C" [[noreturn]] void __stack_chk_fail();

void __stack_chk_fail()
{
    Kernel::Processor::halt();
}

READONLY_AFTER_INIT bool g_in_early_boot;

namespace Kernel {

static void draw_logo();
static u32 query_firmware_version();

extern "C" [[noreturn]] void halt();
extern "C" [[noreturn]] void init();

ALWAYS_INLINE static Processor& bootstrap_processor()
{
    alignas(Processor) static u8 bootstrap_processor_storage[sizeof(Processor)];
    return (Processor&)bootstrap_processor_storage;
}

Atomic<Graphics::Console*> g_boot_console;

extern "C" [[noreturn]] void init()
{
    g_in_early_boot = true;

    dbgln("Welcome to Serenity OS!");
    dbgln("Imagine this being your ideal operating system.");
    dbgln("Observed deviations from that ideal are shortcomings of your imagination.");
    dbgln();

    CommandLine::early_initialize("");

    new (&bootstrap_processor()) Processor();
    bootstrap_processor().initialize(0);

    // We call the constructors of kmalloc.cpp separately, because other constructors in the Kernel
    // might rely on being able to call new/kmalloc in the constructor. We do have to run the
    // kmalloc constructors, because kmalloc_init relies on that.
    for (ctor_func_t* ctor = start_heap_ctors; ctor < end_heap_ctors; ctor++)
        (*ctor)();
    kmalloc_init();

    load_kernel_symbol_table();

    CommandLine::initialize();

    auto& framebuffer = RPi::Framebuffer::the();
    if (framebuffer.initialized()) {
        g_boot_console = &try_make_lock_ref_counted<Graphics::BootFramebufferConsole>(framebuffer.gpu_buffer(), framebuffer.width(), framebuffer.width(), framebuffer.pitch()).value().leak_ref();
        draw_logo();
    }
    dmesgln("Starting SerenityOS...");

    DeviceManagement::initialize();

    // Invoke all static global constructors in the kernel.
    // Note that we want to do this as early as possible.
    for (ctor_func_t* ctor = start_ctors; ctor < end_ctors; ctor++)
        (*ctor)();

    initialize_interrupts();
    InterruptManagement::initialize();
    Processor::enable_interrupts();

    auto firmware_version = query_firmware_version();
    dmesgln("Firmware version: {}", firmware_version);

    dmesgln("Initialize MMU");
    init_page_tables();

    auto& timer = RPi::Timer::the();
    timer.set_interrupt_interval_usec(1'000'000);
    timer.enable_interrupt_mode();

    dmesgln("Enter loop");

    // This will not disable interrupts, so the timer will still fire and show that
    // interrupts are working!
    for (;;)
        asm volatile("wfi");
}

class QueryFirmwareVersionMboxMessage : RPi::Mailbox::Message {
public:
    u32 version;

    QueryFirmwareVersionMboxMessage()
        : RPi::Mailbox::Message(0x0000'0001, 4)
    {
        version = 0;
    }
};

static u32 query_firmware_version()
{
    struct __attribute__((aligned(16))) {
        RPi::Mailbox::MessageHeader header;
        QueryFirmwareVersionMboxMessage query_firmware_version;
        RPi::Mailbox::MessageTail tail;
    } message_queue;

    if (!RPi::Mailbox::the().send_queue(&message_queue, sizeof(message_queue))) {
        return 0xffff'ffff;
    }

    return message_queue.query_firmware_version.version;
}

extern "C" const u32 serenity_boot_logo_start;
extern "C" const u32 serenity_boot_logo_size;

static void draw_logo()
{
    BootPPMParser logo_parser(reinterpret_cast<u8 const*>(&serenity_boot_logo_start), serenity_boot_logo_size);
    if (!logo_parser.parse()) {
        dbgln("Failed to parse boot logo.");
        return;
    }

    dbgln("Boot logo size: {} ({} x {})", serenity_boot_logo_size, logo_parser.image.width, logo_parser.image.height);

    auto& framebuffer = RPi::Framebuffer::the();
    auto fb_ptr = framebuffer.gpu_buffer();
    auto image_left = (framebuffer.width() - logo_parser.image.width) / 2;
    auto image_right = image_left + logo_parser.image.width;
    auto image_top = (framebuffer.height() - logo_parser.image.height) / 2;
    auto image_bottom = image_top + logo_parser.image.height;
    auto logo_pixels = logo_parser.image.pixel_data;

    for (u32 y = 0; y < framebuffer.height(); y++) {
        for (u32 x = 0; x < framebuffer.width(); x++) {
            if (x >= image_left && x < image_right && y >= image_top && y < image_bottom) {
                switch (framebuffer.pixel_order()) {
                case RPi::Framebuffer::PixelOrder::RGB:
                    fb_ptr[0] = logo_pixels[0];
                    fb_ptr[1] = logo_pixels[1];
                    fb_ptr[2] = logo_pixels[2];
                    break;
                case RPi::Framebuffer::PixelOrder::BGR:
                    fb_ptr[0] = logo_pixels[2];
                    fb_ptr[1] = logo_pixels[1];
                    fb_ptr[2] = logo_pixels[0];
                    break;
                default:
                    dbgln("Unsupported pixel format");
                    VERIFY_NOT_REACHED();
                }

                logo_pixels += 3;
            } else {
                fb_ptr[0] = 0xBD;
                fb_ptr[1] = 0xBD;
                fb_ptr[2] = 0xBD;
            }

            fb_ptr[3] = 0xFF;
            fb_ptr += 4;
        }
        fb_ptr += framebuffer.pitch() - framebuffer.width() * 4;
    }
}

}
