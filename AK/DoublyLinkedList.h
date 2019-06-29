#pragma once

#include <AK/Assertions.h>
#include <AK/StdLibExtras.h>
#include <AK/Traits.h>

namespace AK {

template<typename ListType, typename ElementType>
class DoublyLinkedListIterator {
public:
    bool operator!=(const DoublyLinkedListIterator& other) const { return m_node != other.m_node; }
    bool operator==(const DoublyLinkedListIterator& other) const { return m_node == other.m_node; }
    DoublyLinkedListIterator& operator++()
    {
        m_node = m_node->next;
        return *this;
    }
    ElementType& operator*() { return m_node->value; }
    ElementType* operator->() { return &m_node->value; }
    bool is_end() const { return !m_node; }
    static DoublyLinkedListIterator universal_end() { return DoublyLinkedListIterator(nullptr); }

private:
    friend ListType;
    explicit DoublyLinkedListIterator(typename ListType::Node* node)
        : m_node(node)
    {
    }
    typename ListType::Node* m_node;
};

template<typename T>
class DoublyLinkedList {
private:
    struct Node {
        explicit Node(const T& v)
            : value(v)
        {
        }
        explicit Node(T&& v)
            : value(move(v))
        {
        }
        T value;
        Node* next { nullptr };
        Node* prev { nullptr };
    };

public:
    DoublyLinkedList() {}
    ~DoublyLinkedList() { clear(); }

    bool is_empty() const { return !head(); }

    void clear()
    {
        for (auto* node = m_head; node;) {
            auto* next = node->next;
            delete node;
            node = next;
        }
        m_head = nullptr;
        m_tail = nullptr;
    }

    T& first()
    {
        ASSERT(head());
        return head()->value;
    }
    const T& first() const
    {
        ASSERT(head());
        return head()->value;
    }
    T& last()
    {
        ASSERT(head());
        return tail()->value;
    }
    const T& last() const
    {
        ASSERT(head());
        return tail()->value;
    }

    void append(T&& value)
    {
        append_node(new Node(move(value)));
    }

    void append(const T& value)
    {
        append_node(new Node(value));
    }

    bool contains_slow(const T& value) const
    {
        for (auto* node = m_head; node; node = node->next) {
            if (node->value == value)
                return true;
        }
        return false;
    }

    using Iterator = DoublyLinkedListIterator<DoublyLinkedList, T>;
    friend Iterator;
    Iterator begin() { return Iterator(m_head); }
    Iterator end() { return Iterator::universal_end(); }

    using ConstIterator = DoublyLinkedListIterator<const DoublyLinkedList, const T>;
    friend ConstIterator;
    ConstIterator begin() const { return ConstIterator(m_head); }
    ConstIterator end() const { return ConstIterator::universal_end(); }

    ConstIterator find(const T& value) const
    {
        for (auto* node = m_head; node; node = node->next) {
            if (Traits<T>::equals(node->value, value))
                return ConstIterator(node);
        }
        return end();
    }

    Iterator find(const T& value)
    {
        for (auto* node = m_head; node; node = node->next) {
            if (Traits<T>::equals(node->value, value))
                return Iterator(node);
        }
        return end();
    }

    void remove(Iterator& it)
    {
        ASSERT(it.m_node);
        auto* node = it.m_node;
        if (node->prev) {
            ASSERT(node != m_head);
            node->prev->next = node->next;
        } else {
            ASSERT(node == m_head);
            m_head = node->next;
        }
        if (node->next) {
            ASSERT(node != m_tail);
            node->next->prev = node->prev;
        } else {
            ASSERT(node == m_tail);
            m_tail = node->prev;
        }
        delete node;
    }

private:
    void append_node(Node* node)
    {
        if (!m_head) {
            ASSERT(!m_tail);
            m_head = node;
            m_tail = node;
            return;
        }
        ASSERT(m_tail);
        m_tail->next = node;
        node->prev = m_tail;
        m_tail = node;
    }

    Node* head() { return m_head; }
    const Node* head() const { return m_head; }

    Node* tail() { return m_tail; }
    const Node* tail() const { return m_tail; }

    Node* m_head { nullptr };
    Node* m_tail { nullptr };
};

}

using AK::DoublyLinkedList;
