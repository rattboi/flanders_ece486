#include "predictor.h"

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
  bool prediction = true;

  if (br->is_conditional)
  prediction = false;

  return prediction;   // true for taken, false for not taken
}

// Update the predictor after a prediction has been made.  This should accept
// the branch record (br) and architectural state (os), as well as a third
// argument (taken) indicating whether or not the branch was taken.
void PREDICTOR::update_predictor(const branch_record_c* br, const op_state_c* os, bool taken, uint actual_target_address)
{
    return;
}

bool PREDICTOR::choose_predictor(const branch_record_c* br)
// return of PRED_LOCAL means use local history
// return of PRED_GLOBAL means use global history
{
  uint16_t curr_cpt_entry;                      // holds current choice predict table entry

  curr_cpt_entry = cpt[phistory];               // current cpt entry, indexed by path history
  curr_cpt_entry = ( curr_cpt_entry & 2 );      // only care about bit 1 of saturating counter


  if (curr_cpt_entry == 0)  return PRED_LOCAL;  // bit 1 was not set
  else                      return PRED_GLOBAL; // bit 1 was set
}

uint16_t PREDICTOR::keep_lower(uint16_t target, int n_bits)
// keeps lower n_bits of target ( glorified AND operation, because C++ != verilog )
{
    switch (n_bits)
    {
        case 2:     return target & 3;
        case 3:     return target & 7;
        case 10:    return target & 1023;
        case 12:    return target & 4095;
        default:    return 0;
    }

    return 0;
}


//    branch_record_c();
//    ~branch_record_c();
//    void init();                   // init the branch record
//    void debug_print();            // print the information in the branch record (for debugging)
//    uint   instruction_addr;       // the branch's PC (program counter)
//    uint   instruction_next_addr;  // the PC of the static instruction following the branch
//    bool   is_indirect;            // true if the target is computed; false if it's PC-rel; returns are also considered indirect
//    bool   is_conditional;         // true if the branch is conditional; false otherwise
//    bool   is_call;                // true if the branch is a call; false otherwise
//    bool   is_return;              // true if the branch is a return; false otherwise
