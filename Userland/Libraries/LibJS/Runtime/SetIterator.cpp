/*
 * Copyright (c) 2021, Idan Horowitz <idan.horowitz@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <LibJS/Runtime/GlobalObject.h>
#include <LibJS/Runtime/SetIterator.h>

namespace JS {

SetIterator* SetIterator::create(Realm& realm, Set& set, Object::PropertyKind iteration_kind)
{
    return realm.heap().allocate<SetIterator>(realm, set, iteration_kind, *realm.global_object().set_iterator_prototype());
}

SetIterator::SetIterator(Set& set, Object::PropertyKind iteration_kind, Object& prototype)
    : Object(prototype)
    , m_set(set)
    , m_iteration_kind(iteration_kind)
    , m_iterator(static_cast<Set const&>(set).begin())
{
}

void SetIterator::visit_edges(Cell::Visitor& visitor)
{
    Base::visit_edges(visitor);
    visitor.visit(&m_set);
}

}
