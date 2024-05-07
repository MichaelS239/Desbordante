#include "algorithms/md/hymd/preprocessing/similarity_measure/immediate_similarity_measure.h"
#include "algorithms/md/hymd/preprocessing/similarity_measure/levenshtein_distance.h"
#include "algorithms/md/hymd/similarity_measure_creator.h"
#include "config/exceptions.h"
#include "model/types/string_type.h"

namespace algos::hymd::preprocessing::similarity_measure {

class LevenshteinSimilarityMeasure : public ImmediateSimilarityMeasure {
    static constexpr auto kName = "levenshtein_similarity";

public:
    class Creator final : public SimilarityMeasureCreator {
        model::md::DecisionBoundary const min_sim_;

    public:
        Creator(model::md::DecisionBoundary min_sim)
            : SimilarityMeasureCreator(kName), min_sim_(min_sim) {
            if (!(0.0 <= min_sim_ && min_sim_ <= 1.0)) {
                throw config::ConfigurationError("Minimum similarity out of range");
            }
        }

        std::unique_ptr<SimilarityMeasure> MakeMeasure() const final {
            return std::make_unique<LevenshteinSimilarityMeasure>(min_sim_);
        }
    };

    LevenshteinSimilarityMeasure(model::md::DecisionBoundary min_sim)
        : ImmediateSimilarityMeasure(
                  std::make_unique<model::StringType>(),
                  [min_sim](std::byte const* l, std::byte const* r) {
                      std::string const& left = model::Type::GetValue<model::String>(l);
                      std::string const& right = model::Type::GetValue<model::String>(r);
                      std::size_t dist = LevenshteinDistance(left, right);
                      std::size_t const max_dist = std::max(left.size(), right.size());
                      Similarity sim =
                              static_cast<double>(max_dist - dist) / static_cast<double>(max_dist);
                      if (sim < min_sim) return kLowestBound;
                      return sim;
                  }) {}
};
}  // namespace algos::hymd::preprocessing::similarity_measure