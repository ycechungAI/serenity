/*
 * Copyright (c) 2023, Luke Wilde <lukew@serenityos.org>
 * Copyright (c) 2024, Shannon Booth <shannon@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <LibWeb/Bindings/BaseAudioContextPrototype.h>
#include <LibWeb/Bindings/Intrinsics.h>
#include <LibWeb/HTML/EventNames.h>
#include <LibWeb/WebAudio/BaseAudioContext.h>

namespace Web::WebAudio {

BaseAudioContext::BaseAudioContext(JS::Realm& realm)
    : DOM::EventTarget(realm)
{
}

BaseAudioContext::~BaseAudioContext() = default;

void BaseAudioContext::initialize(JS::Realm& realm)
{
    Base::initialize(realm);
    WEB_SET_PROTOTYPE_FOR_INTERFACE(BaseAudioContext);
}

void BaseAudioContext::set_onstatechange(WebIDL::CallbackType* event_handler)
{
    set_event_handler_attribute(HTML::EventNames::statechange, event_handler);
}

WebIDL::CallbackType* BaseAudioContext::onstatechange()
{
    return event_handler_attribute(HTML::EventNames::statechange);
}

// https://webaudio.github.io/web-audio-api/#dom-baseaudiocontext-createbuffer
WebIDL::ExceptionOr<void> BaseAudioContext::verify_audio_options_inside_nominal_range(JS::Realm& realm, WebIDL::UnsignedLong number_of_channels, WebIDL::UnsignedLong length, float sample_rate)
{
    // A NotSupportedError exception MUST be thrown if any of the arguments is negative, zero, or outside its nominal range.

    if (number_of_channels == 0)
        return WebIDL::NotSupportedError::create(realm, "Number of channels must not be '0'"_fly_string);

    if (number_of_channels > MAX_NUMBER_OF_CHANNELS)
        return WebIDL::NotSupportedError::create(realm, "Number of channels is greater than allowed range"_fly_string);

    if (length == 0)
        return WebIDL::NotSupportedError::create(realm, "Length of buffer must be at least 1"_fly_string);

    if (sample_rate < MIN_SAMPLE_RATE || sample_rate > MAX_SAMPLE_RATE)
        return WebIDL::NotSupportedError::create(realm, "Sample rate is outside of allowed range"_fly_string);

    return {};
}

}
