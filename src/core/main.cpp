#include <iostream>
#include <stdexcept>

#include <easylogging++.h>

#include "algorithms/md/hymd/hymd.h"
#include "algorithms/md/hymd/lattice_traverser.h"
#include "algorithms/md/hymd/preprocessing/similarity_measure/levenshtein_similarity_measure.h"
#include "algorithms/md/hymd/record_pair_inferrer.h"
#include "algorithms/md/hymd/validator.h"
#include "parser/csv_parser/csv_parser.h"

INITIALIZE_EASYLOGGINGPP

// std::string const kPath =
//         "/home/buyt/Projects/desb_forks/buyt-1/Desbordante/flights.tsv";
//  constexpr char kSeparator = ',';
//  constexpr bool kHasHeader = true;

int main(int argc, char** argv) {
    if (argc < 4 || argc > 5) std::terminate();
    std::string path = argv[1];
    char separator = argv[2][0];
    bool has_header = argv[3][0] == '1' ? true : false;
    bool verbose = false;
    for (int i = 4; i != argc; ++i) {
        if (argv[i][0] == '-' && argv[i][1] == 'v') {
            verbose = true;
            break;
        }
    }

    LOG(DEBUG) << "Started";
    algos::hymd::HyMD hymd;
    config::InputTable t = std::make_shared<CSVParser>(path, separator, has_header);
    hymd.SetOption("left_table", t);
    hymd.SetOption("right_table");
    hymd.LoadData();
    hymd.SetOption("min_support");
    hymd.SetOption("prune_nondisjoint");

    /*
    std::vector<std::tuple<std::string, std::string,
                           std::shared_ptr<algos::hymd::SimilarityMeasureCreator>>>
            col_matches{
                    {"2", "2",
                     std::make_shared<algos::hymd::preprocessing::similarity_measure::
                                              LevenshteinSimilarityMeasure::Creator>(0.45)},
                    {"3", "3",
                     std::make_shared<algos::hymd::preprocessing::similarity_measure::
                                              LevenshteinSimilarityMeasure::Creator>(0.45)},
            };
    hymd.SetOption("column_matches", col_matches);
    */

    hymd.SetOption("column_matches");
    hymd.Execute();
    auto const& md_list = hymd.MdList();
    std::cout << "Pair inference found not minimal: " << algos::hymd::pair_inference_not_minimal
              << std::endl;
    std::cout << "Pair inference found trivial: " << algos::hymd::pair_inference_trivial
              << std::endl;
    std::cout << "Pair inference lowered to 0: " << algos::hymd::pair_inference_lowered_to_zero
              << std::endl;
    std::cout << "Pair inference lowered to not 0: " << algos::hymd::pair_inference_lowered_non_zero
              << std::endl;
    std::cout << "Pair inference no violation discovered: " << algos::hymd::pair_inference_accepted
              << std::endl;
    std::cout << std::endl;
    std::cout << "Validations: " << algos::hymd::validations << std::endl;
    std::cout << "Confirmed by validation: " << algos::hymd::confirmed << std::endl;
    std::cout << "Lowered to not 0 during lattice traversal: "
              << algos::hymd::traversal_lowered_to_non_zero << std::endl;
    std::cout << "Lowered to 0 during lattice traversal: " << algos::hymd::traversal_lowered_to_zero
              << std::endl;
    std::cout << "Unsupported: " << algos::hymd::unsupported << std::endl;
    std::cout << std::endl;
    std::cout << "Found " << md_list.size() << " MDs" << std::endl;
    if (verbose) {
        for (auto const& md : md_list) {
            std::cout << md.ToStringShort() << std::endl;
        }
    }
    std::cout << std::endl;
    return 0;
}
