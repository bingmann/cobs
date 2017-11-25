#pragma once

#include <string>

namespace isi {
    void print_errno(const std::string& msg);
    void exit_error(const std::string& msg);
    void assert_exit(bool cond, const std::string& msg);
    void exit_error_errno(const std::string& msg);
    template<class E>
    void assert_throw(bool cond, const std::string& msg);
}


namespace isi {
    template<class E>
    void assert_throw(bool cond, const std::string& msg) {
        if (!cond) {
            throw E(msg);
        }
    }
}
