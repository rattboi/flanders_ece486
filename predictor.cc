#include "predictor.h"
#define PC (br->instruction_addr)
#define PC10 (keep_lower(br->instruction_addr,10))
#define NEXT (br->instruction_next_addr)
#define PC_LOWER (keep_lower(PC,INDEX))

int logbase2(int input)
{
  int result = 0;

  if (input == 0) return 0;

  while (input >>= 1) ++result;
  return result;

}

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

bool PREDICTOR::get_local_predict(const branch_record_c* br)
  {
    uint16_t alpha_lht_ind = PC10;
    uint16_t curr = alpha_lht[alpha_lht_ind];
    uint16_t pred_bits = alpha_lpt[curr];

    return (pred_bits & 4)>>2;
  }
bool PREDICTOR::get_global_predict(const branch_record_c* br)
  {
  return (alpha_gpt[phistory]&2)>>1;
  }
bool PREDICTOR::get_prediction(const branch_record_c* br, const op_state_c* os, uint *predicted_target_address)
{
 // for (int j=0; j<2050; j+=2)
 //   printf("logbase2 of %d: %d\n", j, logbase2(j));


  //  ALPHA PREDICTION
    local_prediction = get_local_predict(br);
    global_prediction = get_global_predict(br);
    pred_choice = choose_predictor(br);

    final_prediction = (pred_choice == PRED_LOCAL) ? local_prediction : global_prediction;

//  TARGET PREDICTION
  if( thecache.predict(br, predicted_target_address) == false );
    thecache.needs_update = true;
return final_prediction;
}

void PREDICTOR::update_predictor(const branch_record_c* br, const op_state_c* os, bool taken, uint actual_target_address)
{
  uint16_t curr = alpha_lht[PC10];
// ALPHA PREDICTOR UPDATE
  // update prediction tables
  if (taken)
  {
    if (alpha_lpt[curr] < 7)      alpha_lpt[curr]++;
    if (alpha_gpt[phistory] < 3)  alpha_gpt[phistory]++;
  }
  else
  {
    if (alpha_lpt[curr] > 0)      alpha_lpt[curr]--;
    if (alpha_gpt[phistory] > 0)  alpha_gpt[phistory]--;
  }
  // update prediction choice table
  if (taken != final_prediction && global_prediction != local_prediction) // misprediction occured, but one predictor was correct
  {
    if (taken == global_prediction)
    {
      if (alpha_choice[phistory] < 3)   alpha_choice[phistory]++;
    }
    else
    {
      if (alpha_choice[phistory] > 0)   alpha_choice[phistory]--;
    }
  }

  // update path history
  phistory = keep_lower((phistory << 1) | taken, 12);

  // update local history table:  shift left, OR with taken, keep 10 bits
  alpha_lht[PC10] = keep_lower((alpha_lht[PC10] << 1) | taken, 10);

// TARGET UPDATE
  if( thecache.needs_update )
  {
    thecache.update(br, actual_target_address);
    thecache.needs_update = false;
  }

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
  else return PRED_GLOBAL; // bit 1 was set
}

CACHE::CACHE()
{
  needs_update = false;

}

// returns true on hits (and updates *predicted_target_address )
// returns false on misses (and sets *predicted_target_address to 0)
bool CACHE::predict(const branch_record_c* br, uint *predicted_target_address)
{
  for (int i = 0; i < WAYS; i++)
  {
    if( tag[PC_LOWER][i] == PC >> INDEX )
    {
      *predicted_target_address = data[PC_LOWER][i];
      return true;
    }
  }

  // cycled through all ways and no tags matched.  Not in cache.o
  *predicted_target_address = 0;
  return false;
}

// no need to call unless misprediction or empty
void CACHE::update(const branch_record_c* br, uint actual_target_address)
{
  // if there is an empty way, put the data there
  for (int i = 0; i < WAYS; i++)
  {
    if (data[PC_LOWER][i] == 0) // look through all ways
      tag[PC_LOWER][i] = PC >> INDEX;  // empty tag field found, indicating empty way
      data[PC_LOWER][i] = actual_target_address; // store branch address in empty way
      return;
  }

  //else, we need to evict
  uint victim = count[PC_LOWER];  // read out the current victim
  tag[PC_LOWER][victim] = PC >> INDEX;  // use overwrite victim's tag field
  data[PC_LOWER][victim] = actual_target_address; // overwrite victim's data field

  // this if/else structure allows non power-of-2 values to be used for ways
  if (victim == WAYS)
    count[PC_LOWER] = 0;
  else
    count[PC_LOWER] = victim++;

  return;


  //tag[count[PC_LOWER]][PC_LOWER] = actual_target_address >> INDEX;
  //count[PC_LOWER] = (count[PC_LOWER]++) % WAYS;
  //return;
}

// keeps lower n_bits of target ( glorified AND operation, because C++ != verilog )
uint32_t keep_lower(uint32_t target, int n_bits)
{
    return target & ((1<<n_bits)-1);
}

