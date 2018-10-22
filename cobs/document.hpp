/*******************************************************************************
 * cobs/document.hpp
 *
 * Copyright (c) 2018 Florian Gauger
 * Copyright (c) 2018 Timo Bingmann
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#ifndef COBS_DOCUMENT_HEADER
#define COBS_DOCUMENT_HEADER

#include <cobs/kmer.hpp>

#include <algorithm>
#include <ostream>
#include <vector>

namespace cobs {

template <unsigned int N>
class document
{
private:
    std::vector<kmer<N> > m_data;

public:
    void print(std::ostream& ostream) const;
    std::vector<kmer<N> >& data() {
        return m_data;
    }
    const std::vector<kmer<N> >& data() const {
        return m_data;
    }

    void sort_kmers() {
        std::sort(m_data.begin(), m_data.end());
    }
};

template <unsigned int N>
void document<N>::print(std::ostream& ostream) const {
    for (size_t i = 0; i < m_data.size(); i++) {
        ostream << m_data[i] << std::endl;
    }
}

} // namespace cobs

#endif // !COBS_DOCUMENT_HEADER

/******************************************************************************/
