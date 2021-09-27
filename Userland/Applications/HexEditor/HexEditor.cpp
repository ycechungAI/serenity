/*
 * Copyright (c) 2018-2020, Andreas Kling <kling@serenityos.org>
 * Copyright (c) 2021, Mustafa Quraish <mustafa@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "HexEditor.h"
#include "SearchResultsModel.h"
#include <AK/Debug.h>
#include <AK/ScopeGuard.h>
#include <AK/StringBuilder.h>
#include <LibGUI/Action.h>
#include <LibGUI/Clipboard.h>
#include <LibGUI/Menu.h>
#include <LibGUI/MessageBox.h>
#include <LibGUI/Painter.h>
#include <LibGUI/Scrollbar.h>
#include <LibGUI/TextEditor.h>
#include <LibGUI/Window.h>
#include <LibGfx/FontDatabase.h>
#include <LibGfx/Palette.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

HexEditor::HexEditor()
    : m_blink_timer(Core::Timer::construct())
{
    set_should_hide_unnecessary_scrollbars(true);
    set_focus_policy(GUI::FocusPolicy::StrongFocus);
    set_scrollbars_enabled(true);
    set_font(Gfx::FontDatabase::default_fixed_width_font());
    set_background_role(ColorRole::Base);
    set_foreground_role(ColorRole::BaseText);
    vertical_scrollbar().set_step(line_height());

    m_blink_timer->set_interval(500);
    m_blink_timer->on_timeout = [this]() {
        m_cursor_blink_active = !m_cursor_blink_active;
        update();
    };
    m_blink_timer->start();
}

HexEditor::~HexEditor()
{
}

void HexEditor::set_readonly(bool readonly)
{
    if (m_readonly == readonly)
        return;
    m_readonly = readonly;
}

void HexEditor::set_buffer(const ByteBuffer& buffer)
{
    m_buffer = buffer;
    set_content_length(buffer.size());
    m_tracked_changes.clear();
    m_position = 0;
    m_cursor_at_low_nibble = false;
    update();
    update_status();
}

void HexEditor::fill_selection(u8 fill_byte)
{
    if (!has_selection())
        return;

    for (size_t i = m_selection_start; i < m_selection_end; i++) {
        m_tracked_changes.set(i, m_buffer.data()[i]);
        m_buffer.data()[i] = fill_byte;
    }

    update();
    did_change();
}

void HexEditor::set_position(size_t position)
{
    if (position > m_buffer.size())
        return;

    m_position = position;
    m_cursor_at_low_nibble = false;
    reset_cursor_blink_state();
    scroll_position_into_view(position);
    update_status();
}

bool HexEditor::write_to_file(const String& path)
{
    if (m_buffer.is_empty())
        return true;

    int fd = open(path.characters(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd < 0) {
        perror("open");
        return false;
    }

    return write_to_file(fd);
}

bool HexEditor::write_to_file(int fd)
{
    ScopeGuard fd_guard = [fd] { close(fd); };

    int rc = ftruncate(fd, m_buffer.size());
    if (rc < 0) {
        perror("ftruncate");
        return false;
    }

    ssize_t nwritten = write(fd, m_buffer.data(), m_buffer.size());
    if (nwritten < 0) {
        perror("write");
        close(fd);
        return false;
    }

    if (static_cast<size_t>(nwritten) == m_buffer.size()) {
        m_tracked_changes.clear();
        update();
    }

    return true;
}

size_t HexEditor::selection_size()
{
    if (!has_selection())
        return 0;
    return m_selection_end - m_selection_start;
}

bool HexEditor::copy_selected_hex_to_clipboard()
{
    if (!has_selection())
        return false;

    StringBuilder output_string_builder;
    for (size_t i = m_selection_start; i < m_selection_end; i++)
        output_string_builder.appendff("{:02X} ", m_buffer.data()[i]);

    GUI::Clipboard::the().set_plain_text(output_string_builder.to_string());
    return true;
}

bool HexEditor::copy_selected_text_to_clipboard()
{
    if (!has_selection())
        return false;

    StringBuilder output_string_builder;
    for (size_t i = m_selection_start; i < m_selection_end; i++)
        output_string_builder.append(isprint(m_buffer.data()[i]) ? m_buffer[i] : '.');

    GUI::Clipboard::the().set_plain_text(output_string_builder.to_string());
    return true;
}

bool HexEditor::copy_selected_hex_to_clipboard_as_c_code()
{
    if (!has_selection())
        return false;

    StringBuilder output_string_builder;
    output_string_builder.appendff("unsigned char raw_data[{}] = {{\n", (m_selection_end - m_selection_start) + 1);
    output_string_builder.append("    ");
    for (size_t i = m_selection_start, j = 1; i < m_selection_end; i++, j++) {
        output_string_builder.appendff("{:#02X}", m_buffer.data()[i]);
        if (i != m_selection_end)
            output_string_builder.append(", ");
        if ((j % 12) == 0) {
            output_string_builder.append("\n");
            output_string_builder.append("    ");
        }
    }
    output_string_builder.append("\n};\n");

    GUI::Clipboard::the().set_plain_text(output_string_builder.to_string());
    return true;
}

void HexEditor::set_bytes_per_row(size_t bytes_per_row)
{
    m_bytes_per_row = bytes_per_row;
    auto newWidth = offset_margin_width() + (m_bytes_per_row * (character_width() * 3)) + 10 + (m_bytes_per_row * character_width()) + 20;
    auto newHeight = total_rows() * line_height() + 10;
    set_content_size({ static_cast<int>(newWidth), static_cast<int>(newHeight) });
    update();
}

void HexEditor::set_content_length(size_t length)
{
    if (length == m_content_length)
        return;
    m_content_length = length;
    auto newWidth = offset_margin_width() + (m_bytes_per_row * (character_width() * 3)) + 10 + (m_bytes_per_row * character_width()) + 20;
    auto newHeight = total_rows() * line_height() + 10;
    set_content_size({ static_cast<int>(newWidth), static_cast<int>(newHeight) });
}

void HexEditor::mousedown_event(GUI::MouseEvent& event)
{
    if (event.button() != GUI::MouseButton::Primary) {
        return;
    }

    auto absolute_x = horizontal_scrollbar().value() + event.x();
    auto absolute_y = vertical_scrollbar().value() + event.y();

    auto hex_start_x = frame_thickness() + 90;
    auto hex_start_y = frame_thickness() + 5;
    auto hex_end_x = static_cast<int>(hex_start_x + (bytes_per_row() * (character_width() * 3)));
    auto hex_end_y = static_cast<int>(hex_start_y + 5 + (total_rows() * line_height()));

    auto text_start_x = static_cast<int>(frame_thickness() + 100 + (bytes_per_row() * (character_width() * 3)));
    auto text_start_y = frame_thickness() + 5;
    auto text_end_x = static_cast<int>(text_start_x + (bytes_per_row() * character_width()));
    auto text_end_y = static_cast<int>(text_start_y + 5 + (total_rows() * line_height()));

    if (absolute_x >= hex_start_x && absolute_x <= hex_end_x && absolute_y >= hex_start_y && absolute_y <= hex_end_y) {
        if (absolute_x < hex_start_x || absolute_y < hex_start_y)
            return;

        auto byte_x = (absolute_x - hex_start_x) / (character_width() * 3);
        auto byte_y = (absolute_y - hex_start_y) / line_height();
        auto offset = (byte_y * m_bytes_per_row) + byte_x;

        if (offset >= m_buffer.size())
            return;

        dbgln_if(HEX_DEBUG, "HexEditor::mousedown_event(hex): offset={}", offset);

        m_edit_mode = EditMode::Hex;
        m_cursor_at_low_nibble = false;
        m_position = offset;
        m_in_drag_select = true;
        m_selection_start = offset;
        m_selection_end = offset;
        update();
        update_status();
    }

    if (absolute_x >= text_start_x && absolute_x <= text_end_x && absolute_y >= text_start_y && absolute_y <= text_end_y) {
        if (absolute_x < hex_start_x || absolute_y < hex_start_y)
            return;

        auto byte_x = (absolute_x - text_start_x) / character_width();
        auto byte_y = (absolute_y - text_start_y) / line_height();
        auto offset = (byte_y * m_bytes_per_row) + byte_x;

        if (offset >= m_buffer.size())
            return;

        dbgln_if(HEX_DEBUG, "HexEditor::mousedown_event(text): offset={}", offset);

        m_position = offset;
        m_cursor_at_low_nibble = false;
        m_in_drag_select = true;
        m_selection_start = offset;
        m_selection_end = offset;
        m_edit_mode = EditMode::Text;
        update();
        update_status();
    }
}

void HexEditor::mousemove_event(GUI::MouseEvent& event)
{
    auto absolute_x = horizontal_scrollbar().value() + event.x();
    auto absolute_y = vertical_scrollbar().value() + event.y();

    auto hex_start_x = frame_thickness() + 90;
    auto hex_start_y = frame_thickness() + 5;
    auto hex_end_x = static_cast<int>(hex_start_x + (bytes_per_row() * (character_width() * 3)));
    auto hex_end_y = static_cast<int>(hex_start_y + 5 + (total_rows() * line_height()));

    auto text_start_x = static_cast<int>(frame_thickness() + 100 + (bytes_per_row() * (character_width() * 3)));
    auto text_start_y = frame_thickness() + 5;
    auto text_end_x = static_cast<int>(text_start_x + (bytes_per_row() * character_width()));
    auto text_end_y = static_cast<int>(text_start_y + 5 + (total_rows() * line_height()));

    if ((absolute_x >= hex_start_x && absolute_x <= hex_end_x
            && absolute_y >= hex_start_y && absolute_y <= hex_end_y)
        || (absolute_x >= text_start_x && absolute_x <= text_end_x
            && absolute_y >= text_start_y && absolute_y <= text_end_y)) {
        set_override_cursor(Gfx::StandardCursor::IBeam);
    } else {
        set_override_cursor(Gfx::StandardCursor::None);
    }

    if (m_in_drag_select) {
        if (absolute_x >= hex_start_x && absolute_x <= hex_end_x && absolute_y >= hex_start_y && absolute_y <= hex_end_y) {
            if (absolute_x < hex_start_x || absolute_y < hex_start_y)
                return;

            auto byte_x = (absolute_x - hex_start_x) / (character_width() * 3);
            auto byte_y = (absolute_y - hex_start_y) / line_height();
            auto offset = (byte_y * m_bytes_per_row) + byte_x;

            if (offset > m_buffer.size())
                return;

            m_selection_end = offset;
            m_position = offset;
            scroll_position_into_view(offset);
        }

        if (absolute_x >= text_start_x && absolute_x <= text_end_x && absolute_y >= text_start_y && absolute_y <= text_end_y) {
            if (absolute_x < hex_start_x || absolute_y < hex_start_y)
                return;

            auto byte_x = (absolute_x - text_start_x) / character_width();
            auto byte_y = (absolute_y - text_start_y) / line_height();
            auto offset = (byte_y * m_bytes_per_row) + byte_x;

            if (offset > m_buffer.size())
                return;

            m_selection_end = offset;
            m_position = offset;
            scroll_position_into_view(offset);
        }
        update_status();
        update();
        return;
    }
}

void HexEditor::mouseup_event(GUI::MouseEvent& event)
{
    if (event.button() == GUI::MouseButton::Primary) {
        if (m_in_drag_select) {
            if (m_selection_end < m_selection_start) {
                // lets flip these around
                auto start = m_selection_end;
                m_selection_end = m_selection_start;
                m_selection_start = start;
            }
            m_in_drag_select = false;
        }
        update();
        update_status();
    }
}

void HexEditor::scroll_position_into_view(size_t position)
{
    size_t y = position / bytes_per_row();
    size_t x = position % bytes_per_row();
    Gfx::IntRect rect {
        static_cast<int>(frame_thickness() + offset_margin_width() + (x * (character_width() * 3)) + 10),
        static_cast<int>(frame_thickness() + 5 + (y * line_height())),
        static_cast<int>((character_width() * 3)),
        static_cast<int>(line_height() - m_line_spacing)
    };
    scroll_into_view(rect, true, true);
}

void HexEditor::keydown_event(GUI::KeyEvent& event)
{
    dbgln_if(HEX_DEBUG, "HexEditor::keydown_event key={}", static_cast<u8>(event.key()));

    if (event.key() == KeyCode::Key_Up) {
        if (m_position >= bytes_per_row()) {
            m_position -= bytes_per_row();
            m_selection_start = m_selection_end = m_position;
            m_cursor_at_low_nibble = false;
            reset_cursor_blink_state();
            scroll_position_into_view(m_position);
            update();
            update_status();
        }
        return;
    }

    if (event.key() == KeyCode::Key_Down) {
        if (m_position + bytes_per_row() < m_buffer.size()) {
            m_position += bytes_per_row();
            m_selection_start = m_selection_end = m_position;
            m_cursor_at_low_nibble = false;
            reset_cursor_blink_state();
            scroll_position_into_view(m_position);
            update();
            update_status();
        }
        return;
    }

    if (event.key() == KeyCode::Key_Left) {
        if (m_position >= 1) {
            m_position--;
            m_selection_start = m_selection_end = m_position;
            m_cursor_at_low_nibble = false;
            reset_cursor_blink_state();
            scroll_position_into_view(m_position);
            update();
            update_status();
        }
        return;
    }

    if (event.key() == KeyCode::Key_Right) {
        if (m_position + 1 < m_buffer.size()) {
            m_position++;
            m_selection_start = m_selection_end = m_position;
            m_cursor_at_low_nibble = false;
            reset_cursor_blink_state();
            scroll_position_into_view(m_position);
            update();
            update_status();
        }
        return;
    }

    if (event.key() == KeyCode::Key_Backspace) {
        if (m_position > 0) {
            m_position--;
            m_selection_start = m_selection_end = m_position;
            m_cursor_at_low_nibble = false;
            reset_cursor_blink_state();
            scroll_position_into_view(m_position);
            update();
            update_status();
        }
        return;
    }

    if (!is_readonly() && !event.ctrl() && !event.alt() && !event.text().is_empty()) {
        if (m_edit_mode == EditMode::Hex) {
            hex_mode_keydown_event(event);
        } else {
            text_mode_keydown_event(event);
        }
    }
}

void HexEditor::hex_mode_keydown_event(GUI::KeyEvent& event)
{
    if ((event.key() >= KeyCode::Key_0 && event.key() <= KeyCode::Key_9) || (event.key() >= KeyCode::Key_A && event.key() <= KeyCode::Key_F)) {
        if (m_buffer.is_empty())
            return;
        if (m_position == m_buffer.size())
            return;

        VERIFY(m_position <= m_buffer.size());

        // yes, this is terrible... but it works.
        auto value = (event.key() >= KeyCode::Key_0 && event.key() <= KeyCode::Key_9)
            ? event.key() - KeyCode::Key_0
            : (event.key() - KeyCode::Key_A) + 0xA;

        if (!m_cursor_at_low_nibble) {
            m_tracked_changes.set(m_position, m_buffer.data()[m_position]);
            m_buffer.data()[m_position] = value << 4 | (m_buffer.data()[m_position] & 0xF); // shift new value left 4 bits, OR with existing last 4 bits
            m_cursor_at_low_nibble = true;
        } else {
            m_buffer.data()[m_position] = (m_buffer.data()[m_position] & 0xF0) | value; // save the first 4 bits, OR the new value in the last 4
            if (m_position + 1 < m_buffer.size())
                m_position++;
            m_cursor_at_low_nibble = false;
        }

        reset_cursor_blink_state();
        update();
        update_status();
        did_change();
    }
}

void HexEditor::text_mode_keydown_event(GUI::KeyEvent& event)
{
    if (m_buffer.is_empty())
        return;
    VERIFY(m_position < m_buffer.size());

    if (event.code_point() == 0) // This is a control key
        return;

    m_tracked_changes.set(m_position, m_buffer.data()[m_position]);
    m_buffer.data()[m_position] = event.code_point();
    if (m_position + 1 < m_buffer.size())
        m_position++;
    m_cursor_at_low_nibble = false;

    reset_cursor_blink_state();
    update();
    update_status();
    did_change();
}

void HexEditor::update_status()
{
    if (on_status_change)
        on_status_change(m_position, m_edit_mode, m_selection_start, m_selection_end);
}

void HexEditor::did_change()
{
    if (on_change)
        on_change();
}

void HexEditor::paint_event(GUI::PaintEvent& event)
{
    GUI::Frame::paint_event(event);

    GUI::Painter painter(*this);
    painter.add_clip_rect(widget_inner_rect());
    painter.add_clip_rect(event.rect());
    painter.fill_rect(event.rect(), palette().color(background_role()));

    if (m_buffer.is_empty())
        return;

    painter.translate(frame_thickness(), frame_thickness());
    painter.translate(-horizontal_scrollbar().value(), -vertical_scrollbar().value());

    Gfx::IntRect offset_clip_rect {
        0,
        vertical_scrollbar().value(),
        85,
        height() - height_occupied_by_horizontal_scrollbar() //(total_rows() * line_height()) + 5
    };
    painter.fill_rect(offset_clip_rect, palette().ruler());
    painter.draw_line(offset_clip_rect.top_right(), offset_clip_rect.bottom_right(), palette().ruler_border());

    auto margin_and_hex_width = static_cast<int>(offset_margin_width() + (m_bytes_per_row * (character_width() * 3)) + 15);
    painter.draw_line({ margin_and_hex_width, 0 },
        { margin_and_hex_width, vertical_scrollbar().value() + (height() - height_occupied_by_horizontal_scrollbar()) },
        palette().ruler_border());

    size_t view_height = (height() - height_occupied_by_horizontal_scrollbar());
    size_t min_row = max(0, vertical_scrollbar().value() / line_height());              // if below 0 then use 0
    size_t max_row = min(total_rows(), min_row + ceil_div(view_height, line_height())); // if above calculated rows, use calculated rows

    // paint offsets
    for (size_t i = min_row; i < max_row; i++) {
        Gfx::IntRect side_offset_rect {
            frame_thickness() + 5,
            static_cast<int>(frame_thickness() + 5 + (i * line_height())),
            width() - width_occupied_by_vertical_scrollbar(),
            height() - height_occupied_by_horizontal_scrollbar()
        };

        bool is_current_line = (m_position / bytes_per_row()) == i;
        auto line = String::formatted("{:#08X}", i * bytes_per_row());
        painter.draw_text(
            side_offset_rect,
            line,
            is_current_line ? font().bold_variant() : font(),
            Gfx::TextAlignment::TopLeft,
            is_current_line ? palette().ruler_active_text() : palette().ruler_inactive_text());
    }

    for (size_t i = min_row; i < max_row; i++) {
        for (size_t j = 0; j < bytes_per_row(); j++) {
            auto byte_position = (i * bytes_per_row()) + j;
            if (byte_position >= m_buffer.size())
                return;

            const bool edited_flag = m_tracked_changes.contains(byte_position);

            const bool selection_inbetween_start_end = byte_position >= m_selection_start && byte_position < m_selection_end;
            const bool selection_inbetween_end_start = byte_position >= m_selection_end && byte_position < m_selection_start;
            const bool highlight_flag = selection_inbetween_start_end || selection_inbetween_end_start;

            Gfx::IntRect hex_display_rect {
                frame_thickness() + offset_margin_width() + (static_cast<int>(j) * (character_width() * 3)) + 10,
                frame_thickness() + 5 + (static_cast<int>(i) * line_height()),
                (character_width() * 3),
                line_height() - m_line_spacing
            };

            Gfx::Color background_color = palette().color(background_role());
            Gfx::Color text_color = edited_flag ? Color::Red : palette().color(foreground_role());

            if (highlight_flag) {
                background_color = edited_flag ? palette().selection().inverted() : palette().selection();
                text_color = edited_flag ? palette().selection_text().inverted() : palette().selection_text();
            } else if (byte_position == m_position && m_edit_mode == EditMode::Text) {
                background_color = palette().inactive_selection();
                text_color = palette().inactive_selection_text();
            }
            painter.fill_rect(hex_display_rect, background_color);

            auto line = String::formatted("{:02X}", m_buffer[byte_position]);
            painter.draw_text(hex_display_rect, line, Gfx::TextAlignment::TopLeft, text_color);

            if (m_edit_mode == EditMode::Hex) {
                if (byte_position == m_position && m_cursor_blink_active) {
                    Gfx::IntRect cursor_position_rect {
                        static_cast<int>(hex_display_rect.left() + m_cursor_at_low_nibble * character_width()),
                        hex_display_rect.top(),
                        2,
                        hex_display_rect.height()
                    };
                    painter.fill_rect(cursor_position_rect, palette().text_cursor());
                }
            }

            Gfx::IntRect text_display_rect {
                static_cast<int>(frame_thickness() + offset_margin_width() + (bytes_per_row() * (character_width() * 3)) + (j * character_width()) + 20),
                static_cast<int>(frame_thickness() + 5 + (i * line_height())),
                static_cast<int>(character_width()),
                static_cast<int>(line_height() - m_line_spacing)
            };

            background_color = palette().color(background_role());
            text_color = edited_flag ? Color::Red : palette().color(foreground_role());

            if (highlight_flag) {
                background_color = edited_flag ? palette().selection().inverted() : palette().selection();
                text_color = edited_flag ? palette().selection_text().inverted() : palette().selection_text();
            } else if (byte_position == m_position && m_edit_mode == EditMode::Hex) {
                background_color = palette().inactive_selection();
                text_color = palette().inactive_selection_text();
            }

            painter.fill_rect(text_display_rect, background_color);
            painter.draw_text(text_display_rect, String::formatted("{:c}", isprint(m_buffer[byte_position]) ? m_buffer[byte_position] : '.'), Gfx::TextAlignment::TopLeft, text_color);

            if (m_edit_mode == EditMode::Text) {
                if (byte_position == m_position && m_cursor_blink_active) {
                    Gfx::IntRect cursor_position_rect {
                        text_display_rect.left(), text_display_rect.top(), 2, text_display_rect.height()
                    };
                    painter.fill_rect(cursor_position_rect, palette().text_cursor());
                }
            }
        }
    }
}

void HexEditor::select_all()
{
    highlight(0, m_buffer.size() - 1);
    set_position(0);
}

void HexEditor::highlight(size_t start, size_t end)
{
    m_selection_start = start;
    m_selection_end = end;
    set_position(start);
}

Optional<size_t> HexEditor::find_and_highlight(ByteBuffer& needle, size_t start)
{
    auto end_of_match = find(needle, start);
    if (end_of_match.has_value()) {
        highlight(end_of_match.value() - needle.size(), end_of_match.value() - 1);
    }
    return end_of_match;
}

Optional<size_t> HexEditor::find(ByteBuffer& needle, size_t start)
{
    if (m_buffer.is_empty())
        return {};

    auto raw_offset = memmem(m_buffer.data() + start, m_buffer.size() - start, needle.data(), needle.size());
    if (raw_offset == NULL)
        return {};

    int relative_offset = static_cast<const u8*>(raw_offset) - m_buffer.data();
    dbgln("find: start={} raw_offset={} relative_offset={}", start, raw_offset, relative_offset);

    auto end_of_match = relative_offset + needle.size();

    return end_of_match;
}

Vector<Match> HexEditor::find_all(ByteBuffer& needle, size_t start)
{
    if (m_buffer.is_empty())
        return {};

    Vector<Match> matches;

    size_t i = start;
    while (i < m_buffer.size()) {
        auto raw_offset = memmem(m_buffer.data() + i, m_buffer.size() - i, needle.data(), needle.size());
        if (raw_offset == NULL)
            break;

        int relative_offset = static_cast<const u8*>(raw_offset) - m_buffer.data();
        dbgln("find_all: needle={} start={} raw_offset={} relative_offset={}", needle.data(), i, raw_offset, relative_offset);
        matches.append({ relative_offset, String::formatted("{}", StringView { needle }.to_string().characters()) });
        i = relative_offset + needle.size();
    }

    if (matches.is_empty())
        return {};

    auto first_match = matches.at(0);
    highlight(first_match.offset, first_match.offset + first_match.value.length());

    return matches;
}

Vector<Match> HexEditor::find_all_strings(size_t min_length)
{
    if (m_buffer.is_empty())
        return {};

    Vector<Match> matches;

    int offset = -1;
    StringBuilder builder;
    for (size_t i = 0; i < m_buffer.size(); i++) {
        char c = m_buffer.bytes().at(i);
        if (isprint(c)) {
            if (offset == -1)
                offset = i;
            builder.append(c);
        } else {
            if (builder.length() >= min_length) {
                dbgln("find_all_strings: relative_offset={} string={}", offset, builder.to_string());
                matches.append({ offset, builder.to_string() });
            }
            builder.clear();
            offset = -1;
        }
    }

    if (matches.is_empty())
        return {};

    auto first_match = matches.at(0);
    highlight(first_match.offset, first_match.offset + first_match.value.length());

    return matches;
}

void HexEditor::reset_cursor_blink_state()
{
    m_cursor_blink_active = true;
    m_blink_timer->restart();
}
