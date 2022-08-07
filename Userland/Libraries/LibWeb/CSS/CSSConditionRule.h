/*
 * Copyright (c) 2021-2022, Sam Atkins <atkinssj@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/NonnullRefPtr.h>
#include <LibWeb/CSS/CSSGroupingRule.h>
#include <LibWeb/Forward.h>

namespace Web::CSS {

class CSSConditionRule : public CSSGroupingRule {
    AK_MAKE_NONCOPYABLE(CSSConditionRule);
    AK_MAKE_NONMOVABLE(CSSConditionRule);
    JS_OBJECT(CSSConditionRule, CSSGroupingRule);

public:
    CSSConditionRule& impl() { return *this; }

    virtual ~CSSConditionRule() = default;

    virtual String condition_text() const = 0;
    virtual void set_condition_text(String) = 0;
    virtual bool condition_matches() const = 0;

    virtual void for_each_effective_style_rule(Function<void(CSSStyleRule const&)> const& callback) const override;

protected:
    explicit CSSConditionRule(Bindings::WindowObject&, CSSRuleList&);
};

}

namespace Web::Bindings {
inline JS::Object* wrap(JS::Realm&, Web::CSS::CSSConditionRule& object) { return &object; }
using CSSConditionRuleWrapper = Web::CSS::CSSConditionRule;
}
