/* Author: Mark Faust
 *
 * C version of predictor file
*/

#ifndef PREDICTOR_H_SEEN
#define PREDICTOR_H_SEEN

#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <cstring>
#include <inttypes.h>
#include <vector>
#include <deque>
#include "op_state.h"   // defines op_state_c (architectural state) class
#include "tread.h"      // defines branch_record_c class

#define SIZE_1K 1<<10
#define SIZE_4K 1<<12
#define SIZE_512 1<<9

#define PRED_LOCAL false
#define PRED_GLOBAL true

#define TAKEN true
#define NOT_TAKEN false

#define ENTRIES 64   // number of entries in cache
#define INDEX 6       //(logbase2(ENTRIES))    // log2()
#define WAYS 8        // number of ways

class LRU
{
public:
  LRU();
  uint counter[ENTRIES][WAYS];

  void update_all( uint way_accessed, uint32_t index );
  uint get_victim( uint32_t index );
};

class CACHE
{
public:
  CACHE();

  uint data[ENTRIES][WAYS];
  uint tag[ENTRIES][WAYS];
  uint count[ENTRIES];
  bool needs_update;
  bool line_full;

  bool predict(const branch_record_c* br, uint *predicted_target_address);
  bool update(const branch_record_c* br, uint actual_target_address);
  LRU thelru;

};

class RAS
{
  public:
    RAS();
    uint32_t pop_ret_pred();
    bool push_call(uint32_t address);

  private:
    std::deque<uint> stack;
    unsigned long stack_size;
};

class ALPHA
{
  public:
    ALPHA();
    bool get_prediction(const branch_record_c* br);
    void update(const branch_record_c* br, bool taken);

  private:
    bool get_local_predict(const branch_record_c* br);
    bool get_global_predict(const branch_record_c* br);
    // returns PRED_LOCAL or PRED GLOBAL indicating whether local or global history should be used
    bool choose_predictor(const branch_record_c* br);

    uint16_t alpha_lht[SIZE_1K];     //  local history table     (1024*10)/8    = 1280 bytes
    uint16_t alpha_lpt[SIZE_1K];     //  local prediction table  (1024*03)/8    = 384 bytes
    uint16_t alpha_gpt[SIZE_4K];     //  global prediction table (4096*02)/8    = 1024 bytes
    uint16_t alpha_choice[SIZE_4K];  //  choice prediction table (4096*02)/8    = 1024 bytes
    uint32_t phistory;               //  path history                           = 1.5 bytes
                                     //                                   total = 3713.5 bytes
    bool local_prediction;
    bool global_prediction;

    bool final_prediction;

    uint predicted_address;
};

class PREDICTOR
{
public:
  PREDICTOR();
  bool get_prediction(const branch_record_c* br, const op_state_c* os, uint *predicted_target_address);
  void update_predictor(const branch_record_c* br, const op_state_c* os, bool taken, uint actual_target_address);

private:
  
  // ALPHA STUFF
  ALPHA alpha;

  // TARGET STUFF
  CACHE thecache;
  RAS ras;
};

#endif // PREDICTOR_H_SEEN

