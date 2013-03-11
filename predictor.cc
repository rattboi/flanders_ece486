#include "predictor.h"
#include <cmath>

PREDICTOR::PREDICTOR()
{
  for (int i = 0; i < SIZE_1K; i++)
  {
    lht[i] = 0;
    lpt[i] = 0;
  }

  for (int i = 0; i < SIZE_4K; i++)
  {
    gpt[i] = 0;
    cpt[i] = 2;
  }
  phistory = 0;

  for (int i = 0; i < SIZE_1K; i++)
    btb[i] = 0;
}

bool PREDICTOR::get_local_predict(const branch_record_c* br, uint *predicted_target_address)
{
  uint16_t lht_ind = keep_lower(br->instruction_addr, 10);
  uint16_t lp_ind = lht[lht_ind];
  uint16_t pred_bits = lpt[lp_ind];

  return (pred_bits & 4)>>2;
}

bool PREDICTOR::get_global_predict(const branch_record_c* br, uint *predicted_target_address)
{
  return (gpt[phistory]&2)>>1;
}

bool PREDICTOR::get_prediction(const branch_record_c* br, const op_state_c* os, uint *predicted_target_address)
{
  local_prediction = get_local_predict(br, predicted_target_address);
  global_prediction = get_global_predict(br, predicted_target_address);

  pred_choice = choose_predictor(br);

  if (pred_choice == PRED_LOCAL) //choose which predictor to use, local or global
    final_prediction = local_prediction;
  else
    final_prediction = global_prediction;


  if (final_prediction == TAKEN)
  {
    if (btb[keep_lower(br->instruction_addr,10)] == 0)
      *predicted_target_address = br->instruction_next_addr;
    else
      *predicted_target_address = btb[keep_lower(br->instruction_addr,10)];
  }
  else
    *predicted_target_address = br->instruction_next_addr;


  if(br->is_conditional == false && btb[keep_lower(br->instruction_addr,10)])
  {
    *predicted_target_address = btb[keep_lower(br->instruction_addr,10)];
  }

  return final_prediction;   // true for taken, false for not taken
}

// Update the predictor after a prediction has been made.  This should accept
// the branch record (br) and architectural state (os), as well as a third
// argument (taken) indicating whether or not the branch was taken.
void PREDICTOR::update_predictor(const branch_record_c* br, const op_state_c* os, bool taken, uint actual_target_address)
{
  // update local first
  uint16_t lht_ind = keep_lower(br->instruction_addr, 10);
  uint16_t lp_ind = lht[lht_ind];

  if((br->is_conditional) && !(br->is_return) && br->instruction_addr < actual_target_address && actual_target_address - br->instruction_addr > maxdisp)
    maxdisp = actual_target_address - br->instruction_addr;

  if((br->is_conditional) && !(br->is_return) && br->instruction_addr > actual_target_address && br->instruction_addr - actual_target_address > mindisp)
    mindisp = br->instruction_addr - actual_target_address;




  stuff = 0;

  if (br->is_call)          stuff = stuff + 1;
  if (br->is_conditional)   stuff = stuff + 2;
  if (br->is_indirect)      stuff = stuff + 4;
  if (br->is_return)        stuff = stuff + 8;

  stats[stuff]++;

  // update local prediction table
  if (taken)
  {
    if (lpt[lp_ind] < 7)
      lpt[lp_ind]++;
  }
  else
  {
    if (lpt[lp_ind] > 0)
      lpt[lp_ind]--;
  }

  // update local history table
  lht[lht_ind] = keep_lower((lht[lht_ind] << 1) | taken, 10);

  // update global now

  // update global prediction table
  if (taken)
  {
    if (gpt[phistory] < 3)
      gpt[phistory]++;
  }
  else
  {
    if (gpt[phistory] > 0)
      gpt[phistory]--;
  }

  // update prediction choice table
  if (taken != final_prediction && global_prediction != local_prediction)
  {
    if (taken == global_prediction)
    {
      if (cpt[phistory] < 3)
        cpt[phistory]++;
    }
    else
    {
      if (cpt[phistory] > 0)
        cpt[phistory]--;
    }
  }

  // update path history
  phistory = keep_lower((phistory << 1) | taken, 12);

  // update BTB entry
  btb[keep_lower(br->instruction_addr,10)] = actual_target_address;

  return;
}

// return of PRED_LOCAL means use local history
// return of PRED_GLOBAL means use global history
bool PREDICTOR::choose_predictor(const branch_record_c* br)
{
  uint16_t curr_cpt_entry;                      // holds current choice predict table entry

  curr_cpt_entry = cpt[phistory];               // current cpt entry, indexed by path history
  curr_cpt_entry = ( curr_cpt_entry & 2 );      // only care about bit 1 of saturating counter

  if (curr_cpt_entry == 0)  return PRED_LOCAL;  // bit 1 was not set
  else                      return PRED_GLOBAL; // bit 1 was set
}

// keeps lower n_bits of target ( glorified AND operation, because C++ != verilog )
uint32_t PREDICTOR::keep_lower(uint32_t target, int n_bits)
{
    return target & ((1<<n_bits)-1);
}
