/* Author: Mark Faust
 *
 * C version of predictor file
*/

#ifndef PREDICTOR_H_SEEN
#define PREDICTOR_H_SEEN

#include <cstddef>
#include <cstring>
#include <inttypes.h>
#include <vector>
#include "op_state.h"   // defines op_state_c (architectural state) class
#include "tread.h"      // defines branch_record_c class

#define SIZE_1K 1<<10
#define SIZE_4K 1<<12
#define SIZE_512 1<<9

#define PRED_LOCAL false
#define PRED_GLOBAL true

#define TAKEN true
#define NOT_TAKEN false


#define ENTRIES 32   // number of entries in cache
#define INDEX 5       //(logbase2(ENTRIES))    // log2()
#define WAYS 2        // number of ways

int logbase2(int input);

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
};

class PREDICTOR
{
public:
  PREDICTOR();
  bool get_prediction(const branch_record_c* br, const op_state_c* os, uint *predicted_target_address);

  void update_predictor(const branch_record_c* br, const op_state_c* os, bool taken, uint actual_target_address);
private:

  // ALPHA STUFF
  uint16_t alpha_lht[SIZE_1K];  //  local history table (1024*10)/8         = 1280 bytes
  uint16_t alpha_lpt[SIZE_1K];  //  local prediction table (1024*3)/8       = 384 bytes
  uint16_t alpha_gpt[SIZE_4K];  //  global predition table (4096*2)/8       = 1024 bytes
  uint16_t alpha_choice[SIZE_4K];  //  choice predition table (4096*2)/8    = 1024 bytes
                          //                                          total = 3712 bytes
  uint32_t phistory;
  bool pred_choice;
  bool local_prediction;
  bool global_prediction;
  bool final_prediction;
  bool get_local_predict(const branch_record_c* br);
  bool get_global_predict(const branch_record_c* br);

  // returns PRED_LOCAL or PRED GLOBAL indicating whether local or global history should be used

  bool choose_predictor(const branch_record_c* br);
  // TARGET STUFF
  CACHE thecache;
};

// keeps lower n_bits of target
uint32_t keep_lower(uint32_t target, int n_bits);



#endif // PREDICTOR_H_SEEN
