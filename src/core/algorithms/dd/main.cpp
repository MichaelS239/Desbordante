#include <iostream>
#include <stdexcept>

#include <boost/dynamic_bitset.hpp>

#include "core/algorithms/dd/fastdd/fastdd.h"
#include "core/algorithms/dd/split/split.h"
#include "core/util/logger.h"

int main(int argc, char** argv) {
    if (argc != 3) std::terminate();
    std::string path = argv[1];
    std::string dif_path = argv[2];
    util::logging::EnsureInitialized();
    LOG_DEBUG("Started");
    algos::dd::FastDD fastdd;
    config::InputTable t = std::make_shared<CSVParser>(path, ',', true);
    config::InputTable dif = std::make_shared<CSVParser>(dif_path, ',', true);
    fastdd.SetOption("table", t);
    fastdd.LoadData();
    fastdd.SetOption("difference_table", dif);
    fastdd.SetOption("num_rows");
    fastdd.SetOption("num_columns");
    fastdd.SetOption("shard_length");
    fastdd.Execute();

    algos::dd::Split split;
    config::InputTable t1 = std::make_shared<CSVParser>(path, ',', true);
    config::InputTable dif1 = std::make_shared<CSVParser>(dif_path, ',', true);
    split.SetOption("table", t1);
    split.LoadData();
    split.SetOption("difference_table", dif1);
    split.SetOption("num_rows");
    split.SetOption("num_columns");
    split.Execute();

    return 0;
}
