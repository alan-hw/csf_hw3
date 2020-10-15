#include <iostream>
#include <vector>
#include <sstream>

#include "cache_simulator.h"
cache_simulator::cache_simulator (int n_set_num, int n_block_per_set, int n_dword_per_block, 
                                    int n_evict_type, int is_write_back, int n_is_write_alloc) 
: is_write_alloc{n_is_write_alloc}, write_bt{is_write_back}, evict_type{n_evict_type}, 
set_num{n_set_num}, block_per_set{n_block_per_set}, dword_per_block{n_dword_per_block}
{
    
}

void cache_simulator::load_data (int address)
{
    /*
    tag, ind, offset = get_cache_ind(address)
    // the index of block found with matching tag, negative if not found
    // negative number represents the index for potential eviction.
    block_id = is_hit(ind, tag) 
    // load data //
    if (block_id != -1) { // hit!
        // "load" the data //
        // update metric//
    } else{
        // miss
        block_id = fetch_evict_block(index, tag) // evict the block in question
        // "load" the data //
        // update metric//
    }
    */

}

void cache_simulator::save_data (int address)
{
    /*
    tag, ind, offset = get_cache_ind(address)
    block_id = is_hit(ind, tag)
    // save data //
    if (block_id >= 0) { // hit!
        if (this->write_bt=="write-through") {
            // "save" to memory //
        }
        // "save" to cache//
    } else {
        // miss
        if (! this->is_write_alloc) {
            // "save" to memory
            return
        }
        block_id = fetch_evict_block(index, tag) // evict the block in question
        // "save" to cache //
    }
    */

}
int cache_simulator::get_cac_ind(int addr)
{

}

int cache_simulator::fetch_evict_block(int idx, int block)
{ 
    /* 
    // evict the block pointed to be idx block
    cur_block = this->sets[idx][block]
    // based on strategy (this it matter? are they all just written when flushed)
    // "save" content
    // clean the block for future write
    // return the block // actually not needed
    */

}


int cache_simulator::is_hit(int idx, int tag)
{
    /*
    pot_evict_block = NULL;
    cur_set = this->sets[idx]
    for (int i=0; i<cur_set.size(); ++i) {
        // based on strategy, 
        // update pot_evict_block with potential block to be evicted
        
        // search for matching tag
        if (cur_set[i]."tag" = tag) {
            return i;
        }
    }
    */

}


std::vector<std::pair<char, int>> read_traces(char * trace_string)
{
    // read the traces from trace_string
    // returns a vector of pairs of (load/save, address)
    // load string as stringbuf
    std::stringbuf buffer(trace_string);
    // load stringbuf as input stream
    std::istream trace_in(&buffer);

    std::vector<std::pair<char, int>> output_vec;
    std::pair<char, int> temp_pair; // used to store input temporarily
    int ignore; // ignore the third input in line

    // actually reading values from the stream
    while (trace_in >> temp_pair.first >> temp_pair.second >> ignore) {
        output_vec.push_back(temp_pair);
    }
    return output_vec;
}