/*******************************************************************************
 * cobs/util/string_view.hpp
 *
 * Copyright (c) 2019 Timo Bingmann
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#ifndef COBS_UTIL_STRING_VIEW_HEADER
#define COBS_UTIL_STRING_VIEW_HEADER

#include <iterator>
#include <ostream>
#include <string>

namespace cobs {

/*!
 * string_view - a string view for std::string.
 *
 * based on boost::string_view -- greatly simplified for std::string
 *
 * Copyright (c) Marshall Clow 2012-2015.
 * Copyright (c) Beman Dawes 2015
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 *
 * For more information, see http://www.boost.org
 *
 * Based on the StringRef implementation in LLVM (http://llvm.org) and
 * N3422 by Jeffrey Yasskin
 *   http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2012/n3442.html
 * Updated July 2015 to reflect the Library Fundamentals TS
 *   http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2015/n4480.html
 */
class string_view
{
public:
    // types
    typedef char value_type;
    typedef char* pointer;
    typedef const char* const_pointer;
    typedef char& reference;
    typedef const char& const_reference;
    typedef const_pointer const_iterator;
    typedef const_iterator iterator;
    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
    typedef const_reverse_iterator reverse_iterator;
    typedef std::size_t size_type;
    typedef std::ptrdiff_t difference_type;

    static constexpr size_type npos = size_type(-1);

    //! construct/copy
    string_view() noexcept : ptr_(nullptr), size_(0) { }

    //! copy construction
    string_view(const string_view& rhs) noexcept = default;
    //! assignment
    string_view& operator = (const string_view& rhs) noexcept = default;

    //! assign a whole string
    string_view(const std::string& str) noexcept
        : ptr_(str.data()), size_(str.size()) { }

    //! constructing a string_view from a temporary string is a bad idea
    string_view(std::string&&) = delete;

    string_view(const char* str) : ptr_(str), size_(std::strlen(str)) { }

    string_view(const char* str, size_type size) : ptr_(str), size_(size) { }

    // iterators
    const_iterator begin() const noexcept { return ptr_; }
    const_iterator cbegin() const noexcept { return ptr_; }
    const_iterator end() const noexcept { return ptr_ + size_; }
    const_iterator cend() const noexcept { return ptr_ + size_; }

    const_reverse_iterator rbegin() const noexcept {
        return const_reverse_iterator(end());
    }
    const_reverse_iterator crbegin() const noexcept {
        return const_reverse_iterator(end());
    }
    const_reverse_iterator rend() const noexcept {
        return const_reverse_iterator(begin());
    }
    const_reverse_iterator crend() const noexcept {
        return const_reverse_iterator(begin());
    }

    // capacity
    size_type size() const noexcept { return size_; }
    size_type length() const noexcept { return size_; }
    size_type max_size() const noexcept { return size_; }
    bool empty() const noexcept { return size_ == 0; }

    // element access
    constexpr const_reference operator [] (size_type pos) const noexcept {
        return ptr_[pos];
    }

    constexpr const_reference at(size_t pos) const {
        if (pos >= size_)
            throw std::out_of_range("string_view::at");
        return ptr_[pos];
    }

    const_reference front() const { return ptr_[0]; }
    const_reference back() const { return ptr_[size_ - 1]; }
    const_pointer data() const noexcept { return ptr_; }

    // modifiers
    void clear() noexcept { size_ = 0; }

    void remove_prefix(size_type n) {
        if (n > size_)
            n = size_;
        ptr_ += n;
        size_ -= n;
    }

    void remove_suffix(size_type n) {
        if (n > size_)
            n = size_;
        size_ -= n;
    }

    void swap(string_view& s) noexcept {
        std::swap(ptr_, s.ptr_);
        std::swap(size_, s.size_);
    }

    // string_view string operations
    explicit operator std::string() const {
        return std::string(begin(), end());
    }

    std::string to_string() const { return std::string(begin(), end()); }

    size_type copy(char* s, size_type n, size_type pos = 0) const {
        if (pos > size())
            throw std::out_of_range("string_view::copy");
        size_type rsize = std::min(n, size_ - pos);
        std::copy(data(), data() + rsize, s);
        return rsize;
    }

    string_view substr(size_type pos, size_type n = npos) const {
        if (pos > size())
            throw std::out_of_range("string_view::substr");
        return string_view(data() + pos, (std::min)(size() - pos, n));
    }

    int compare(string_view x) const noexcept {
        const int cmp = std::strncmp(ptr_, x.ptr_, std::min(size_, x.size_));
        return cmp != 0 ? cmp
               : (size_ == x.size_ ? 0 : size_ < x.size_ ? -1 : 1);
    }

    int compare(size_type pos1, size_type n1, string_view x) const noexcept {
        return substr(pos1, n1).compare(x);
    }

    int compare(size_type pos1, size_type n1, string_view x, size_type pos2,
                size_type n2) const {
        return substr(pos1, n1).compare(x.substr(pos2, n2));
    }

    int compare(const char* x) const {
        return compare(string_view(x));
    }

    int compare(size_type pos1, size_type n1, const char* x) const {
        return substr(pos1, n1).compare(string_view(x));
    }

    int compare(
        size_type pos1, size_type n1, const char* x, size_type n2) const {
        return substr(pos1, n1).compare(string_view(x, n2));
    }

    //  searches
    bool starts_with(char c) const noexcept {
        return !empty() && c == front();
    }

    bool starts_with(string_view x) const noexcept {
        return size_ >= x.size_ && std::strncmp(ptr_, x.ptr_, x.size_) == 0;
    }

    bool ends_with(char c) const noexcept {
        return !empty() && c == back();
    }

    bool ends_with(string_view x) const noexcept {
        return size_ >= x.size_ &&
               std::strncmp(ptr_ + size_ - x.size_, x.ptr_, x.size_) == 0;
    }

    //  find
    size_type find(string_view s, size_type pos = 0) const noexcept {
        if (pos > size())
            return npos;
        if (s.empty())
            return pos;
        const_iterator iter =
            std::search(cbegin() + pos, cend(), s.cbegin(), s.cend());
        return iter == cend() ? npos : std::distance(cbegin(), iter);
    }
    size_type find(char c, size_type pos = 0) const noexcept {
        return find(string_view(&c, 1), pos);
    }
    size_type find(const char* s, size_type pos, size_type n) const noexcept {
        return find(string_view(s, n), pos);
    }
    size_type find(const char* s, size_type pos = 0) const noexcept {
        return find(string_view(s), pos);
    }

    //  rfind
    size_type rfind(string_view s, size_type pos = npos) const noexcept {
        if (size_ < s.size_)
            return npos;
        if (pos > size_ - s.size_)
            pos = size_ - s.size_;
        if (s.size_ == 0u) // an empty string is always found
            return pos;
        for (const char* cur = ptr_ + pos; ; --cur) {
            if (std::strncmp(cur, s.ptr_, s.size_) == 0)
                return cur - ptr_;
            if (cur == ptr_)
                return npos;
        }
    }
    size_type rfind(char c, size_type pos = npos) const noexcept {
        return rfind(string_view(&c, 1), pos);
    }
    size_type rfind(const char* s, size_type pos, size_type n) const noexcept {
        return rfind(string_view(s, n), pos);
    }
    size_type rfind(const char* s, size_type pos = npos) const noexcept {
        return rfind(string_view(s), pos);
    }

    //  find_first_of
    size_type find_first_of(string_view s, size_type pos = 0) const noexcept {
        if (pos >= size_ || s.size_ == 0)
            return npos;
        const_iterator iter =
            std::find_first_of(cbegin() + pos, cend(), s.cbegin(), s.cend());
        return iter == cend() ? npos : std::distance(cbegin(), iter);
    }
    size_type find_first_of(char c, size_type pos = 0) const noexcept {
        return find_first_of(string_view(&c, 1), pos);
    }
    size_type find_first_of(const char* s,
                            size_type pos, size_type n) const noexcept {
        return find_first_of(string_view(s, n), pos);
    }
    size_type find_first_of(const char* s, size_type pos = 0) const noexcept {
        return find_first_of(string_view(s), pos);
    }

    //  find_last_of
    size_type find_last_of(string_view s, size_type pos = npos) const noexcept {
        if (s.size_ == 0u)
            return npos;
        if (pos >= size_)
            pos = 0;
        else
            pos = size_ - (pos + 1);
        const_reverse_iterator iter =
            std::find_first_of(crbegin() + pos, crend(), s.cbegin(), s.cend());
        return iter == crend() ? npos : reverse_distance(crbegin(), iter);
    }
    size_type find_last_of(char c, size_type pos = npos) const noexcept {
        return find_last_of(string_view(&c, 1), pos);
    }
    size_type find_last_of(const char* s,
                           size_type pos, size_type n) const noexcept {
        return find_last_of(string_view(s, n), pos);
    }
    size_type find_last_of(const char* s, size_type pos = npos) const noexcept {
        return find_last_of(string_view(s), pos);
    }

    //  find_first_not_of
    size_type find_first_not_of(string_view s,
                                size_type pos = 0) const noexcept {
        if (pos >= size_)
            return npos;
        if (s.size_ == 0)
            return pos;
        const_iterator iter = find_not_of(cbegin() + pos, cend(), s);
        return iter == cend() ? npos : std::distance(cbegin(), iter);
    }
    size_type find_first_not_of(char c, size_type pos = 0) const noexcept {
        return find_first_not_of(string_view(&c, 1), pos);
    }
    size_type find_first_not_of(const char* s, size_type pos,
                                size_type n) const noexcept {
        return find_first_not_of(string_view(s, n), pos);
    }
    size_type find_first_not_of(const char* s,
                                size_type pos = 0) const noexcept {
        return find_first_not_of(string_view(s), pos);
    }

    //  find_last_not_of
    size_type find_last_not_of(string_view s,
                               size_type pos = npos) const noexcept {
        if (pos >= size_)
            pos = size_ - 1;
        if (s.size_ == 0u)
            return pos;
        pos = size_ - (pos + 1);
        const_reverse_iterator iter = find_not_of(crbegin() + pos, crend(), s);
        return iter == crend() ? npos : reverse_distance(crbegin(), iter);
    }
    size_type find_last_not_of(char c, size_type pos = npos) const noexcept {
        return find_last_not_of(string_view(&c, 1), pos);
    }
    size_type find_last_not_of(const char* s,
                               size_type pos, size_type n) const noexcept {
        return find_last_not_of(string_view(s, n), pos);
    }
    size_type find_last_not_of(const char* s,
                               size_type pos = npos) const noexcept {
        return find_last_not_of(string_view(s), pos);
    }

    // ostream
    friend std ::ostream& operator << (std::ostream& os, const string_view& v) {
        os.write(v.data(), v.size());
        return os;
    }

private:
    template <typename r_iter>
    size_type reverse_distance(r_iter first, r_iter last) const noexcept {
        // Portability note here: std::distance is not NOEXCEPT, but calling it
        // with a string_view::reverse_iterator will not throw.
        return size_ - 1 - std::distance(first, last);
    }

    template <typename Iterator>
    Iterator find_not_of(Iterator first, Iterator last, string_view s) const
    noexcept {
        for ( ; first != last; ++first)
            if (0 == std::char_traits<char>::find(s.ptr_, s.size_, *first))
                return first;
        return last;
    }

    const char* ptr_;
    size_type size_;
};

//  Comparison operators
//  Equality
static inline bool operator == (string_view x, string_view y) noexcept {
    if (x.size() != y.size())
        return false;
    return x.compare(y) == 0;
}

//  Inequality
static inline bool operator != (string_view x, string_view y) noexcept {
    if (x.size() != y.size())
        return true;
    return x.compare(y) != 0;
}

//  Less than
static inline bool operator < (string_view x, string_view y) noexcept {
    return x.compare(y) < 0;
}

//  Greater than
static inline bool operator > (string_view x, string_view y) noexcept {
    return x.compare(y) > 0;
}

//  Less than or equal to
static inline bool operator <= (string_view x, string_view y) noexcept {
    return x.compare(y) <= 0;
}

//  Greater than or equal to
static inline bool operator >= (string_view x, string_view y) noexcept {
    return x.compare(y) >= 0;
}

// "sufficient additional overloads of comparison functions"
static inline bool operator == (string_view x, const std::string& y) noexcept {
    return x == string_view(y);
}

static inline bool operator == (const std::string& x, string_view y) noexcept {
    return string_view(x) == y;
}

static inline bool operator == (string_view x, const char* y) noexcept {
    return x == string_view(y);
}

static inline bool operator == (const char* x, string_view y) noexcept {
    return string_view(x) == y;
}

static inline bool operator != (string_view x, const std::string& y) noexcept {
    return x != string_view(y);
}

static inline bool operator != (const std::string& x, string_view y) noexcept {
    return string_view(x) != y;
}

static inline bool operator != (string_view x, const char* y) noexcept {
    return x != string_view(y);
}

static inline bool operator != (const char* x, string_view y) noexcept {
    return string_view(x) != y;
}

static inline bool operator < (string_view x, const std::string& y) noexcept {
    return x < string_view(y);
}

static inline bool operator < (const std::string& x, string_view y) noexcept {
    return string_view(x) < y;
}

static inline bool operator < (string_view x, const char* y) noexcept {
    return x < string_view(y);
}

static inline bool operator < (const char* x, string_view y) noexcept {
    return string_view(x) < y;
}

static inline bool operator > (string_view x, const std::string& y) noexcept {
    return x > string_view(y);
}

static inline bool operator > (const std::string& x, string_view y) noexcept {
    return string_view(x) > y;
}

static inline bool operator > (string_view x, const char* y) noexcept {
    return x > string_view(y);
}

static inline bool operator > (const char* x, string_view y) noexcept {
    return string_view(x) > y;
}

static inline bool operator <= (string_view x, const std::string& y) noexcept {
    return x <= string_view(y);
}

static inline bool operator <= (const std::string& x, string_view y) noexcept {
    return string_view(x) <= y;
}

static inline bool operator <= (string_view x, const char* y) noexcept {
    return x <= string_view(y);
}

static inline bool operator <= (const char* x, string_view y) noexcept {
    return string_view(x) <= y;
}

static inline bool operator >= (string_view x, const std::string& y) noexcept {
    return x >= string_view(y);
}

static inline bool operator >= (const std::string& x, string_view y) noexcept {
    return string_view(x) >= y;
}

static inline bool operator >= (string_view x, const char* y) noexcept {
    return x >= string_view(y);
}

static inline bool operator >= (const char* x, string_view y) noexcept {
    return string_view(x) >= y;
}

} // namespace cobs

#endif // !COBS_UTIL_STRING_VIEW_HEADER

/******************************************************************************/
