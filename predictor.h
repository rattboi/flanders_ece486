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

class RAS
{
  public:
    RAS(unsigned long maxsize);
    uint32_t pop_ret_pred();
    bool push_call(uint32_t address);

  private:
    std::deque<uint32_t> stack;
    unsigned long stack_size;
};

class PREDICTOR
{
public:
  PREDICTOR();
  bool get_prediction(const branch_record_c* br, const op_state_c* os, uint *predicted_target_address);

  void update_predictor(const branch_record_c* br, const op_state_c* os, bool taken, uint actual_target_address);

private:

  uint16_t lht[SIZE_1K];  //  local history table (1024*10)/8     = 1280 bytes
  uint16_t lpt[SIZE_1K];  //  local prediction table (1024*3)/8   = 384 bytes

  uint16_t gpt[SIZE_4K];  //  global predition table (4096*2)/8   = 1024 bytes

  uint16_t cpt[SIZE_4K];  //  choice predition table (4096*2)/8   = 1024 bytes

  uint16_t phistory;      //  path history (only care about lower 12 bits) = (1.5 bytes)

                          //                                total = 3713.5 bytes = 3.62Kb
  uint32_t btb[SIZE_1K];

  RAS ras;

  bool pred_choice;

  bool local_prediction;
  bool global_prediction;

  bool final_prediction;

  bool get_local_predict(const branch_record_c* br, uint *predicted_target_address);

  bool get_global_predict(const branch_record_c* br, uint *predicted_target_address);

  // returns PRED_LOCAL or PRED GLOBAL indicating whether local or global history should be used
  bool choose_predictor(const branch_record_c* br);

  // keeps lower n_bits of target
  uint32_t keep_lower(uint32_t target, int n_bits);
};

#endif // PREDICTOR_H_SEEN

