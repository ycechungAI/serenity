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

#pragma once

#include <LibGUI/GFrame.h>

class GScrollBar;

class GScrollableWidget : public GFrame {
    C_OBJECT(GScrollableWidget)
public:
    virtual ~GScrollableWidget() override;

    Size content_size() const { return m_content_size; }
    int content_width() const { return m_content_size.width(); }
    int content_height() const { return m_content_size.height(); }

    Rect visible_content_rect() const;

    Rect widget_inner_rect() const;

    void scroll_into_view(const Rect&, Orientation);
    void scroll_into_view(const Rect&, bool scroll_horizontally, bool scroll_vertically);

    void set_scrollbars_enabled(bool);
    bool is_scrollbars_enabled() const { return m_scrollbars_enabled; }

    Size available_size() const;

    GScrollBar& vertical_scrollbar() { return *m_vertical_scrollbar; }
    const GScrollBar& vertical_scrollbar() const { return *m_vertical_scrollbar; }
    GScrollBar& horizontal_scrollbar() { return *m_horizontal_scrollbar; }
    const GScrollBar& horizontal_scrollbar() const { return *m_horizontal_scrollbar; }
    GWidget& corner_widget() { return *m_corner_widget; }
    const GWidget& corner_widget() const { return *m_corner_widget; }

    void scroll_to_top();
    void scroll_to_bottom();

    int width_occupied_by_vertical_scrollbar() const;
    int height_occupied_by_horizontal_scrollbar() const;

    void set_should_hide_unnecessary_scrollbars(bool b) { m_should_hide_unnecessary_scrollbars = b; }
    bool should_hide_unnecessary_scrollbars() const { return m_should_hide_unnecessary_scrollbars; }

    Point to_content_position(const Point& widget_position) const;
    Point to_widget_position(const Point& content_position) const;

protected:
    explicit GScrollableWidget(GWidget* parent);
    virtual void custom_layout() override;
    virtual void resize_event(GResizeEvent&) override;
    virtual void mousewheel_event(GMouseEvent&) override;
    virtual void did_scroll() {}
    void set_content_size(const Size&);
    void set_size_occupied_by_fixed_elements(const Size&);

private:
    void update_scrollbar_ranges();

    RefPtr<GScrollBar> m_vertical_scrollbar;
    RefPtr<GScrollBar> m_horizontal_scrollbar;
    RefPtr<GWidget> m_corner_widget;
    Size m_content_size;
    Size m_size_occupied_by_fixed_elements;
    bool m_scrollbars_enabled { true };
    bool m_should_hide_unnecessary_scrollbars { false };
};
