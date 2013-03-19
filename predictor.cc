#include "predictor.h"
#define PC (br->instruction_addr)
#define PC10 (keep_lower(br->instruction_addr,10))
#define NEXT (br->instruction_next_addr)
#define PC_LOWER (keep_lower(addr_idx,idx_bits))

int logbase2(int input);

ALPHA::ALPHA()
{
  // set all our data structures to initial values
  for (int i = 0; i < SIZE_1K; i++)
  {
    alpha_lht[i] = 0;
    alpha_lpt[i] = WEAK_NT_LOCAL;
  }

  for (int i = 0; i < SIZE_4K; i++)
  {
    alpha_gpt[i]    = WEAK_NT_GLOBAL;
    alpha_choice[i] = WEAK_GLOB_PRED; 
  }

  phistory = 0;
}

bool ALPHA::get_local_predict(const uint32_t address)
{
  uint16_t alpha_lht_ind = (uint16_t) address;
  uint16_t curr = alpha_lht[alpha_lht_ind];
  uint16_t pred_bits = alpha_lpt[curr];

  return (pred_bits >> (SAT_LOCAL - 1));
}

bool ALPHA::get_global_predict()
{
  return (alpha_gpt[phistory] >> (SAT_GLOBAL - 1));
}

// return of PRED_LOCAL means use local history
// return of PRED_GLOBAL means use global history
bool ALPHA::choose_predictor()
{
  uint16_t curr_alpha_choice_entry;                     // holds current choice predict table entry
  curr_alpha_choice_entry = alpha_choice[phistory];     // current alpha_choice entry, indexed by path history
  return ( curr_alpha_choice_entry >> (SAT_PRED - 1));  // only care about bit 1 of saturating counter
}

bool ALPHA::get_prediction(const branch_record_c* br)
{
  local_prediction = get_local_predict(PC10);
  global_prediction = get_global_predict();

  final_prediction = (choose_predictor() == PRED_LOCAL) ? local_prediction : global_prediction;

  return final_prediction;
}

void ALPHA::update(const branch_record_c* br, bool taken)
{
  uint16_t curr = alpha_lht[PC10];

  // update prediction tables
  if (taken)
  {
    if (alpha_lpt[curr] < SAT_LOCAL_MAX)      alpha_lpt[curr]++;
    if (alpha_gpt[phistory] < SAT_GLOBAL_MAX) alpha_gpt[phistory]++;
  }
  else
  {
    if (alpha_lpt[curr] > SAT_LOCAL_MIN)      alpha_lpt[curr]--;
    if (alpha_gpt[phistory] > SAT_GLOBAL_MIN) alpha_gpt[phistory]--;
  }

  // update prediction choice table

  // concatenate global and local predictors
  // damn you, verilog, for being so nice!
  switch(((global_prediction == taken)<<1)|(local_prediction == taken))
  {
    case 0:
      break;
    case 1:
      if (alpha_choice[phistory] > SAT_PRED_MIN) alpha_choice[phistory]--;
      break;
    case 2:
      if (alpha_choice[phistory] < SAT_PRED_MAX) alpha_choice[phistory]++;
      break;
    case 3:
      break;
    default:
      ;
  }

  // update path history
  phistory = keep_lower((phistory << 1) | taken, PHISTORY_BITS);

  // update local history table:  shift left, OR with taken, keep 10 bits
  alpha_lht[PC10] = keep_lower((alpha_lht[PC10] << 1) | taken, LHISTORY_BITS);
}

PREDICTOR::PREDICTOR()
{
  maincache = new CACHE(M_ENTRIES, M_WAYS);
  victimcache = new CACHE(V_ENTRIES, V_WAYS);
}

PREDICTOR::~PREDICTOR()
{
  delete maincache;
}

bool PREDICTOR::get_prediction(const branch_record_c* br, const op_state_c* os, uint *predicted_target_address)
{

  //  TARGET PREDICTION
  bool main_miss;

  main_miss = maincache->predict(PC, predicted_target_address);  // updates *predicted_target_address

  if (main_miss)
    victimcache->predict(PC, predicted_target_address);

  if (br->is_return)
    *predicted_target_address = ras.pop_ret_pred();

  // ALPHA PREDICTION
  if (!(br->is_conditional))
    return TAKEN;
  else
    return alpha.get_prediction(br);   // true for taken, false for not taken
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
  bool evicted = false;
  uint evicted_tag;
  uint evicted_data;

  if( maincache->needs_update() )
    evicted = maincache->update(PC, actual_target_address, &evicted_tag, &evicted_data);

  if ( evicted )
    victimcache->update(evicted_tag, evicted_data, NULL, NULL);

  return;
}

//
//RETURN ADDRESS STACK
//
RAS::RAS()
{
  stack_size = RAS_ENTRIES;
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
    idx_bits = logbase2(entries);

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

CACHE::~CACHE()
{
  if (lru)
    delete lru;

  for (int i = 0; i < entries; i++)
  {
    if (data[i])
      delete [] data[i];
    if (tag[i])
      delete [] tag[i];
  }

  if (data)
    delete [] data;

  if (tag)
    delete [] tag;
}
// returns false (indicating no update needed) on hits (and updates *predicted_target_address )
// returns true (indicating update required) on misses (and sets *predicted_target_address to 0)
bool CACHE::predict(const uint32_t addr_idx, uint *predicted_target_address)
{
  for (int i = 0; i < ways; i++)
  {
    if( tag[PC_LOWER][i] == (addr_idx >> idx_bits))
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
bool CACHE::update(const uint32_t addr_idx, uint actual_target_address, uint *evicted_tag, uint *evicted_data)
{
  b_needs_update = false; // dismiss the flag

  // if there is an empty way, put the data there
  for (int i = 0; i < ways; i++)
  {
    if (data[PC_LOWER][i] == 0)
    {
      tag[PC_LOWER][i] = (addr_idx >> idx_bits);  // store tag in empty way
      data[PC_LOWER][i] = actual_target_address; // store branch address in empty way
      lru->update_all(i, PC_LOWER);
      return false; // there was an empty place to put
    }
  }

  //else, we need to evict
  uint victim = lru->get_victim( PC_LOWER );

  if (evicted_tag && evicted_data)
  {
    *evicted_tag = (((tag[PC_LOWER][victim])<<idx_bits) | PC_LOWER);
    *evicted_data = data[PC_LOWER][victim];
  }

  tag[PC_LOWER][victim] = (addr_idx >> idx_bits);  // use overwrite victim's tag field
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

  for (int i = 0; i < entries; i++)
    counter[i] = new uint[sizeof(uint) * ways];

  for (int i = 0; i < entries; i++)
  {
    for (int j = 0; j < ways; j++)
    {
      counter[i][j]=j;
    }
  }
}

LRU::~LRU()
{
  for (int i = 0; i < entries; i++)
  {
    if (counter[i])
      delete [] counter[i];
  }
  delete [] counter;
}

// given way_accessed, updates the counters at cache line given in index
void LRU::update_all( uint way_accessed, uint32_t index)
{
  uint way_value = counter[index][way_accessed];

  for (int i = 0; i < ways; i++)  // increase all counters
  {
    if ( counter[index][i] < way_value && counter[index][i] < (uint)(ways - 1))  // if it is lower than the count of the way used
      counter[index][i]++;                                // and less than the max count (saturating counters) then increase it
  }

  counter[index][way_accessed] = 0;  // and set the way accessed to 0 (most recently used)
  return;
}

// given a cache line (index), returns the LRU (ie the highest count)
uint LRU::get_victim( uint32_t index )
{
  for (int i = 0; i < ways; i++)
  {
    if (counter[index][i] == (uint)(ways - 1))
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
