/*
 * Copyright (c) 2018-2020, Andreas Kling <kling@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <LibGUI/Widget.h>

namespace GUI {

class SpinBox : public Widget {
    C_OBJECT(SpinBox)
public:
    virtual ~SpinBox() override;

    int value() const { return m_value; }
    void set_value(int);

    int min() const { return m_min; }
    int max() const { return m_max; }
    void set_min(int min) { set_range(min, max()); }
    void set_max(int max) { set_range(min(), max); }
    void set_range(int min, int max);

    Function<void(int value)> on_change;

protected:
    SpinBox();

    virtual void mousewheel_event(MouseEvent&) override;
    virtual void resize_event(ResizeEvent&) override;

private:
    RefPtr<TextEditor> m_editor;
    RefPtr<Button> m_increment_button;
    RefPtr<Button> m_decrement_button;

    int m_min { 0 };
    int m_max { 100 };
    int m_value { 0 };
};

}
