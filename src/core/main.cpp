#include <iostream>
#include <stdexcept>

#include <easylogging++.h>

#include "algorithms/md/hymd/enums.h"
#include "algorithms/md/hymd/hymd.h"
#include "algorithms/md/hymd/preprocessing/similarity_measure/monge_elkan_similarity_measure.h"
#include "algorithms/md/hymd/similarity_data.h"
#include "parser/csv_parser/csv_parser.h"

INITIALIZE_EASYLOGGINGPP

// std::string const kPath =
//         "/home/buyt/Projects/desb_forks/buyt-1/Desbordante/flights.tsv";
//  constexpr char kSeparator = ',';
//  constexpr bool kHasHeader = true;

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

    if (path.find("cora_fix") != std::string::npos) {
        algos::hymd::SimilarityData::Measures column_matches_option;
        std::size_t num_columns = t->GetNumberOfColumns();
        column_matches_option.reserve(num_columns);
        for (model::Index i = 0; i != num_columns; ++i) {
            column_matches_option.push_back(
                    std::make_shared<algos::hymd::preprocessing::similarity_measure::
                                             MongeElkanSimilarityMeasure>(i, i, 0.7));
        }
        hymd.SetOption("column_matches", column_matches_option);
    } else {
        hymd.SetOption("column_matches");
    }

    hymd.Execute();
    auto const& md_list = hymd.MdList();
    std::cout << "Found " << md_list.size() << " MDs" << std::endl;
    if (verbose) {
        for (auto const& md : md_list) {
            std::cout << md.ToStringShort() << std::endl;
        }
    }
    std::cout << std::endl;
    return 0;
}
