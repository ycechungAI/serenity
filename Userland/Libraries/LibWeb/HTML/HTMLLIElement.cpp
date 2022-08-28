/*
 * Copyright (c) 2020, the SerenityOS developers.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <LibWeb/Bindings/HTMLLIElementPrototype.h>
#include <LibWeb/HTML/HTMLLIElement.h>
#include <LibWeb/HTML/Window.h>

namespace Web::HTML {

HTMLLIElement::HTMLLIElement(DOM::Document& document, DOM::QualifiedName qualified_name)
    : HTMLElement(document, move(qualified_name))
{
    set_prototype(&window().ensure_web_prototype<Bindings::HTMLLIElementPrototype>("HTMLLIElement"));
}

HTMLLIElement::~HTMLLIElement() = default;
}
