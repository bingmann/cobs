/*******************************************************************************
 * cobs/util/addressable_priority_queue.hpp
 *
 * Copyright (c) 2010-2011 Raoul Steffen <R-Steffen@gmx.de>
 * Copyright (c) 2018 Timo Bingmann <tb@panthema.net>
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#ifndef COBS_UTIL_ADDRESSABLE_PRIORITY_QUEUE_HEADER
#define COBS_UTIL_ADDRESSABLE_PRIORITY_QUEUE_HEADER

#include <cassert>
#include <map>
#include <set>
#include <utility>

namespace cobs {

/*!
 * An internal priority queue that allows removing elements addressed with (a
 * copy of) themselves.
 *
 * \tparam Key Type of contained elements.  \tparam Priority Type of
 * Priority.
 */
template <typename Key, typename Priority,
          typename Cmp = std::less<Priority> >
class AddressablePriorityQueue
{
    using PriorityKeyPair = std::pair<Priority, Key>;

    struct ComparePair // like < for pair, but uses Cmp for < on first
    {
        bool operator () (const PriorityKeyPair& left,
                          const PriorityKeyPair& right) const {
            Cmp cmp_;
            return cmp_(left.first, right.first) ||
                   ((!cmp_(right.first, left.first)) && left.second < right.second);
        }
    };

    using container_type = std::set<PriorityKeyPair, ComparePair>;
    using container_iterator = typename container_type::iterator;
    using handle_map_type = std::map<Key, container_iterator>;

    container_type container_;
    handle_map_type handle_map_;

public:
    //! Type of handle to an entry. For use with insert and remove.
    using handle = typename handle_map_type::iterator;

    //! Create an empty queue.
    AddressablePriorityQueue() = default;

    //! Check if queue is empty.
    bool empty() const {
        return container_.empty();
    }

    //! Return size of queue
    size_t size() const {
        return container_.size();
    }

    /*!
     * Insert new element. If the element is already in, its priority is updated.
     *
     * \param e Element to insert.
     * \param o Priority of element.
     * \return pair<handle, bool> Iterator to element; if element was newly inserted.
     */
    std::pair<handle, bool> insert(const Key& e, const Priority& o) {
        std::pair<container_iterator, bool> s = container_.insert(std::make_pair(o, e));
        std::pair<handle, bool> r = handle_map_.insert(std::make_pair(e, s.first));
        if (!r.second && s.second) {
            // was already in with different priority
            container_.erase(r.first->second);
            r.first->second = s.first;
        }
        return r;
    }

    /*!
     * Erase element from the queue.
     *
     * \param e Element to remove.
     * \return If element was in.
     */
    bool erase(const Key& e) {
        handle mi = handle_map_.find(e);
        if (mi == handle_map_.end())
            return false;
        container_.erase(mi->second);
        handle_map_.erase(mi);
        return true;
    }

    //! Erase element from the queue.
    //! \param i Iterator to element to remove.
    void erase(const handle& i) {
        container_.erase(i->second);
        handle_map_.erase(i);
    }

    //! Access top (= minimum) element in the queue.
    const Key& top() const {
        return container_.begin()->second;
    }

    //! Return priority of top (= minimum) element in the queue.
    const Priority& top_priority() const {
        return container_.begin()->first;
    }

    //! Remove top (= minimum) element from the queue.
    //! \return Top element.
    Key pop() {
        assert(!empty());
        const Key e = top();
        handle_map_.erase(e);
        container_.erase(container_.begin());
        return e;
    }
};

template <typename Key, typename Priority,
          typename Cmp = std::less<Priority> >
using addressable_priority_queue =
    AddressablePriorityQueue<Key, Priority, Cmp>;

//! \}

} // namespace cobs

#endif // !COBS_UTIL_ADDRESSABLE_PRIORITY_QUEUE_HEADER

/******************************************************************************/
