/*
 * Copyright (c) 2019-2020, Sergey Bugaev <bugaevc@serenityos.org>
 * Copyright (c) 2021, Spencer Dixon <spencercdixon@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/DeprecatedString.h>
#include <AK/DistinctNumeric.h>
#include <AK/Function.h>
#include <AK/Result.h>
#include <LibCore/Object.h>
#include <pthread.h>

namespace Threading {

AK_TYPEDEF_DISTINCT_ORDERED_ID(intptr_t, ThreadError);

class Thread final : public Core::Object {
    C_OBJECT(Thread);

public:
    virtual ~Thread();

    ErrorOr<void> set_priority(int priority);
    ErrorOr<int> get_priority() const;

    void start();
    void detach();

    template<typename T = void>
    Result<T, ThreadError> join();

    DeprecatedString thread_name() const;
    pthread_t tid() const;
    bool is_started() const;

private:
    explicit Thread(Function<intptr_t()> action, StringView thread_name = {});
    Function<intptr_t()> m_action;
    pthread_t m_tid { 0 };
    DeprecatedString m_thread_name;
    bool m_detached { false };
    bool m_started { false };
};

template<typename T>
Result<T, ThreadError> Thread::join()
{
    void* thread_return = nullptr;
    int rc = pthread_join(m_tid, &thread_return);
    if (rc != 0) {
        return ThreadError { rc };
    }

    m_tid = 0;
    if constexpr (IsVoid<T>)
        return {};
    else
        return { static_cast<T>(thread_return) };
}

}

template<>
struct AK::Formatter<Threading::Thread> : AK::Formatter<FormatString> {
    ErrorOr<void> format(FormatBuilder& builder, Threading::Thread const& thread)
    {
        return Formatter<FormatString>::format(builder, "Thread \"{}\"({})"sv, thread.thread_name(), thread.tid());
    }
};
