/*
 * Copyright (c) 2022, Tim Flynn <trflynn89@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/String.h>
#include <AK/StringView.h>
#include <LibJS/Runtime/Intl/NumberFormat.h>
#include <LibJS/Runtime/Object.h>
#include <LibUnicode/PluralRules.h>

namespace JS::Intl {

class PluralRules final : public NumberFormatBase {
    JS_OBJECT(PluralRules, NumberFormatBase);

public:
    PluralRules(Object& prototype);
    virtual ~PluralRules() override = default;

    Unicode::PluralForm type() const { return m_type; }
    StringView type_string() const { return Unicode::plural_form_to_string(m_type); }
    void set_type(StringView type) { m_type = Unicode::plural_form_from_string(type); }

private:
    Unicode::PluralForm m_type { Unicode::PluralForm::Cardinal }; // [[Type]]
};

}
