#pragma once

#include <cstdint>
#include <fstream>
#include <ctime>

namespace isi {
    uint64_t get_page_size();

    template<class T>
    auto operator<<(std::ostream& os, const T& t) -> decltype(t.print(os), os);
    std::string random_sequence(size_t len, size_t seed = std::time(0));
    void initialize_map();
    void initialize_map_server();
}


namespace isi {
    template<class T>
    auto operator<<(std::ostream& os, const T& t) -> decltype(t.print(os), os) {
        t.print(os);
        return os;
    }
}
