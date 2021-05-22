/*
 * Copyright (c) 2018-2021, Andreas Kling <kling@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "ThreadStackWidget.h"
#include <AK/ByteBuffer.h>
#include <LibCore/Timer.h>
#include <LibGUI/BoxLayout.h>
#include <LibSymbolication/Symbolication.h>

ThreadStackWidget::ThreadStackWidget()
{
    set_layout<GUI::VerticalBoxLayout>();
    layout()->set_margins({ 4, 4, 4, 4 });
    m_stack_editor = add<GUI::TextEditor>();
    m_stack_editor->set_mode(GUI::TextEditor::ReadOnly);
}

ThreadStackWidget::~ThreadStackWidget()
{
}

void ThreadStackWidget::show_event(GUI::ShowEvent&)
{
    refresh();
    if (!m_timer)
        m_timer = add<Core::Timer>(1000, [this] { refresh(); });
}

void ThreadStackWidget::hide_event(GUI::HideEvent&)
{
    m_timer = nullptr;
}

void ThreadStackWidget::set_ids(pid_t pid, pid_t tid)
{
    if (m_pid == pid && m_tid == tid)
        return;
    m_pid = pid;
    m_tid = tid;
}

void ThreadStackWidget::refresh()
{
    auto symbols = Symbolication::symbolicate_thread(m_pid, m_tid);

    StringBuilder builder;

    for (auto& symbol : symbols) {
        builder.appendff("{:p}", symbol.address);
        if (!symbol.name.is_empty())
            builder.appendff("  {}", symbol.name);
        builder.append('\n');
    }

    if (m_stack_editor->text() != builder.string_view()) {
        m_stack_editor->set_text(builder.string_view());
    }
}
