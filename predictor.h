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

class PREDICTOR
{
public:
    bool get_prediction(const branch_record_c* br, const op_state_c* os, uint *predicted_target_address);

    void update_predictor(const branch_record_c* br, const op_state_c* os, bool taken, uint actual_target_address);

private:

    uint16_t lht[1024]; //  local history table
    uint16_t lpt[1024]; //  local prediction table

    uint16_t gpt[1024]; //  global predition table

    uint16_t cpt[1024]; //  choice predition table

    uint16_t phistory;  //  path history

    bool get_local_predict(const branch_record_c* br, uint *predicted_target_address);

    bool get_global_predict(const branch_record_c* br, uint *predicted_target_address);

    bool get_choice_predict(const branch_record_c* br);
};

#endif // PREDICTOR_H_SEEN

