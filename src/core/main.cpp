#include <iostream>
#include <stdexcept>

#include <boost/dynamic_bitset.hpp>
#include <easylogging++.h>

#include "algorithms/dd/fastdd/fastdd.h"

INITIALIZE_EASYLOGGINGPP

/*boost::dynamic_bitset<> Reverse(boost::dynamic_bitset<>&& bitset) {
    std::size_t const bitset_size = bitset.size();
    boost::dynamic_bitset<> reversed(std::move(bitset));
    for (std::size_t i = 0; i != bitset_size / 2; ++i) {
        bool temp = reversed[i];
        reversed[i] = reversed[bitset_size - 1 - i];
        reversed[bitset_size - 1 - i] = temp;
    }

    return reversed;
}*/

int main(int argc, char** argv) {
    if (argc != 3) std::terminate();
    std::string path = argv[1];
    std::string dif_path = argv[2];
    LOG(DEBUG) << "Started";
    algos::dd::FastDD fastdd;
    config::InputTable t = std::make_shared<CSVParser>(path, ',', true);
    config::InputTable op = std::make_shared<CSVParser>(dif_path, ',', true);
    fastdd.SetOption("table", t);
    fastdd.LoadData();
    fastdd.SetOption("operator_difference_table", op);
    fastdd.SetOption("num_rows");
    fastdd.SetOption("num_columns");
    fastdd.SetOption("shard_length");
    fastdd.Execute();

    /*LOG(INFO) << sizeof(boost::dynamic_bitset<>);
    boost::dynamic_bitset<> a(64, false);
    LOG(INFO) << sizeof(a);
    LOG(INFO) << sizeof(std::optional<boost::dynamic_bitset<>>);
    LOG(INFO) << sizeof(std::bitset<64>);*/

    /*boost::dynamic_bitset<> a(10, false);
    boost::dynamic_bitset<> b(10, false);
    a[5] = true;
    b[6] = true;
    b[4] = true;
    a[6] = true;
    LOG(INFO) << a;
    LOG(INFO) << b;
    if (a < b) {
        LOG(INFO) << "a<b";
    } else {
        LOG(INFO) << "a>b";
    }
    boost::dynamic_bitset<> const reversed_a = Reverse(std::move(a));
    boost::dynamic_bitset<> const reversed_b = Reverse(std::move(b));
    LOG(INFO) << reversed_a;
    LOG(INFO) << reversed_b;
    if (reversed_a < reversed_b) {
        LOG(INFO) << "a<b";
    } else {
        LOG(INFO) << "a>b";
    }*/

    return 0;
}