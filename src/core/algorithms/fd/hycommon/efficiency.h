#pragma once

#include <cstddef>

namespace algos::hy {

class Efficiency {
private:
    size_t column_id_;

    unsigned num_violations_ = 0;
    unsigned num_comparisons_ = 0;
    unsigned parameter_ = 0;

public:
    explicit Efficiency(size_t column_id) noexcept : column_id_(column_id) {}

    [[nodiscard]] double CalcEfficiency() const noexcept {
        if (num_comparisons_ == 0) {
            return 0.0;
        }

        return 1.00 * num_violations_ / num_comparisons_;
    }

    void IncrementParameter() noexcept {
        parameter_++;
    }

    void SetComparisons(unsigned num_new_comparisons) noexcept {
        num_comparisons_ = num_new_comparisons;
    }

    void SetViolations(unsigned num_new_violations) noexcept {
        num_violations_ = num_new_violations;
    }

    [[nodiscard]] size_t GetAttr() const noexcept {
        return column_id_;
    }

    [[nodiscard]] unsigned GetParameter() const noexcept {
        return parameter_;
    }

    bool operator<(Efficiency const& other) const noexcept {
        return CalcEfficiency() < other.CalcEfficiency();
    }
};

}  // namespace algos::hy
