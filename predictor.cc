#include "predictor.h"
#define PC (br->instruction_addr)
#define PC10 (keep_lower(br->instruction_addr,10))
#define NEXT (br->instruction_next_addr)

PREDICTOR::PREDICTOR()
{
  for (int i = 0; i < SIZE_1K; i++)
  {
    alpha_lht[i] = 0;
    alpha_lpt[i] = 0;
  }

  for (int i = 0; i < SIZE_4K; i++)
  {
    alpha_gpt[i] = 0;
    alpha_choice[i] = 2;
  }
  phistory = 0;

}

bool PREDICTOR::get_local_predict(const branch_record_c* br, uint *predicted_target_address)
{
  uint16_t alpha_lht_ind = PC10;
  uint16_t curr = alpha_lht[alpha_lht_ind];
  uint16_t pred_bits = alpha_lpt[curr];

  return (pred_bits & 4)>>2;
}

bool PREDICTOR::get_global_predict(const branch_record_c* br, uint *predicted_target_address)
{
  return (alpha_gpt[phistory]&2)>>1;
}

bool PREDICTOR::get_prediction(const branch_record_c* br, const op_state_c* os, uint *predicted_target_address)
{
  //  ALPHA PREDICTION
  local_prediction = get_local_predict(br, predicted_target_address);
  global_prediction = get_global_predict(br, predicted_target_address);
  pred_choice = choose_predictor(br);
  final_prediction = (pred_choice == PRED_LOCAL) ? local_prediction : global_prediction;

  //  TARGET PREDICTION

  return final_prediction;   // true for taken, false for not taken
}

// Update the predictor after a prediction has been made.  This should accept
// the branch record (br) and architectural state (os), as well as a third
// argument (taken) indicating whether or not the branch was taken.
void PREDICTOR::update_predictor(const branch_record_c* br, const op_state_c* os, bool taken, uint actual_target_address)
{
  // ALPHA PREDICTOR UPDATE
  uint16_t curr = alpha_lht[PC10];

  // update prediction tables
  if (taken)
  {
    if (alpha_lpt[curr] < 7)
      alpha_lpt[curr]++;
    if (alpha_gpt[phistory] < 3)
      alpha_gpt[phistory]++;
  }
  else
  {
    if (alpha_lpt[curr] > 0)
      alpha_lpt[curr]--;
    if (alpha_gpt[phistory] > 0)
      alpha_gpt[phistory]--;
  }

  // update prediction choice table
  if (taken != final_prediction && global_prediction != local_prediction) // misprediction occured, but one predictor was correct
  {
    if (taken == global_prediction)
    {
      if (alpha_choice[phistory] < 3)
        alpha_choice[phistory]++;
    }
    else
    {
      if (alpha_choice[phistory] > 0)
        alpha_choice[phistory]--;
    }
  }

  // update path history
  phistory = keep_lower((phistory << 1) | taken, 12);

  // update local history table:  shift left, OR with taken, keep 10 bits
  alpha_lht[PC10] = keep_lower((alpha_lht[PC10] << 1) | taken, 10);

  return;
}

// return of PRED_LOCAL means use local history
// return of PRED_GLOBAL means use global history
bool PREDICTOR::choose_predictor(const branch_record_c* br)
{
  uint16_t curr_alpha_choice_entry;                      // holds current choice predict table entry

  curr_alpha_choice_entry = alpha_choice[phistory];               // current alpha_choice entry, indexed by path history
  curr_alpha_choice_entry = ( curr_alpha_choice_entry & 2 );      // only care about bit 1 of saturating counter

  if (curr_alpha_choice_entry == 0)  return PRED_LOCAL;  // bit 1 was not set
  else                      return PRED_GLOBAL; // bit 1 was set
}

// keeps lower n_bits of target ( glorified AND operation, because C++ != verilog )
uint32_t PREDICTOR::keep_lower(uint32_t target, int n_bits)
{
    return target & ((1<<n_bits)-1);
}
