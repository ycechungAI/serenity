/*
 * Copyright (c) 2018-2020, Andreas Kling <kling@serenityos.org>
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

#include <LibGUI/Action.h>
#include <LibGUI/ActionGroup.h>
#include <LibGUI/BoxLayout.h>
#include <LibGUI/Button.h>
#include <LibGUI/Painter.h>
#include <LibGUI/ToolBar.h>
#include <LibGfx/Palette.h>

namespace GUI {

ToolBar::ToolBar(Orientation orientation, int button_size)
    : m_button_size(button_size)
{
    if (orientation == Orientation::Horizontal) {
        set_size_policy(SizePolicy::Fill, SizePolicy::Fixed);
        set_preferred_size(0, button_size + 8);
    } else {
        set_size_policy(SizePolicy::Fixed, SizePolicy::Fill);
        set_preferred_size(button_size + 8, 0);
    }
    set_layout<BoxLayout>(orientation);
    layout()->set_spacing(0);
    layout()->set_margins({ 2, 2, 2, 2 });
}

ToolBar::~ToolBar()
{
}

class ToolBarButton final : public Button {
    C_OBJECT(ToolBarButton);

public:
    virtual ~ToolBarButton() override { }

private:
    explicit ToolBarButton(Action& action)
    {
        if (action.group() && action.group()->is_exclusive())
            set_exclusive(true);
        set_action(action);
        set_tooltip(action.text());
        set_focus_policy(FocusPolicy::TabFocus);
        if (action.icon())
            set_icon(action.icon());
        else
            set_text(action.text());
        set_button_style(Gfx::ButtonStyle::CoolBar);
        set_size_policy(SizePolicy::Fixed, SizePolicy::Fixed);
    }
};

void ToolBar::add_action(Action& action)
{
    auto item = make<Item>();
    item->type = Item::Type::Action;
    item->action = action;

    auto& button = add<ToolBarButton>(action);
    button.set_preferred_size(m_button_size + 8, m_button_size + 8);

    m_items.append(move(item));
}

class SeparatorWidget final : public Widget {
    C_OBJECT(SeparatorWidget)
public:
    SeparatorWidget()
    {
        set_size_policy(SizePolicy::Fixed, SizePolicy::Fixed);
        set_preferred_size(8, 18);
    }
    virtual ~SeparatorWidget() override { }

    virtual void paint_event(PaintEvent& event) override
    {
        Painter painter(*this);
        painter.add_clip_rect(event.rect());
        painter.translate(rect().center().x() - 1, 0);
        painter.draw_line({ 0, 0 }, { 0, rect().bottom() }, palette().threed_shadow1());
        painter.draw_line({ 1, 0 }, { 1, rect().bottom() }, palette().threed_highlight());
    }
};

void ToolBar::add_separator()
{
    auto item = make<Item>();
    item->type = Item::Type::Separator;
    add<SeparatorWidget>();
    m_items.append(move(item));
}

void ToolBar::paint_event(PaintEvent& event)
{
    Painter painter(*this);
    painter.add_clip_rect(event.rect());

    painter.fill_rect(event.rect(), palette().button());
}

}
