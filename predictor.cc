#include "predictor.h"
#define PC (br->instruction_addr)
#define PC10 (keep_lower(br->instruction_addr,10))
#define NEXT (br->instruction_next_addr)
#define PC_LOWER (keep_lower(PC,M_INDEX))

int logbase2(int input);

ALPHA::ALPHA()
{
  // set all our data structures to initial values
  for (int i = 0; i < SIZE_1K; i++)
  {
    alpha_lht[i] = 0;
    alpha_lpt[i] = 3;
  }

  for (int i = 0; i < SIZE_4K; i++)
  {
    alpha_gpt[i] = 1;
    alpha_choice[i] = 2; //default to weakly global
  }

  phistory = 0;
}

bool ALPHA::get_local_predict(const branch_record_c* br)
{
  uint16_t alpha_lht_ind = PC10;
  uint16_t curr = alpha_lht[alpha_lht_ind];
  uint16_t pred_bits = alpha_lpt[curr];

  return (pred_bits & 4)>>2;
}

bool ALPHA::get_global_predict(const branch_record_c* br)
{
  return (alpha_gpt[phistory]&2)>>1;
}

// return of PRED_LOCAL means use local history
// return of PRED_GLOBAL means use global history
bool ALPHA::choose_predictor(const branch_record_c* br)
{
  uint16_t curr_alpha_choice_entry;                      // holds current choice predict table entry
  curr_alpha_choice_entry = alpha_choice[phistory];               // current alpha_choice entry, indexed by path history
  curr_alpha_choice_entry = ( curr_alpha_choice_entry & 2 );      // only care about bit 1 of saturating counter
  if (curr_alpha_choice_entry == 0)  return PRED_LOCAL;  // bit 1 was not set
  else return PRED_GLOBAL; // bit 1 was set
}

bool ALPHA::get_prediction(const branch_record_c* br)
{
  local_prediction = get_local_predict(br);
  global_prediction = get_global_predict(br);

  final_prediction = (choose_predictor(br) == PRED_LOCAL) ? local_prediction : global_prediction;

  return final_prediction;
}

void ALPHA::update(const branch_record_c* br, bool taken)
{
  uint16_t curr = alpha_lht[PC10];

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

  switch(((global_prediction == taken)<<1)|(local_prediction == taken))
  {
    case 0:
      break;
    case 1:
      if (alpha_choice[phistory] > 0)   alpha_choice[phistory]--;
      break;
    case 2:
      if (alpha_choice[phistory] < 3)   alpha_choice[phistory]++;
      break;
    case 3:
      break;
    default:
      ;
  }

  // update path history
  phistory = keep_lower((phistory << 1) | taken, 12);

  // update local history table:  shift left, OR with taken, keep 10 bits
  alpha_lht[PC10] = keep_lower((alpha_lht[PC10] << 1) | taken, 10);
}

PREDICTOR::PREDICTOR()
{
  maincache = new CACHE(M_ENTRIES, M_WAYS);
  

}

bool PREDICTOR::get_prediction(const branch_record_c* br, const op_state_c* os, uint *predicted_target_address)
{
  // ALPHA PREDICTION
  bool pred;
  pred =  alpha.get_prediction(br);   // true for taken, false for not taken

  //  TARGET PREDICTION
  maincache->predict(br, predicted_target_address);  // updates *predicted_target_address

  if (br->is_return)
    *predicted_target_address = ras.pop_ret_pred();

  if (!(br->is_conditional))
    return TAKEN;
  else
    return pred;   // true for taken, false for not taken
}

// Update the predictor after a prediction has been made.  This should accept
// the branch record (br) and architectural state (os), as well as a third
// argument (taken) indicating whether or not the branch was taken.
void PREDICTOR::update_predictor(const branch_record_c* br, const op_state_c* os, bool taken, uint actual_target_address)
{
  // ALPHA UPDATE
  if (br->is_conditional)
    alpha.update(br, taken);

  if (br->is_call) //if a call is happening, save the next address to the RAS
    ras.push_call(NEXT);

  // TARGET UPDATE
  if( maincache->needs_update() )
    maincache->update(br, actual_target_address);

  return;
}

//
//RETURN ADDRESS STACK
//
RAS::RAS()
{
  stack_size = 64;
};

uint RAS::pop_ret_pred()
{
  uint retval;

  if (!stack.empty())
  {
    retval = stack.front();
    stack.pop_front();
    return retval;
  }
  else
    return 0;
}

bool RAS::push_call(uint address)
{
  stack.push_front(address);

  if (stack.size() == stack_size)
  {
    stack.pop_back();
    return false;
  }

  return true;
}

//
// CACHE
//
CACHE::CACHE(uint m_entries, uint m_ways)
{
    entries = m_entries; 
    ways = m_ways;

    b_needs_update = false;

    lru = new LRU(entries, ways);

    data = new uint*[sizeof(uint*)*entries];
    tag = new uint*[sizeof(uint*)*entries];

    for (int i = 0; i < entries; i++)
    {
      data[i] = new uint[sizeof(uint)*ways];
      tag[i] = new uint[sizeof(uint)*ways];
    }

    for (int i = 0; i < entries; i++)
    {
      for (int j = 0; j < ways; j++)
      {
        data[i][j] = 0;
        tag[i][j] = 0;
      }
    }
}

// returns false (indicating no update needed) on hits (and updates *predicted_target_address )
// returns true (indicating update required) on misses (and sets *predicted_target_address to 0)
bool CACHE::predict(const branch_record_c* br, uint *predicted_target_address)
{
  for (int i = 0; i < ways; i++)
  {
    if( tag[PC_LOWER][i] == (PC >> logbase2(entries)))
    {
      *predicted_target_address = data[PC_LOWER][i];
      lru->update_all(i, PC_LOWER); // update LRU counters
      b_needs_update = false;
      return false; // does not need to update
    }
  }

  // cycled through all ways and no tags matched.  Not in cache
  *predicted_target_address = 0;
  // make it look in the victim

  b_needs_update = true;
  return true; // cache needs to update because this was a miss
}

// no need to call unless misprediction or empty
bool CACHE::update(const branch_record_c* br, uint actual_target_address)
{
  b_needs_update = false; // dismiss the flag

  // if there is an empty way, put the data there
  for (int i = 0; i < ways; i++)
  {
    if (data[PC_LOWER][i] == 0)
    {
      tag[PC_LOWER][i] = (PC >> logbase2(entries));  // store tag in empty way
      data[PC_LOWER][i] = actual_target_address; // store branch address in empty way
      lru->update_all(i, PC_LOWER);
      return false; // there was an empty place to put
    }
  }

  //else, we need to evict
  uint victim = lru->get_victim( PC_LOWER );\
  tag[PC_LOWER][victim] = (PC >> logbase2(entries));  // use overwrite victim's tag field
  data[PC_LOWER][victim] = actual_target_address; // overwrite victim's data field
  lru->update_all(victim, PC_LOWER);
  return true;  // eviction was made
}

bool CACHE::needs_update()
{
  return b_needs_update;
}

//
// LRU
//
// counters must be initialized to a valid ordering (no repeating values)
LRU::LRU(uint m_entries, uint m_ways)
{
  entries = m_entries;
  ways = m_ways;

  counter = new uint*[sizeof(uint *) * entries];
  
  for (uint i = 0; i < entries; i++)
    counter[i] = new uint[sizeof(uint) * ways];

  for (uint i = 0; i < entries; i++)
  {
    for (uint j = 0; j < ways; j++)
    {
      counter[i][j]=j;
    }
  }
}

// given way_accessed, updates the counters at cache line given in index
void LRU::update_all( uint way_accessed, uint32_t index)
{
  uint way_value = counter[index][way_accessed];

  for (uint i = 0; i < ways; i++)  // increase all counters
  {
    if ( counter[index][i] < way_value && counter[index][i] < (ways-1)) // if it is lower than the count of the way used
          counter[index][i]++;         // and less than the max count (saturating counters) then increase it
  }

  counter[index][way_accessed] = 0;  // and set the way accessed to 0 (most recently used)
  return;
}

// given a cache line (index), returns the LRU (ie the highest count)
uint LRU::get_victim( uint32_t index )
{
  for (uint i = 0; i < ways; i++)
  {
    if (counter[index][i] == (ways - 1))
    {
      return i;
    }
  }
  exit(-1); // should never reach this point-- included to silence compiler warnings
}

// keeps lower n_bits of target ( glorified AND operation, because C++ != verilog )
uint32_t keep_lower(uint32_t target, int n_bits)
{
  return target & ((1<<n_bits)-1);
}

int logbase2(int input)
{
  int result = 0;

  if (input == 0) return 0;

  while (input >>= 1) ++result;
  return result;
}
