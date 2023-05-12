/*
 * Copyright (c) 2020, the SerenityOS developers.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <LibWeb/HTML/HTMLElement.h>

namespace Web::HTML {

class HTMLSourceElement final : public HTMLElement {
    WEB_PLATFORM_OBJECT(HTMLSourceElement, HTMLElement);

public:
    virtual ~HTMLSourceElement() override;

private:
    HTMLSourceElement(DOM::Document&, DOM::QualifiedName);

    virtual JS::ThrowCompletionOr<void> initialize(JS::Realm&) override;

    virtual void inserted() override;
    virtual void removed_from(DOM::Node*) override;
};

}
