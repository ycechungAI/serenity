/*
 * Copyright (c) 2021-2022, Tim Flynn <trflynn89@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <LibJS/Runtime/NativeFunction.h>

namespace JS::Intl {

class DateTimeFormatConstructor final : public NativeFunction {
    JS_OBJECT(DateTimeFormatConstructor, NativeFunction);

public:
    virtual void initialize(Realm&) override;
    virtual ~DateTimeFormatConstructor() override = default;

    virtual ThrowCompletionOr<Value> call() override;
    virtual ThrowCompletionOr<Object*> construct(FunctionObject& new_target) override;

private:
    explicit DateTimeFormatConstructor(Realm&);

    virtual bool has_constructor() const override { return true; }

    JS_DECLARE_NATIVE_FUNCTION(supported_locales_of);
};

ThrowCompletionOr<DateTimeFormat*> initialize_date_time_format(VM&, DateTimeFormat&, Value locales_value, Value options_value);

}
