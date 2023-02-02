/*
 * Copyright (c) 2022, Tim Flynn <trflynn89@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#define AK_DONT_REPLACE_STD

#include "../HelperProcess.h"
#include "../Utilities.h"
#include <AK/Platform.h>
#include <LibCore/ArgsParser.h>
#include <LibCore/Directory.h>
#include <LibCore/EventLoop.h>
#include <LibCore/StandardPaths.h>
#include <LibCore/System.h>
#include <LibCore/TCPServer.h>
#include <LibMain/Main.h>
#include <QCoreApplication>
#include <WebDriver/Client.h>

#if defined(AK_OS_MACOS)
#    include <crt_externs.h>
#endif

extern DeprecatedString s_serenity_resource_root;

static char** environment()
{
#if defined(AK_OS_MACOS)
    return *_NSGetEnviron();
#else
    extern char** environ;
    return environ;
#endif
}

static ErrorOr<pid_t> launch_process(StringView application, char const* argv[])
{
    auto paths = TRY(get_paths_for_helper_process(application));

    ErrorOr<pid_t> result = -1;
    for (auto const& path : paths) {
        auto path_view = path.bytes_as_string_view();
        argv[0] = path_view.characters_without_null_termination();
        result = Core::System::posix_spawn(path_view, nullptr, nullptr, const_cast<char**>(argv), environment());
        if (!result.is_error())
            break;
    }
    return result;
}

static ErrorOr<pid_t> launch_browser(DeprecatedString const& socket_path)
{
    char const* argv[] = {
        "ladybird",
        "--webdriver-content-path",
        socket_path.characters(),
        nullptr,
    };

    return launch_process("ladybird"sv, argv);
}

static ErrorOr<pid_t> launch_headless_browser(DeprecatedString const& socket_path)
{
    auto resources = DeprecatedString::formatted("{}/res", s_serenity_resource_root);
    auto error_page = DeprecatedString::formatted("{}/res/html/error.html", s_serenity_resource_root);
    auto certs = DeprecatedString::formatted("{}/etc/ca_certs.ini", s_serenity_resource_root);

    char const* argv[] = {
        "headless-browser",
        "--resources",
        resources.characters(),
        "--error-page",
        error_page.characters(),
        "--certs",
        certs.characters(),
        "--webdriver-ipc-path",
        socket_path.characters(),
        "about:blank",
        nullptr,
    };

    return launch_process("headless-browser"sv, argv);
}

ErrorOr<int> serenity_main(Main::Arguments arguments)
{
    // Note: only creating this to get access to its static methods in HelperProcess
    QCoreApplication application(arguments.argc, arguments.argv);

    auto listen_address = "0.0.0.0"sv;
    int port = 8000;

    Core::ArgsParser args_parser;
    args_parser.add_option(listen_address, "IP address to listen on", "listen-address", 'l', "listen_address");
    args_parser.add_option(port, "Port to listen on", "port", 'p', "port");
    args_parser.parse(arguments);

    auto ipv4_address = IPv4Address::from_string(listen_address);
    if (!ipv4_address.has_value()) {
        warnln("Invalid listen address: {}", listen_address);
        return 1;
    }

    if ((u16)port != port) {
        warnln("Invalid port number: {}", port);
        return 1;
    }

    platform_init();

    auto webdriver_socket_path = DeprecatedString::formatted("{}/webdriver", TRY(Core::StandardPaths::runtime_directory()));
    TRY(Core::Directory::create(webdriver_socket_path, Core::Directory::CreateDirectories::Yes));

    Core::EventLoop loop;
    auto server = TRY(Core::TCPServer::try_create());

    // FIXME: Propagate errors
    server->on_ready_to_accept = [&] {
        auto maybe_client_socket = server->accept();
        if (maybe_client_socket.is_error()) {
            warnln("Failed to accept the client: {}", maybe_client_socket.error());
            return;
        }

        auto maybe_buffered_socket = Core::Stream::BufferedTCPSocket::create(maybe_client_socket.release_value());
        if (maybe_buffered_socket.is_error()) {
            warnln("Could not obtain a buffered socket for the client: {}", maybe_buffered_socket.error());
            return;
        }

        auto maybe_client = WebDriver::Client::try_create(maybe_buffered_socket.release_value(), { launch_browser, launch_headless_browser }, server);
        if (maybe_client.is_error()) {
            warnln("Could not create a WebDriver client: {}", maybe_client.error());
            return;
        }
    };

    TRY(server->listen(ipv4_address.value(), port, Core::TCPServer::AllowAddressReuse::Yes));
    outln("Listening on {}:{}", ipv4_address.value(), port);

    return loop.exec();
}
