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

#include <AK/NonnullRefPtrVector.h>
#include <LibWeb/DOM/Element.h>
#include <LibWeb/Forward.h>

namespace Web {

class StackOfOpenElements {
public:
    StackOfOpenElements() { }
    ~StackOfOpenElements();

    bool is_empty() const { return m_elements.is_empty(); }
    void push(NonnullRefPtr<Element> element) { m_elements.append(move(element)); }
    NonnullRefPtr<Element> pop() { return m_elements.take_last(); }

    const Element& current_node() const { return m_elements.last(); }
    Element& current_node() { return m_elements.last(); }

    bool has_in_scope(const FlyString& tag_name) const;
    bool has_in_button_scope(const FlyString& tag_name) const;
    bool has_in_table_scope(const FlyString& tag_name) const;

    bool contains(const Element&) const;

    const NonnullRefPtrVector<Element>& elements() const { return m_elements; }

private:
    bool has_in_scope_impl(const FlyString& tag_name, const Vector<FlyString>&) const;

    NonnullRefPtrVector<Element> m_elements;
};

}
