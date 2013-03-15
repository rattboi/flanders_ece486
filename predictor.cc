#include "predictor.h"
#define PC (br->instruction_addr)
#define PC10 (keep_lower(br->instruction_addr,10))
#define NEXT (br->instruction_next_addr)

PREDICTOR::PREDICTOR()
{
  for (int i = 0; i < SIZE_1K; i++)
  {
    lht[i] = 0;
    lpt[i] = 0;
    btb_offset[i] = 0;
  }

  for (int i = 0; i < SIZE_4K; i++)
  {
    gpt[i] = 0;
    cpt[i] = 2;
  }
  phistory = 0;
}

bool PREDICTOR::get_local_predict(const branch_record_c* br, uint *predicted_target_address)
{
  uint16_t lp_ind = lht[PC10];
  uint16_t pred_bits = lpt[lp_ind];
  return (pred_bits & 4)>>2;
}

bool PREDICTOR::get_global_predict(const branch_record_c* br, uint *predicted_target_address)
{
  return (gpt[phistory]&2)>>1;
}

bool PREDICTOR::get_prediction(const branch_record_c* br, const op_state_c* os, uint *predicted_target_address)
{
  int32_t offset;

  local_prediction  = get_local_predict  (br, predicted_target_address);
  global_prediction = get_global_predict (br, predicted_target_address);

  pred_choice = choose_predictor(br);

  final_prediction = (pred_choice == PRED_LOCAL) ? local_prediction : global_prediction;

  // the following logic may need to change
  if (final_prediction == TAKEN)
  {
    if ( br->is_indirect == false )  // if branch is PC-relative
      if ( btb_offset[PC10] != 0)  // and if we have something in the branch target buffer at this index
      {
        offset = keep_lower(btb_offset[PC10], 24);
        offset = sign_extend24(offset);
        *predicted_target_address = PC + offset;
      }
  }
  else
    *predicted_target_address = NEXT;

  return final_prediction;   // true for taken, false for not taken
}

// Update the predictor after a prediction has been made.  This should accept
// the branch record (br) and architectural state (os), as well as a third
// argument (taken) indicating whether or not the branch was taken.
void PREDICTOR::update_predictor(const branch_record_c* br, const op_state_c* os, bool taken, uint actual_target_address)
{
  // update local first
  uint16_t lp_ind = lht[PC10];

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

  // update local history table (shift left, OR with 'taken,'' then keep only 10 bits)
  lht[PC10] = keep_lower((lht[PC10] << 1) | taken, 10);

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
  if ((taken != final_prediction) && (global_prediction != local_prediction))
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

  // update global path history (shift left, OR with 'taken', then keep only 10 bits)
  phistory = keep_lower((phistory << 1) | taken, 12);

  if (br->is_indirect == false)
    btb_offset[PC10] = keep_lower(actual_target_address - br->instruction_addr,24);

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

uint32_t PREDICTOR::sign_extend24(uint32_t val_to_extend)
{
  if (val_to_extend & 0x800000) // if first bit is set
    return val_to_extend | 0xFF000000; // set upper bits
  else
    return val_to_extend;
}
