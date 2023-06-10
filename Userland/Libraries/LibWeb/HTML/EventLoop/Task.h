/*
 * Copyright (c) 2021-2022, Andreas Kling <kling@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Function.h>
#include <AK/NonnullOwnPtr.h>
#include <AK/RefPtr.h>
#include <LibJS/Heap/Handle.h>
#include <LibJS/SafeFunction.h>
#include <LibWeb/Forward.h>

namespace Web::HTML {

struct UniqueTaskSource;

class Task {
public:
    // https://html.spec.whatwg.org/multipage/webappapis.html#generic-task-sources
    enum class Source {
        Unspecified,
        DOMManipulation,
        UserInteraction,
        Networking,
        HistoryTraversal,
        IdleTask,
        PostedMessage,
        Microtask,
        TimerTask,
        JavaScriptEngine,

        // Some elements, such as the HTMLMediaElement, must have a unique task source per instance.
        // Keep this field last, to serve as the base value of all unique task sources.
        UniqueTaskSourceStart,

        // https://html.spec.whatwg.org/multipage/webappapis.html#navigation-and-traversal-task-source
        NavigationAndTraversal,

        // https://w3c.github.io/FileAPI/#fileReadingTaskSource
        FileReading,
    };

    static NonnullOwnPtr<Task> create(Source source, DOM::Document const* document, JS::SafeFunction<void()> steps)
    {
        return adopt_own(*new Task(source, document, move(steps)));
    }
    ~Task();

    Source source() const { return m_source; }
    void execute();

    DOM::Document const* document() const;

    bool is_runnable() const;

private:
    Task(Source, DOM::Document const*, JS::SafeFunction<void()> steps);

    Source m_source { Source::Unspecified };
    JS::SafeFunction<void()> m_steps;
    JS::Handle<DOM::Document const> m_document;
};

struct UniqueTaskSource {
    UniqueTaskSource();
    ~UniqueTaskSource();

    Task::Source const source;
};

}
