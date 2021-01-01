/*
 * Copyright (c) 2020, Andreas Kling <kling@serenityos.org>
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

#include <AK/Function.h>
#include <LibJS/Runtime/Function.h>

namespace JS {

class NativeFunction : public Function {
    JS_OBJECT(NativeFunction, Function);

public:
    static NativeFunction* create(GlobalObject&, const FlyString& name, AK::Function<Value(VM&, GlobalObject&)>);

    explicit NativeFunction(const FlyString& name, AK::Function<Value(VM&, GlobalObject&)>, Object& prototype);
    virtual void initialize(GlobalObject&) override { }
    virtual ~NativeFunction() override;

    virtual Value call() override;
    virtual Value construct(Function& new_target) override;

    virtual const FlyString& name() const override { return m_name; };
    virtual bool has_constructor() const { return false; }

    virtual bool is_strict_mode() const override;

protected:
    NativeFunction(const FlyString& name, Object& prototype);
    explicit NativeFunction(Object& prototype);

private:
    virtual LexicalEnvironment* create_environment() override final;

    FlyString m_name;
    AK::Function<Value(VM&, GlobalObject&)> m_native_function;
};

}
