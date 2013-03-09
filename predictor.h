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

#define PRED_LOCAL false
#define PRED_GLOBAL true

class PREDICTOR
{
public:
  PREDICTOR();
  bool get_prediction(const branch_record_c* br, const op_state_c* os, uint *predicted_target_address);

  void update_predictor(const branch_record_c* br, const op_state_c* os, bool taken, uint actual_target_address);

private:

  uint16_t lht[SIZE_1K]; //  local history table
  uint16_t lpt[SIZE_1K]; //  local prediction table

  uint16_t gpt[SIZE_4K]; //  global predition table

  uint16_t cpt[SIZE_4K]; //  choice predition table

  uint16_t phistory;     //  path history (only care about lower 12 bits)

  bool pred_choice;
  bool local_prediction;
  bool global_prediction;
  bool final_prediction;

  bool get_local_predict(const branch_record_c* br, uint *predicted_target_address);

  bool get_global_predict(const branch_record_c* br, uint *predicted_target_address);

  bool choose_predictor(const branch_record_c* br);
  // returns PRED_LOCAL or PRED GLOBAL indicating whether local or global history should be used

  uint16_t keep_lower(uint16_t target, int n_bits);
  // keeps lower n_bits of target
};
#endif // PREDICTOR_H_SEEN

