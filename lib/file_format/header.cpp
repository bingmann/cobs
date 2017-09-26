#include "header.hpp"
#include <helpers.hpp>

namespace genome::file_format {
    const std::string header::header_separator = "HEADER";

    std::ostream& operator<<(std::ostream& ost, const header& header) {
        ost << header.header_separator;
        genome::serialize(ost, header.size(), header.type());
        header.serialize_content(ost);
        ost << header.header_separator;
        return ost;
    }

}
