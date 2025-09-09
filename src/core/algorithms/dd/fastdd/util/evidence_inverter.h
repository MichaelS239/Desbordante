#pragma once

#include <cstddef>
#include <utility>
#include <vector>

#include <boost/dynamic_bitset.hpp>

namespace algos::dd {

class EvidenceInverter {
private:
    std::vector<boost::dynamic_bitset<>> bitsets_;
    std::size_t df_num_;

    std::vector<std::size_t> CountDFFrequencies() const;

public:
    explicit EvidenceInverter(std::vector<boost::dynamic_bitset<>> bitsets, std::size_t df_num)
        : bitsets_(std::move(bitsets)), df_num_(df_num) {}

    std::vector<boost::dynamic_bitset<>> GetCovers() const;
};

}  // namespace algos::dd
