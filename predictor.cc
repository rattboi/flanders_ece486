#include "predictor.h"

bool PREDICTOR::get_local_predict(const branch_record_c* br, uint *predicted_target_address)
{
  uint16_t lht_ind = mask_upper(br->instruction_addr, 10);
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
// return of 0 (false) means use local history
// return of 1 (true) means use global history
{
    return ( cpt[phistory] & 2 ) >> 1;
}

uint16_t PREDICTOR::mask_upper(uint16_t target, int keep_lower)
{
    switch (keep_lower)
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
