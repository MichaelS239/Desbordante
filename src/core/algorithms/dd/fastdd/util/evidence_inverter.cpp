#include "algorithms/dd/fastdd/util/evidence_inverter.h"

namespace algos::dd {

std::vector<std::size_t> EvidenceInverter::CountDFFrequencies() const {
    std::vector<std::size_t> freqs(df_num_);
    for (auto& bitset : bitsets_) {
        for (std::size_t index = bitset.find_first(); index != boost::dynamic_bitset<>::npos;
             index = bitset.find_next(index)) {
            ++freqs[index];
        }
    }
}

std::vector<boost::dynamic_bitset<>> EvidenceInverter::GetCovers() const {}

}  // namespace algos::dd
