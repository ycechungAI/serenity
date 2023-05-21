/*
 * Copyright (c) 2023, Luke Wilde <lukew@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <LibWeb/Bindings/Intrinsics.h>
#include <LibWeb/WebAudio/BaseAudioContext.h>

namespace Web::WebAudio {

BaseAudioContext::BaseAudioContext(JS::Realm& realm)
    : DOM::EventTarget(realm)
{
}

BaseAudioContext::~BaseAudioContext() = default;

JS::ThrowCompletionOr<void> BaseAudioContext::initialize(JS::Realm& realm)
{
    MUST_OR_THROW_OOM(Base::initialize(realm));
    set_prototype(&Bindings::ensure_web_prototype<Bindings::BaseAudioContextPrototype>(realm, "BaseAudioContext"));

    return {};
}

}
