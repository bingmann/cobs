//
// Created by Florian Gauger on 09.09.2017.
//

#pragma once

typedef unsigned char byte;

template<class T>
auto operator<<(std::ostream& os, const T& t) -> decltype(t.print(os), os) {
    t.print(os);
    return os;
}
