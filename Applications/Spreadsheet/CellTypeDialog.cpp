/*
 * Copyright (c) 2020, the SerenityOS developers.
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

#include "CellTypeDialog.h"
#include "Cell.h"
#include "Spreadsheet.h"
#include <AK/StringBuilder.h>
#include <LibGUI/BoxLayout.h>
#include <LibGUI/Button.h>
#include <LibGUI/CheckBox.h>
#include <LibGUI/ComboBox.h>
#include <LibGUI/ItemListModel.h>
#include <LibGUI/Label.h>
#include <LibGUI/ListView.h>
#include <LibGUI/SpinBox.h>
#include <LibGUI/TabWidget.h>
#include <LibGUI/TextEditor.h>
#include <LibGUI/Widget.h>

namespace Spreadsheet {

CellTypeDialog::CellTypeDialog(const Vector<Position>& positions, Sheet& sheet, GUI::Window* parent)
    : GUI::Dialog(parent)
{
    ASSERT(!positions.is_empty());

    StringBuilder builder;

    if (positions.size() == 1)
        builder.appendf("Format Cell %s%zu", positions.first().column.characters(), positions.first().row);
    else
        builder.appendf("Format %zu Cells", positions.size());

    set_title(builder.string_view());
    resize(270, 360);

    auto& main_widget = set_main_widget<GUI::Widget>();
    main_widget.set_layout<GUI::VerticalBoxLayout>().set_margins({ 4, 4, 4, 4 });
    main_widget.set_fill_with_background_color(true);

    auto& tab_widget = main_widget.add<GUI::TabWidget>();
    setup_tabs(tab_widget, positions, sheet);

    auto& buttonbox = main_widget.add<GUI::Widget>();
    buttonbox.set_preferred_size({ 0, 20 });
    buttonbox.set_size_policy(GUI::SizePolicy::Fill, GUI::SizePolicy::Fixed);
    auto& button_layout = buttonbox.set_layout<GUI::HorizontalBoxLayout>();
    button_layout.set_spacing(10);
    button_layout.add_spacer();
    auto& ok_button = buttonbox.add<GUI::Button>("OK");
    ok_button.set_preferred_size({ 80, 0 });
    ok_button.set_size_policy(GUI::SizePolicy::Fixed, GUI::SizePolicy::Fill);
    ok_button.on_click = [&](auto) { done(ExecOK); };
}

const Vector<String> g_horizontal_alignments { "Left", "Center", "Right" };
const Vector<String> g_vertical_alignments { "Top", "Center", "Bottom" };
Vector<String> g_types;

constexpr static CellTypeDialog::VerticalAlignment vertical_alignment_from(Gfx::TextAlignment alignment)
{
    switch (alignment) {
    case Gfx::TextAlignment::CenterRight:
    case Gfx::TextAlignment::CenterLeft:
    case Gfx::TextAlignment::Center:
        return CellTypeDialog::VerticalAlignment::Center;

    case Gfx::TextAlignment::TopRight:
    case Gfx::TextAlignment::TopLeft:
        return CellTypeDialog::VerticalAlignment::Top;

    case Gfx::TextAlignment::BottomRight:
        return CellTypeDialog::VerticalAlignment::Bottom;
    }

    return CellTypeDialog::VerticalAlignment::Center;
}

constexpr static CellTypeDialog::HorizontalAlignment horizontal_alignment_from(Gfx::TextAlignment alignment)
{
    switch (alignment) {
    case Gfx::TextAlignment::Center:
        return CellTypeDialog::HorizontalAlignment::Center;

    case Gfx::TextAlignment::CenterRight:
    case Gfx::TextAlignment::TopRight:
    case Gfx::TextAlignment::BottomRight:
        return CellTypeDialog::HorizontalAlignment::Right;

    case Gfx::TextAlignment::TopLeft:
    case Gfx::TextAlignment::CenterLeft:
        return CellTypeDialog::HorizontalAlignment::Left;
    }

    return CellTypeDialog::HorizontalAlignment::Right;
}

void CellTypeDialog::setup_tabs(GUI::TabWidget& tabs, const Vector<Position>& positions, Sheet& sheet)
{
    g_types.clear();
    for (auto& type_name : CellType::names())
        g_types.append(type_name);

    Vector<Cell*> cells;
    for (auto& position : positions) {
        if (auto cell = sheet.at(position))
            cells.append(cell);
    }

    if (cells.size() == 1) {
        auto& cell = *cells.first();
        m_format = cell.type_metadata().format;
        m_length = cell.type_metadata().length;
        m_type = &cell.type();
        m_vertical_alignment = vertical_alignment_from(cell.type_metadata().alignment);
        m_horizontal_alignment = horizontal_alignment_from(cell.type_metadata().alignment);
    }

    auto& type_tab = tabs.add_tab<GUI::Widget>("Type");
    type_tab.set_layout<GUI::HorizontalBoxLayout>().set_margins({ 2, 2, 2, 2 });
    {
        auto& left_side = type_tab.add<GUI::Widget>();
        left_side.set_layout<GUI::VerticalBoxLayout>();
        auto& right_side = type_tab.add<GUI::Widget>();
        right_side.set_layout<GUI::VerticalBoxLayout>();
        right_side.set_preferred_size(170, 0);
        right_side.set_size_policy(GUI::SizePolicy::Fixed, GUI::SizePolicy::Fill);

        auto& type_list = left_side.add<GUI::ListView>();
        type_list.set_model(*GUI::ItemListModel<String>::create(g_types));
        type_list.set_multi_select(false);
        type_list.set_should_hide_unnecessary_scrollbars(true);
        type_list.on_selection = [&](auto& index) {
            if (!index.is_valid()) {
                m_type = nullptr;
                return;
            }

            m_type = CellType::get_by_name(g_types.at(index.row()));
        };

        {
            auto& checkbox = right_side.add<GUI::CheckBox>("Override max length");
            auto& spinbox = right_side.add<GUI::SpinBox>();
            checkbox.set_checked(m_length != -1);
            spinbox.set_min(0);
            spinbox.set_enabled(m_length != -1);
            if (m_length > -1)
                spinbox.set_value(m_length);

            checkbox.set_preferred_size(0, 20);
            spinbox.set_preferred_size(0, 20);
            checkbox.set_size_policy(GUI::SizePolicy::Fill, GUI::SizePolicy::Fixed);
            spinbox.set_size_policy(GUI::SizePolicy::Fill, GUI::SizePolicy::Fixed);

            checkbox.on_checked = [&](auto checked) {
                spinbox.set_enabled(checked);
                if (!checked) {
                    m_length = -1;
                    spinbox.set_value(0);
                }
            };
            spinbox.on_change = [&](auto value) {
                m_length = value;
            };
        }
        {
            auto& checkbox = right_side.add<GUI::CheckBox>("Override display format");
            auto& editor = right_side.add<GUI::TextEditor>();
            checkbox.set_checked(!m_format.is_null());
            editor.set_should_hide_unnecessary_scrollbars(true);
            editor.set_enabled(!m_format.is_null());
            editor.set_text(m_format);

            checkbox.set_preferred_size(0, 20);
            editor.set_preferred_size(0, 20);
            checkbox.set_size_policy(GUI::SizePolicy::Fill, GUI::SizePolicy::Fixed);
            editor.set_size_policy(GUI::SizePolicy::Fill, GUI::SizePolicy::Fixed);

            checkbox.on_checked = [&](auto checked) {
                editor.set_enabled(checked);
                if (!checked)
                    m_format = String::empty();
                editor.set_text(m_format);
            };
            editor.on_change = [&] {
                m_format = editor.text();
            };
        }
    }

    auto& alignment_tab = tabs.add_tab<GUI::Widget>("Alignment");
    alignment_tab.set_layout<GUI::VerticalBoxLayout>().set_margins({ 2, 2, 2, 2 });

    // FIXME: Frame?
    // Horizontal alignment
    {
        auto& horizontal_alignment_selection_container = alignment_tab.add<GUI::Widget>();
        horizontal_alignment_selection_container.set_layout<GUI::HorizontalBoxLayout>();
        horizontal_alignment_selection_container.layout()->set_margins({ 0, 4, 0, 0 });
        horizontal_alignment_selection_container.set_size_policy(GUI::SizePolicy::Fill, GUI::SizePolicy::Fixed);
        horizontal_alignment_selection_container.set_preferred_size(0, 22);

        auto& horizontal_alignment_label = horizontal_alignment_selection_container.add<GUI::Label>();
        horizontal_alignment_label.set_text_alignment(Gfx::TextAlignment::CenterLeft);
        horizontal_alignment_label.set_text("Horizontal Text Alignment");

        auto& horizontal_combobox = alignment_tab.add<GUI::ComboBox>();
        horizontal_combobox.set_size_policy(GUI::SizePolicy::Fill, GUI::SizePolicy::Fixed);
        horizontal_combobox.set_preferred_size(0, 22);
        horizontal_combobox.set_only_allow_values_from_model(true);
        horizontal_combobox.set_model(*GUI::ItemListModel<String>::create(g_horizontal_alignments));
        horizontal_combobox.set_selected_index((int)m_horizontal_alignment);
        horizontal_combobox.on_change = [&](auto&, const GUI::ModelIndex& index) {
            switch (index.row()) {
            case 0:
                m_horizontal_alignment = HorizontalAlignment::Left;
                break;
            case 1:
                m_horizontal_alignment = HorizontalAlignment::Center;
                break;
            case 2:
                m_horizontal_alignment = HorizontalAlignment::Right;
                break;
            default:
                ASSERT_NOT_REACHED();
            }
        };
    }

    // Vertical alignment
    {
        auto& vertical_alignment_container = alignment_tab.add<GUI::Widget>();
        vertical_alignment_container.set_layout<GUI::HorizontalBoxLayout>();
        vertical_alignment_container.layout()->set_margins({ 0, 4, 0, 0 });
        vertical_alignment_container.set_size_policy(GUI::SizePolicy::Fill, GUI::SizePolicy::Fixed);
        vertical_alignment_container.set_preferred_size(0, 22);

        auto& vertical_alignment_label = vertical_alignment_container.add<GUI::Label>();
        vertical_alignment_label.set_text_alignment(Gfx::TextAlignment::CenterLeft);
        vertical_alignment_label.set_text("Vertical Text Alignment");

        auto& vertical_combobox = alignment_tab.add<GUI::ComboBox>();
        vertical_combobox.set_size_policy(GUI::SizePolicy::Fill, GUI::SizePolicy::Fixed);
        vertical_combobox.set_preferred_size(0, 22);
        vertical_combobox.set_only_allow_values_from_model(true);
        vertical_combobox.set_model(*GUI::ItemListModel<String>::create(g_vertical_alignments));
        vertical_combobox.set_selected_index((int)m_vertical_alignment);
        vertical_combobox.on_change = [&](auto&, const GUI::ModelIndex& index) {
            switch (index.row()) {
            case 0:
                m_vertical_alignment = VerticalAlignment::Top;
                break;
            case 1:
                m_vertical_alignment = VerticalAlignment::Center;
                break;
            case 2:
                m_vertical_alignment = VerticalAlignment::Bottom;
                break;
            default:
                ASSERT_NOT_REACHED();
            }
        };
    }
}

CellTypeMetadata CellTypeDialog::metadata() const
{
    CellTypeMetadata metadata;
    metadata.format = m_format;
    metadata.length = m_length;

    switch (m_vertical_alignment) {
    case VerticalAlignment::Top:
        switch (m_horizontal_alignment) {
        case HorizontalAlignment::Left:
            metadata.alignment = Gfx::TextAlignment::TopLeft;
            break;
        case HorizontalAlignment::Center:
            metadata.alignment = Gfx::TextAlignment::Center; // TopCenter?
            break;
        case HorizontalAlignment::Right:
            metadata.alignment = Gfx::TextAlignment::TopRight;
            break;
        }
        break;
    case VerticalAlignment::Center:
        switch (m_horizontal_alignment) {
        case HorizontalAlignment::Left:
            metadata.alignment = Gfx::TextAlignment::CenterLeft;
            break;
        case HorizontalAlignment::Center:
            metadata.alignment = Gfx::TextAlignment::Center;
            break;
        case HorizontalAlignment::Right:
            metadata.alignment = Gfx::TextAlignment::CenterRight;
            break;
        }
        break;
    case VerticalAlignment::Bottom:
        switch (m_horizontal_alignment) {
        case HorizontalAlignment::Left:
            metadata.alignment = Gfx::TextAlignment::CenterLeft; // BottomLeft?
            break;
        case HorizontalAlignment::Center:
            metadata.alignment = Gfx::TextAlignment::Center;
            break;
        case HorizontalAlignment::Right:
            metadata.alignment = Gfx::TextAlignment::BottomRight;
            break;
        }
        break;
    }

    return metadata;
}

}
