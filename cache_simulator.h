#ifndef CACHE_SIMULATOR_H
#define CACHE_SIMULATOR_H

#include <iostream>
#include <vector>
#include <map>
#include <utility>

// or a map will just suffice
// TODO fininish the metric "skeleton"
struct metric {
    public:
        std::pair<int, int> cac_op_num;  // save, load
        std::pair<int, int> mem_op_num;  // save, load
        std::pair<int, int> hit_miss; // hit, miss?
        int tot_cycle;
};

struct set {
    public:
        std::vector<block> blocks;
};

struct block {
    // TODO: properly select the block type to account for various valid bit/address/etc
    public:
        std::vector<std::pair<int, int>> content_tags;
};

class cache_simulator {
    private:
        int is_write_alloc; // 1 = write-allocate, 0 = no-write-allocate
        int write_bt; // 1 = write-back, 0 = write-trhough
        int evict_type; // LRU/FIFO
        int set_num; // max # of sets in cache
        int block_per_set; // # of block per set
        int dword_per_block; // # of 4-byte per block
        std::vector<set> sets; // a vector of sets.
        metric sim_metric;

    public:
    cache_simulator (int n_set_num, int n_block_per_set, int n_dword_per_block, 
                    int n_evict_type, int is_write_back, int n_is_write_alloc) 
    : is_write_alloc{n_is_write_alloc}, write_bt{is_write_back}, evict_type{n_evict_type}, 
    set_num{n_set_num}, block_per_set{n_block_per_set}, dword_per_block{n_dword_per_block}
    {
        
    }
    void load_data (int addr);
    void save_data (int addr);
    // given a memory address, returns the corresponding set index, tag, etc
    int get_cac_ind(int addr);
    // given a set index and block id, evict the block
    int fetch_evict_block(int idx, int block);
    // given a set index and tag, returns true if it is a hit.
    int is_hit(int idx, int tag);
};

std::vector<std::pair<char, int>> read_traces();
#endif