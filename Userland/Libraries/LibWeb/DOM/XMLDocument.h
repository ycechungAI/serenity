/*
 * Copyright (c) 2023, Luke Wilde <lukew@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <LibWeb/DOM/Document.h>

namespace Web::DOM {

class XMLDocument final : public Document {
    WEB_PLATFORM_OBJECT(XMLDocument, Document);

public:
    virtual ~XMLDocument() override = default;

private:
    XMLDocument(JS::Realm& realm, AK::URL const& url);

    virtual void initialize(JS::Realm&) override;
};

}
