/*
 * Copyright (c) 2020, the SerenityOS developers.
 * Copyright (c) 2022, Andreas Kling <kling@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <LibWeb/HTML/HTMLElement.h>

namespace Web::HTML {

class HTMLOptionElement final : public HTMLElement {
    WEB_PLATFORM_OBJECT(HTMLOptionElement, HTMLElement);

public:
    virtual ~HTMLOptionElement() override;

    bool selected() const { return m_selected; }
    void set_selected(bool);

    String value() const;
    void set_value(String);

    String text() const;
    void set_text(String);

    int index() const;

    bool disabled() const;

private:
    friend class Bindings::OptionConstructor;
    friend class HTMLSelectElement;

    HTMLOptionElement(DOM::Document&, DOM::QualifiedName);

    void parse_attribute(FlyString const& name, String const& value) override;
    void did_remove_attribute(FlyString const& name) override;

    void ask_for_a_reset();

    // https://html.spec.whatwg.org/multipage/form-elements.html#concept-option-selectedness
    bool m_selected { false };

    // https://html.spec.whatwg.org/multipage/form-elements.html#concept-option-dirtiness
    bool m_dirty { false };
};

}
