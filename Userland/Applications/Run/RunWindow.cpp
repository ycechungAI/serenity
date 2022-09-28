/*
 * Copyright (c) 2021, Nick Vella <nick@nxk.io>
 * Copyright (c) 2022, the SerenityOS developers.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "RunWindow.h"
#include <AK/LexicalPath.h>
#include <AK/URL.h>
#include <Applications/Run/RunGML.h>
#include <LibCore/File.h>
#include <LibCore/StandardPaths.h>
#include <LibCore/Stream.h>
#include <LibDesktop/Launcher.h>
#include <LibGUI/Button.h>
#include <LibGUI/Event.h>
#include <LibGUI/FilePicker.h>
#include <LibGUI/Icon.h>
#include <LibGUI/ImageWidget.h>
#include <LibGUI/MessageBox.h>
#include <LibGUI/Widget.h>
#include <spawn.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

RunWindow::RunWindow()
    : m_path_history()
    , m_path_history_model(GUI::ItemListModel<String>::create(m_path_history))
{
    // FIXME: Handle failure to load history somehow.
    (void)load_history();

    auto app_icon = GUI::Icon::default_icon("app-run"sv);

    set_title("Run");
    set_icon(app_icon.bitmap_for_size(16));
    resize(345, 100);
    set_resizable(false);
    set_minimizable(false);

    auto& main_widget = set_main_widget<GUI::Widget>();
    main_widget.load_from_gml(run_gml);

    m_icon_image_widget = *main_widget.find_descendant_of_type_named<GUI::ImageWidget>("icon");
    m_icon_image_widget->set_bitmap(app_icon.bitmap_for_size(32));

    m_path_combo_box = *main_widget.find_descendant_of_type_named<GUI::ComboBox>("path");
    m_path_combo_box->set_model(m_path_history_model);
    if (!m_path_history.is_empty())
        m_path_combo_box->set_selected_index(0);

    m_ok_button = *main_widget.find_descendant_of_type_named<GUI::DialogButton>("ok_button");
    m_ok_button->on_click = [this](auto) {
        do_run();
    };
    m_ok_button->set_default(true);

    m_cancel_button = *main_widget.find_descendant_of_type_named<GUI::DialogButton>("cancel_button");
    m_cancel_button->on_click = [this](auto) {
        close();
    };

    m_browse_button = *find_descendant_of_type_named<GUI::DialogButton>("browse_button");
    m_browse_button->on_click = [this](auto) {
        Optional<String> path = GUI::FilePicker::get_open_filepath(this, {}, Core::StandardPaths::home_directory(), false, GUI::Dialog::ScreenPosition::Center);
        if (path.has_value())
            m_path_combo_box->set_text(path.value().view());
    };
}

void RunWindow::event(Core::Event& event)
{
    if (event.type() == GUI::Event::KeyDown) {
        auto& key_event = static_cast<GUI::KeyEvent&>(event);
        if (key_event.key() == Key_Escape) {
            // Escape key pressed, close dialog
            close();
            return;
        } else if ((key_event.key() == Key_Up || key_event.key() == Key_Down) && m_path_history.is_empty()) {
            return;
        }
    }

    Window::event(event);
}

void RunWindow::do_run()
{
    auto run_input = m_path_combo_box->text().trim_whitespace();

    hide();

    if (run_via_launch(run_input) || run_as_command(run_input)) {
        // Remove any existing history entry, prepend the successful run string to history and save.
        m_path_history.remove_all_matching([&](String v) { return v == run_input; });
        m_path_history.prepend(run_input);
        // FIXME: Handle failure to save history somehow.
        (void)save_history();

        close();
        return;
    }

    GUI::MessageBox::show_error(this, "Failed to run. Please check your command, path, or address, and try again."sv);

    show();
}

bool RunWindow::run_as_command(String const& run_input)
{
    pid_t child_pid;
    char const* shell_executable = "/bin/Shell"; // TODO query and use the user's preferred shell.
    char const* argv[] = { shell_executable, "-c", run_input.characters(), nullptr };

    if ((errno = posix_spawn(&child_pid, shell_executable, nullptr, nullptr, const_cast<char**>(argv), environ))) {
        perror("posix_spawn");
        return false;
    }

    // Command spawned in child shell. Hide and wait for exit code.
    int status;
    if (waitpid(child_pid, &status, 0) < 0)
        return false;

    int child_error = WEXITSTATUS(status);
    dbgln("Child shell exited with code {}", child_error);

    // 127 is typically the shell indicating command not found. 126 for all other errors.
    if (child_error == 126 || child_error == 127) {
        return false;
    }

    dbgln("Ran via command shell.");

    return true;
}

bool RunWindow::run_via_launch(String const& run_input)
{
    auto url = URL::create_with_url_or_path(run_input);

    if (url.scheme() == "file") {
        auto real_path = Core::File::real_path_for(url.path());
        if (real_path.is_null()) {
            // errno *should* be preserved from Core::File::real_path_for().
            warnln("Failed to launch '{}': {}", url.path(), strerror(errno));
            return false;
        }
        url = URL::create_with_url_or_path(real_path);
    }

    if (!Desktop::Launcher::open(url)) {
        warnln("Failed to launch '{}'", url);
        return false;
    }

    dbgln("Ran via URL launch.");

    return true;
}

String RunWindow::history_file_path()
{
    return LexicalPath::canonicalized_path(String::formatted("{}/{}", Core::StandardPaths::config_directory(), "RunHistory.txt"));
}

ErrorOr<void> RunWindow::load_history()
{
    m_path_history.clear();
    auto file = TRY(Core::Stream::File::open(history_file_path(), Core::Stream::OpenMode::Read));
    auto buffered_file = TRY(Core::Stream::BufferedFile::create(move(file)));
    Array<u8, PAGE_SIZE> line_buffer;

    while (!buffered_file->is_eof()) {
        StringView line = TRY(buffered_file->read_line(line_buffer));
        if (!line.is_empty() && !line.is_whitespace())
            m_path_history.append(line);
    }
    return {};
}

ErrorOr<void> RunWindow::save_history()
{
    auto file = TRY(Core::Stream::File::open(history_file_path(), Core::Stream::OpenMode::Write));

    // Write the first 25 items of history
    for (int i = 0; i < min(static_cast<int>(m_path_history.size()), 25); i++)
        TRY(file->write(String::formatted("{}\n", m_path_history[i]).bytes()));

    return {};
}
