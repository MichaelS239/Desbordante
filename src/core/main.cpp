#include <iostream>
#include <stdexcept>

#include <easylogging++.h>

#include "algorithms/dd/split/split.h"

INITIALIZE_EASYLOGGINGPP

int main(int argc, char** argv) {
    if (argc != 2) std::terminate();
    std::string path = argv[1];
    LOG(DEBUG) << "Started";
    algos::dd::Split split;
    config::InputTable t = std::make_shared<CSVParser>(path, ',', true);
    split.SetOption("table", t);
    split.LoadData();
    split.SetOption("difference_table");
    split.SetOption("num_rows");
    split.SetOption("num_columns");
    split.Execute();
    auto const& dds = split.GetDDStringList();
    std::cout << "Found " << dds.size() << " DDs" << std::endl;
    for (auto const& dd : dds) {
        std::cout << dd.ToString() << std::endl;
    }
    return 0;
}