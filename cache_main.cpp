#include <iostream>
#include <vector>
#include <sstream>
#include <math.h>
#include <utility>
#include <math.h>
#include <string.h>
#include <algorithm>  

#include "cache_simulator.h"

int main(int argc, char** argv)
{
    if (argc != 7) { // has to be 7 argc, otherwise error
        return 1;
    }
    std::vector<std::pair<char, unsigned>> traces;
    read_traces(traces); // read traces from cin
    // read in argv for contructing the simulator
    std::vector<unsigned> cache_arg; // {set_num, block_per_set, byte_per_block}
    for (int i = 1; i<4; i++) {
        cache_arg.push_back(std::stoi(argv[i])); 
    }
    if (0==strcmp(argv[4], "write-allocate")) {
        cache_arg.push_back(1);
    } else {cache_arg.push_back(0);}
    if (0==strcmp(argv[5], "write-back")) {
        cache_arg.push_back(1);
    } else {cache_arg.push_back(0);}
    if (0==strcmp(argv[6], "lru")) {
        cache_arg.push_back(1);
    } else {cache_arg.push_back(0);}
    std::cout << cache_arg.size() << "\n";
    // intialise the simulator
    // std::cout << cache_arg[0] << cache_arg[1] << cache_arg[2] << cache_arg[3] << cache_arg[4] << cache_arg[5] << "\n";
    cache_simulator simulator(cache_arg[0], cache_arg[1], cache_arg[2], cache_arg[3], cache_arg[4], cache_arg[5]);
    std::cout << "set_num: " << simulator.get_set_num() << " offset: " << simulator.get_idx_bnum() << "\n";
    std::cout << "block_per_set: " << simulator.get_block_per_set() << "\n";
    std::cout << "byte_per_block: " << simulator.get_byte_per_block() << " offset: " << simulator.get_ofst_bnum() << "\n";
    // run the traces
    simulator.process_ops(traces);
    return 0;
}
