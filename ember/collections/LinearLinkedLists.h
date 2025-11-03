#pragma once

#include <concepts>
#include <functional>

#include <cstdio>

#include "FreeableVector.h"

namespace ember::collections {

    template<typename T, typename I = size_t>
    class LinearLinkedLists {
    private:
        struct Node {
            T obj;
            I next;
        };

    public:
        static constexpr I END_OF_LIST = I(-1);

        LinearLinkedLists() = default;
        LinearLinkedLists(const LinearLinkedLists&) = default;
        LinearLinkedLists(LinearLinkedLists&&) = default;

        void reserve(size_t capacity) {
            m_nodes.reserve(capacity);
        }

        I insert_into(I& head, const T& obj) {
            auto index = m_nodes.insert({ .obj = obj, .next = END_OF_LIST });
            prepend_to_list(head, index);
            return index;
        }

        void erase_if(I& head, const std::function<bool(const T&)>& pred) {
            auto prev = &head;

            auto current = *prev;
            while (current != END_OF_LIST) {
                if (pred(m_nodes.at(current).obj)) {
                    *prev = m_nodes.at(current).next; // update prev to point to current.next
                    m_nodes.erase(current);
                } else {
                    prev = &(m_nodes.at(current).next); // prev now points to current
                }
                current = *prev;
            }
        }

        I size_of(I head) const {
            I count = 0;
            I current = head;
            while (current != END_OF_LIST) {
                count++;
                current = m_nodes.at(current).next;
            }
            return count;
        }

        size_t size() const {
            return m_nodes.size();
        }

        class iterator {
        public:
            using value_type = T;
            using difference_type = ptrdiff_t;

            iterator(): current(END_OF_LIST), nodes(nullptr) { }
            iterator(I head, FreeableVector<Node>* nodes): current(head), nodes(nodes) { }
            iterator(const iterator&) = default;

            value_type& operator*() const {
                assert(current != END_OF_LIST);
                return nodes->at(current).obj;
            }

            value_type* operator->() const {
                assert(current != END_OF_LIST);
                return &(nodes->at(current).obj);
            }

            iterator& operator++() {
                assert(current != END_OF_LIST);
                if (current != END_OF_LIST) {
                    current = nodes->at(current).next;
                }
                return *this;
            }

            iterator operator++(int) {
                auto tmp = *this;
                ++*this;
                return tmp;
            }

            bool operator==(const iterator& rhs) const {
                return (current == rhs.current)
                    && ((current == END_OF_LIST) || (nodes->data() == rhs.nodes->data()));
            }
            bool operator!=(const iterator& rhs) const {
                return !(*this == rhs);
            }

        private:
            I current;
            FreeableVector<Node>* nodes;
        };
        static_assert(std::forward_iterator<iterator>);

        iterator begin(I& head) {
            return iterator(head, &m_nodes);
        }

        iterator end() {
            return iterator();
        }

        class const_iterator {
        public:
            using value_type = const T;
            using difference_type = ptrdiff_t;

            const_iterator(const FreeableVector<Node>* nodes): current(END_OF_LIST), nodes(nodes) { }
            const_iterator(I head, const FreeableVector<Node>* nodes): current(head), nodes(nodes) { }
            const_iterator(const const_iterator&) = default;

            value_type& operator*() const {
                assert(current != END_OF_LIST);
                return nodes->at(current).obj;
            }

            value_type* operator->() const {
                assert(current != END_OF_LIST);
                return &(nodes->at(current).obj);
            }

            iterator& operator++() {
                assert(current != END_OF_LIST);
                if (current != END_OF_LIST) {
                    current = nodes->at(current).next;
                }
                return *this;
            }

            iterator operator++(int) {
                auto tmp = *this;
                ++*this;
                return tmp;
            }

            bool operator==(const iterator& rhs) const {
                return (current == rhs.current)
                    && ((current == END_OF_LIST) || (nodes->data() == rhs.nodes->data()));
            }
            bool operator!=(const iterator& rhs) const {
                return !(*this == rhs);
            }

        private:
            I current;
            const FreeableVector<Node>* nodes;
        };
        static_assert(std::forward_iterator<iterator>);

        const_iterator begin(I head) const {
            return const_iterator(head, &m_nodes);
        }

        const_iterator end() const {
            return const_iterator();
        }

    private:
        FreeableVector<Node> m_nodes;

        void prepend_to_list(I& head, I index) {
            m_nodes[index].next = head;
            head = index;
        }
    };

}