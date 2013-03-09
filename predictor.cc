#include "predictor.h"

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

}

bool PREDICTOR::choose_predictor(const branch_record_c* br)
{
    uint16_t


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
