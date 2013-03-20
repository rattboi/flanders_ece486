/* Author: Eric Krause & Bradon Kanyid
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
#include <deque>
#include "op_state.h"   // defines op_state_c (architectural state) class
#include "tread.h"      // defines branch_record_c class

#define SIZE_1K (1<<10)
#define SIZE_4K (1<<12)

#define PHISTORY_BITS 12
#define LHISTORY_BITS 10

#define PRED_LOCAL false
#define PRED_GLOBAL true

#define TAKEN true
#define NOT_TAKEN false

#define M_ENTRIES 64    // number of entries in main cache
#define M_WAYS 8        // number of ways

#define V_ENTRIES 8     // number of entries in victim cache
#define V_WAYS 8        // number of ways 

#define RAS_ENTRIES 33  // max number of elements in RAS stack

#define ALPHA_LOCAL_INIT  5
#define ALPHA_GLOBAL_INIT 1
#define ALPHA_CHOICE_INIT 1

#define SAT_LOCAL  3
#define SAT_GLOBAL 2
#define SAT_PRED   2

#define SAT_LOCAL_MIN  0
#define SAT_GLOBAL_MIN 0
#define SAT_PRED_MIN   0

#define SAT_LOCAL_MAX  7
#define SAT_GLOBAL_MAX 3
#define SAT_PRED_MAX   3

using namespace std;

class LRU
{
public:
  LRU(uint entries, uint ways);
  ~LRU();
  void update_all( uint way_accessed, uint32_t index );
  uint get_victim( uint32_t index );

private:
  uint **counter; //dynamic array of size entries * ways
  int entries;
  int ways;
};

class CACHE
{
public:
  CACHE(uint entries, uint ways);
  ~CACHE();
  bool predict(const uint32_t addr_idx, uint *predicted_target_address);
  bool update(const uint32_t addr_idx, uint actual_target_address, uint *evicted_tag, uint *evicted_data);
  bool needs_update();

private:
  uint **data;          //dynamic array of size entries * ways
  uint **tag;           //dynamic array of size entries * ways
  bool b_needs_update;

  int entries;
  int ways;
  int idx_bits;

  LRU *lru;             //separate object to abstract LRU functionality from cache functionality
};

class RAS
{
public:
  RAS();
  uint32_t pop_ret_pred();
  bool push_call(uint32_t address);

private:
  deque<uint>   stack;
  unsigned long stack_size;
};

class ALPHA
{
public:
  ALPHA();
  bool get_prediction(const branch_record_c* br);
  void update(const branch_record_c* br, bool taken);

private:
  bool get_local_predict(const uint32_t address);
  bool get_global_predict();
  bool choose_predictor();  // returns PRED_LOCAL or PRED GLOBAL indicating whether local or global history should be used

  uint16_t alpha_lht[SIZE_1K];     //  local history table     (1024*10)/8    = 1280 bytes
  uint16_t alpha_lpt[SIZE_1K];     //  local prediction table  (1024*03)/8    = 384 bytes
  uint16_t alpha_gpt[SIZE_4K];     //  global prediction table (4096*02)/8    = 1024 bytes
  uint16_t alpha_choice[SIZE_4K];  //  choice prediction table (4096*02)/8    = 1024 bytes
  uint32_t phistory;               //  path history                           = 1.5 bytes
                                   //                                   total = 3713.5 bytes
  bool local_prediction;
  bool global_prediction;

  bool final_prediction;
};

class PREDICTOR
{
public:
  PREDICTOR();
  ~PREDICTOR();
  bool get_prediction(const branch_record_c* br, const op_state_c* os, uint *predicted_target_address);
  void update_predictor(const branch_record_c* br, const op_state_c* os, bool taken, uint actual_target_address);

private:
  // Branch Predictor object
  ALPHA alpha;

  // Branch Target Predictor objects
  CACHE *maincache;
  CACHE *victimcache;
  RAS ras;
};

//helper functions
uint32_t keep_lower(uint32_t target, int n_bits);
int logbase2(int input);

#endif // PREDICTOR_H_SEEN

