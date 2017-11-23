#include <isi/util/parameters.hpp>
#include <cmath>
#include <cassert>

namespace isi {
    double calc_signature_size_ratio(double num_hashes, double false_positive_probability) {
        double denominator = std::log(1 - std::pow(false_positive_probability, 1 / num_hashes));
        double result = -num_hashes / denominator;
        assert(result > 0);
        return result;
    }

    uint64_t calc_signature_size(uint64_t num_elements, double num_hashes, double false_positive_probability) {
        double signature_size_ratio = calc_signature_size_ratio(num_hashes, false_positive_probability);
        double result = std::ceil(num_elements * signature_size_ratio);
        assert(result >= 0);
        assert(result <= UINT64_MAX);
        return (uint64_t) result;
    }

    double calc_average_set_bit_ratio(uint64_t signature_size, double num_hashes, double false_positive_probability) {
        double num_elements = signature_size / calc_signature_size_ratio(num_hashes, false_positive_probability);
        double result = 1 - std::pow(1 - 1 / (double) signature_size, num_hashes * num_elements);
        assert(result >= 0);
        assert(result <= 1);
        return result;
    }
}
