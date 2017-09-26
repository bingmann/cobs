#pragma once

#include <string>
#include <ostream>

namespace genome::file_format {
    class header {
    private:
        static const std::string header_separator;
    protected:
        virtual uint32_t type() const = 0;
        virtual uint32_t size() const = 0;
        virtual void serialize_content(std::ostream& stream) const = 0;
    public:
        friend std::ostream& operator<<(std::ostream& ost, const header& header);
    };
}
