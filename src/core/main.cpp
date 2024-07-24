#include <iostream>
#include <stdexcept>

#include <easylogging++.h>

#include "algorithms/md/hymd/enums.h"
#include "algorithms/md/hymd/hymd.h"
#include "algorithms/md/hymd/lattice/md_lattice.h"
#include "algorithms/md/hymd/preprocessing/similarity_measure/levenshtein_similarity_measure.h"
#include "algorithms/md/hymd/validator.h"
#include "algorithms/md/hymd/lattice/md_lattice.h"
#include "parser/csv_parser/csv_parser.h"

INITIALIZE_EASYLOGGINGPP

// std::string const kPath =
//         "/home/buyt/Projects/desb_forks/buyt-1/Desbordante/flights.tsv";
//  constexpr char kSeparator = ',';
//  constexpr bool kHasHeader = true;

std::string MaxHitToString(auto && arr, std::size_t column_matches) {
    std::stringstream ss;
    for (model::Index i = 0; i != column_matches; ++i) {
        ss << arr[i] << " ";
    }
    return ss.str();
}

int main(int argc, char** argv) {
    if (argc < 5 || argc > 8) std::terminate();
    std::string path = argv[1];
    char separator = argv[2][0];
    bool has_header = argv[3][0] == '1' ? true : false;
    unsigned short num_threads = (unsigned short)std::strtoul(argv[4], NULL, 10);
    bool verbose = false;
    bool no_levels = false;
    bool delete_empty_nodes = true;
    for (int i = 5; i != argc; ++i) {
        if (argv[i][0] == '-') {
            switch (argv[i][1]) {
                case 'v':
                    verbose = true;
                    break;
                case 'l':
                    no_levels = true;
                    break;
                case 'n':
                    delete_empty_nodes = false;
                    break;
                default:
                    break;
            }
        }
    }
    algos::hymd::lattice::delete_empty_nodes = delete_empty_nodes;

    LOG(DEBUG) << "Started";
    algos::hymd::HyMD hymd;
    config::InputTable t = std::make_shared<CSVParser>(path, separator, has_header);
    hymd.SetOption("left_table", t);
    hymd.SetOption("right_table");
    hymd.LoadData();
    hymd.SetOption("min_support");
    hymd.SetOption("prune_nondisjoint");
    hymd.SetOption("max_cardinality");
    if (num_threads)
        hymd.SetOption("threads", num_threads);
    else
        hymd.SetOption("threads");
    if (no_levels) {
        hymd.SetOption("level_definition", +algos::hymd::LevelDefinition::lattice);
    } else {
        hymd.SetOption("level_definition");
    }

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
    std::cout << "Pair inference found not minimal: " << algos::hymd::lattice::pair_inference_not_minimal << std::endl;
    std::cout << "Pair inference found trivial: " << algos::hymd::lattice::pair_inference_trivial << std::endl;
    std::cout << "Pair inference lowered to 0: " << algos::hymd::lattice::pair_inference_lowered_to_zero << std::endl;
    std::cout << "Pair inference lowered to not 0: " << algos::hymd::lattice::pair_inference_lowered_non_zero << std::endl;
    std::cout << "Pair inference no violation discovered: " << algos::hymd::lattice::pair_inference_accepted << std::endl;
    std::cout << std::endl;
    std::cout << "Validations: " << algos::hymd::validations << std::endl;
    std::cout << "Confirmed by validation: " << algos::hymd::confirmed << std::endl;
    std::cout << "Lowered to not 0 during lattice traversal: " << algos::hymd::lattice::traversal_lowered << std::endl;
    std::cout << "Lowered to 0 (deleted) during lattice traversal: " << algos::hymd::lattice::traversal_deleted << std::endl;
    std::cout << "Unsupported: " << algos::hymd::unsupported << std::endl;
    std::cout << std::endl;
    std::cout << "Total interestingness CCV ID searches: " << algos::hymd::lattice::get_interestingness_ccv_ids_called << std::endl;
    std::cout << "Max CCV IDs detected for all column matches during raising: " << algos::hymd::lattice::raising_stopped << std::endl;
    std::cout << "Started interestingness bound search with max bounds for all column matches: " << algos::hymd::lattice::interestingness_stopped_immediately << std::endl;
    std::cout << "Interestingness CCV ID requested for every column match: " << MaxHitToString(algos::hymd::lattice::interestingness_indices_requested, algos::hymd::lattice::column_matches_size) << std::endl;
    std::cout << "Max CCV ID hit for every column match: " << MaxHitToString(algos::hymd::lattice::interestingness_indices_hit, algos::hymd::lattice::column_matches_size) << std::endl;
    std::cout << "Started with max CCV ID for every column match: " << MaxHitToString(algos::hymd::lattice::interestingness_indices_max_started, algos::hymd::lattice::column_matches_size) << std::endl;
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
