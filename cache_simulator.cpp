#include <iostream>
#include <vector>
#include <sstream>
#include <math.h>
#include <map>
#include <utility>
#include <math.h>
#include <algorithm>  

#include "cache_simulator.h"
cache_simulator::cache_simulator(unsigned int n_set_num, unsigned int n_block_per_set, unsigned int n_byte_per_block, 
                    int n_evict_type, int is_write_back, int n_is_write_alloc) 
    : is_write_alloc{n_is_write_alloc}, 
    write_bt{is_write_back}, 
    evict_type{n_evict_type}, 
    set_num{n_set_num}, 
    block_per_set{n_block_per_set}, 
    byte_per_block{n_byte_per_block},
    idx_bnum{0}, 
    ofst_bnum{0}
{
  this->sim_metric.tot_cycle = 0;
    while (n_set_num >>= 1) ++idx_bnum; // integer log2(), set_num and ofst_bnum assumed != 0
    while (n_byte_per_block >>= 1) ++ofst_bnum; // integer log2()
    // INITIALISE DATA with empty values, signified by -1 at for block tags
    block empty_block;
    empty_block.content = std::make_pair((long)-1, (int)0); // fill the empty block, -1 is for empty
    set empty_set;
    empty_set.blocks.assign(this->block_per_set, empty_block); // fill set with empty block
    empty_set.stat=-1; // initialise the to prevent initial 0 being treated as a real stat.
    this->sets.assign(this->set_num, empty_set); // fill simulator with empty sets

    // For FIFO, the counters should be initialisd from block_per_set to 0, descending order
    for (unsigned i=0; i <set_num; ++i) {
        for (unsigned j=0; j<block_per_set; ++j) {
            this->sets[i].blocks[j].content.second = block_per_set-i-1; // so first block has block_per_set-1, last has 0
	    this->sets[i].blocks[j].is_dirty = 0;
        }
    }
}

void cache_simulator::load_data (struct_addr addr)
{
    // PART 1: CHECK IF HIT, CALCULATE METRIC FOR PREPARING CACHE BLOCK
    std::pair<int, int> hit_stat = this->fetch_evict_block(addr, 0); // check if it is a hit, 0 for load
    if (hit_stat.second != 1) {
        // a miss
        ++this->sim_metric.load_hm.second; // update load miss
        if (hit_stat.second == 0) {
            // the block is not clean,
            // the current content is first saved
            // then the entire block is loaded, so write/load twice
            this->sim_metric.tot_cycle += ((this->byte_per_block * 25) << 1); 
            this->sim_metric.mem_op_num.second += (this->byte_per_block >> 1); // total byte / 4 * 2
        } else {
            // the block is clean.
            // the current content is first saved
            // then the entire block is loaded
            this->sim_metric.tot_cycle += this->byte_per_block * 25; // the current cache content is saved, then entire block is loaded
            this->sim_metric.mem_op_num.second += (this->byte_per_block >> 2); // total byte / 4
        }
    } else{
        // a hit
        ++this->sim_metric.load_hm.first; // update save hit hit
    }

    // PART 2: ACTUALLY LOAD DATA TO CPU
    ++this->sim_metric.cac_op_num.second; // inc cache operation count
    ++this->sim_metric.tot_cycle; // inc cycle count by 1, required for loading
}

void cache_simulator::save_data (struct_addr addr)
{
    // PART 1: CHECK IF HIT, CALCULATE METRIC FOR PREPARING CACHE BLOCK
    std::pair<int, int> hit_stat = this->fetch_evict_block(addr, 1); // check if it is a hit, 1 for save
    if (hit_stat.second != 1 && this->is_write_alloc == 1) {
        // miss and write allocate -> if no_write_allocate, then no change to cache block
        ++this->sim_metric.save_hm.second; // update save miss
        if (hit_stat.second == 0) {
            // the block is not clean,
            // the current content is first saved
            // then the entire block is loaded, so write/load twice
	  this->sim_metric.tot_cycle += ((this->byte_per_block * 25) << 1); 
            this->sim_metric.mem_op_num.first += (this->byte_per_block >> 1); // total byte / 4 * 2
        } else {
            // the block is clean.
            // the current content is first saved
            // then the entire block is loaded
            this->sim_metric.tot_cycle += this->byte_per_block * 25; // the current cache content is saved, then entire block is loaded
            this->sim_metric.mem_op_num.first += (this->byte_per_block >> 2); // total byte / 4
        }
    } else if (hit_stat.second == 1) {
        // hit, write_allocate doesn't matter
        ++this->sim_metric.save_hm.first; // update save hit
    }

    // PART 2: ACTUALLY LOAD DATA FROM CACHE TO CPU
    if (this->is_write_alloc==0) {
        // no_write_allocate, directly load from memory -> can be integrated to part 1
        this->sim_metric.tot_cycle += 100; // 4 byte saved to memory
        ++this->sim_metric.mem_op_num.first; // 1 operation, 4 bytes saved

    } else {
        ++this->sim_metric.cac_op_num.second; // inc cache operation count
        ++this->sim_metric.tot_cycle; // inc cycle count by 1, required for loading
        if (this->write_bt==0) {
            // if write through, there is addition need to save memory
            this->sim_metric.tot_cycle += 100; // 4 byte saved to memory
            ++this->sim_metric.mem_op_num.first; // increment memory operation
        }
    }
}

struct_addr cache_simulator::get_struct_addr(unsigned raw_addr)
{
    // This method shouldn't really be used as it may be a bottle neck?
    struct_addr output;
    // Determine the bits for offset -> actually never used
    unsigned offset = raw_addr & ((1 << this->ofst_bnum) - 1);
    // Determine the bits for index
    unsigned index = (raw_addr & (((1 << this->idx_bnum) - 1) << this->ofst_bnum)) >> this->ofst_bnum;
    // Determine the bits for tag
    unsigned tag = (raw_addr & ~((1 << (this->idx_bnum + this->ofst_bnum)) - 1)) >> (this->idx_bnum + this->ofst_bnum);
    output.tag = tag; 
    output.index = index;
    output.offset = offset;
    return output;
}

std::pair<int, int> cache_simulator::fetch_evict_block(struct_addr addr, int op_type)
{
  set cur_set = this->sets[addr.index];
  std::pair<int, int> output(0, 0);
  std::vector<block>::iterator targ_it = std::find_if(cur_set.blocks.begin(), cur_set.blocks.end(), [&] (const block &o) {
      return o.content.first == addr.tag;
    });
    if (targ_it != cur_set.blocks.end()) {
      output.second = 1; // hit
    }
    else{
      //locate a cache block to use
      //step 1 find if there is any empty
      targ_it = std::find_if(cur_set.blocks.begin(), cur_set.blocks.end(), [&] (const block &o) {
	  return o.content.first == -1; 
	});
      //step 2 if there is no empty, find one to evict
      //we should evict maximum of timestamps
      if (targ_it == cur_set.blocks.end()) {
	targ_it = std::max_element(cur_set.blocks.begin(), cur_set.blocks.end(), [] (const block &a, const block &b) {
            return a.content.second < b.content.second;
	  });
      }
    }
    
    // convert iterator to index, update hit_status
    output.first = targ_it - cur_set.blocks.begin();
    if(cur_set.blocks[output.first].is_dirty){
      output.second = 0;
    }
    else{
      output.second = -1;
    }
    //output.second = 1 "hit", 0 "dirty miss", -1 "clean miss"

    // if a block is load miss, mark it clean
    if(op_type==0 && output.second!=1){
     cur_set.blocks[output.first].is_dirty = 0;
    }
    // if a write occurs, make it dirty

    if(op_type==1){
      cur_set.blocks[output.first].is_dirty = 1;
    }

    //update the block tag
    if (!(output.second != 1 && op_type == 0 && this->is_write_alloc == 0)) {
        this->sets[addr.index].blocks[output.first].content.first = addr.tag; 
    }
    //update timestamp
    //LRU
    if(this->evict_type==1){
      //miss: set to 0 and increase other counters
      //or Hit least recently used: set to 0 and increase other counters
      if(output.second != 1 || cur_set.blocks[output.first].content.second == (this->block_per_set-1)){
	for (std::vector<block>::iterator it=this->sets[addr.index].blocks.begin(); it!=this->sets[addr.index].blocks.end(); ++it) {
	  ++it->content.second;
	  it->content.second %= this->block_per_set;   
	}
        cur_set.blocks[output.first].content.second = 0; 
      }
      //Hit others: increase some counters
      else if(cur_set.blocks[output.first].content.second != 0){
	for (std::vector<block>::iterator it=this->sets[addr.index].blocks.begin(); it!=this->sets[addr.index].blocks.end(); ++it) {
	  if(it->content.second<cur_set.blocks[output.first].content.second){
	    ++it->content.second;
	    it->content.second %= this->block_per_set; 
	  }
	}
	cur_set.blocks[output.first].content.second = 0; 
      }
    }
    //FIFO
    else{
      	for (std::vector<block>::iterator it=this->sets[addr.index].blocks.begin(); it!=this->sets[addr.index].blocks.end(); ++it) {
	  ++it->content.second;
	  it->content.second %= this->block_per_set;   
	}
    }

    return output;
}
  
    // /* Fetch the block */
    // set cur_set = this->sets[addr.index]; // fetch current indx
    // std::pair<int, int> output(0, 0); // output pair <index_of_block, hit_status>
    // // hit_status: 1=hit, 0=miss, -1=miss but empty
    
    // // Try find any tag matching block
    // std::vector<block>::iterator targ_it = std::find_if(cur_set.blocks.begin(), cur_set.blocks.end(), [&] (const block &o) {
    //     return o.content.first == addr.tag; // block tag equals to desired tag.
    // });
    // // If not found, find any empty block
    // if (targ_it != cur_set.blocks.end()) {
    //     output.second = 1; // hit
    // } else {
    //     targ_it = std::find_if(cur_set.blocks.begin(), cur_set.blocks.end(), [&] (const block &o) {
    //         return o.content.first == -1; // block is empty
    //     });
    //     output.second = -1; // empty miss
    // }
    // // If still not found, find minimum counter
    // if (targ_it == cur_set.blocks.end()) {
    //     targ_it = std::min_element(cur_set.blocks.begin(), cur_set.blocks.end(), [] (const block &a, const block &b) {
    //         return a.content.second < b.content.second; // fist block counter smaller than second?
    //     });
    //     output.second = 0; // filled miss
    // }
    // // convert iterator to index, update hit_status
    // output.first = targ_it - cur_set.blocks.begin();

    // /* Evict/Update the block */
    // // If (!(miss && save && no_write_alloc))) -> update (else no change)
    // if (!(output.second != 1 && op_type == 0 && this->is_write_alloc == 0)) {
    //     this->sets[addr.index].blocks[output.first].content.first = addr.tag; // update tag
    //     this->sets[addr.index].stat = addr.tag; // update 
    //     // if (!(LRU && (cur_tag == prev_tag))):
    //     if (!(this->evict_type==0 && (addr.tag==cur_set.stat))) {
    //         if (!(this->evict_type==1 && output.second<1)) {
    //             // if (not (miss && FIFO)) -> 
    //             // increment all counters (if FIFO, mod the counters w/ block_num)
    //             for (std::vector<block>::iterator it=this->sets[addr.index].blocks.begin(); 
    //                 it!=this->sets[addr.index].blocks.end(); ++it) {
    //                     ++it->content.second;
    //                     if(this->evict_type == 1) {
    //                         // For FIFO, mod the result
    //                         it->content.second %= this->block_per_set;
    //                     }
    //                 }
    //         }
            
    //         if (this->evict_type==0) {
    //             // if LRU reset the counter for new hit LRU
    //             this->sets[addr.index].blocks[output.first].content.second=0; 
    //         }
    //     } 
        
    // }
    // return output;
//}

void cache_simulator::process_ops(std::vector<std::pair<char, unsigned>>& ops)
{
    struct_addr cur_addr;
    // process the operators
    for(unsigned i=0; i < ops.size(); ++i) {
        // for each operator
      
        cur_addr = this->get_struct_addr(ops[i].second);
	//std::cout<<"index: "<< cur_addr.index <<"tag: "<< cur_addr.tag <<std::endl;
	//this->print_metrics();
        if(ops[i].first == 's') {
            this->save_data(cur_addr); // save
        } else {
            this -> load_data(cur_addr); // load
        }
	
    }
    this->print_metrics();
}

void cache_simulator::print_metrics()
{
    int load_num = this->sim_metric.load_hm.first + this->sim_metric.load_hm.second;
    int save_num = this->sim_metric.save_hm.first + this->sim_metric.save_hm.second;

    std::cout << "Total loads: " << load_num << "\n"
    << "Toatal stores: " << save_num << "\n"
    << "Load hits: " << this->sim_metric.load_hm.first << "\n"
    << "Load misses: " << this->sim_metric.load_hm.second << "\n"
    << "Store hits: " << this->sim_metric.save_hm.first << "\n"
    << "Store misses: " << this->sim_metric.save_hm.second << "\n"
    << "Total cycles: " << this->sim_metric.tot_cycle << "\n";
}

void cache_simulator::restart_cache()
{
    // TODO implement this, which re-initialise the simulator and cache
}

metric cache_simulator::get_metrics()
{
    return this->sim_metric;
}

void cache_simulator::print_cache()
{
    for (unsigned i=0; i<this->set_num; ++i) {
    }
}

unsigned cache_simulator::get_set_num()
{
    return this->set_num;
}

unsigned cache_simulator::get_block_per_set()
{
    return this->block_per_set;
}

unsigned cache_simulator::get_byte_per_block()
{
    return this->byte_per_block;
}

unsigned cache_simulator::get_idx_bnum()
{
    return this->idx_bnum;
}

unsigned cache_simulator::get_ofst_bnum()
{
    return this->ofst_bnum;
}

int read_traces(std::vector<std::pair<char, unsigned>>& vec_buffer)
{
    // read the traces from std::cin
    // returns a vector of pairs of (load/save, address) (<char>'l'/'s', <unsigned>addr)

    std::pair<char, unsigned> temp_pair; // used to store input temporarily
    int ignore; // ignore the third input in line
    
    // actually reading values from the stream
    while ((std::cin >> temp_pair.first >> std::hex >> temp_pair.second >> ignore)) {
        vec_buffer.push_back(temp_pair);
        // std::cout << "type " << output_vec[i].first << " addr " << std::hex << output_vec[i].second << "\n";
    }
    return 0;
}

//check if input args is a positive power of two
int check_power_of_two(int num){
  if(num <= 0){
    return 0;
  }
  else{
    return (num & (num - 1)) == 0;
  }
}
