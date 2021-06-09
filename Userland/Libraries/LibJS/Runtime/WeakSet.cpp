/*
 * Copyright (c) 2021, Idan Horowitz <idan.horowitz@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <LibJS/Runtime/WeakSet.h>

namespace JS {

WeakSet* WeakSet::create(GlobalObject& global_object)
{
    return global_object.heap().allocate<WeakSet>(global_object, *global_object.weak_set_prototype());
}

WeakSet::WeakSet(Object& prototype)
    : Object(prototype)
{
}

WeakSet::~WeakSet()
{
}

}
