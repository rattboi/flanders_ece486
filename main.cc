/* Author: Chris Wilkerson;   Created: Thu Aug 12 16:19:58 PDT 2004
 * Description: Branch predictor driver.
*/

#include <cstdio>
#include <cstdlib>
#include "tread.h"

// include and define the predictor
#include "predictor.h"
PREDICTOR predictor;

// usage: predictor <trace>
int
main(int argc, char* argv[])
{
    using namespace std;

    if (2 != argc) {
        printf("usage: %s <trace>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    cbp_trace_reader_c cbptr = cbp_trace_reader_c(argv[1]);
    branch_record_c br;
    uint targetaddress;		// predicted target address

    // read the trace, one branch at a time, placing the branch info in br
    while (cbptr.get_branch_record(&br)) {

        // ************************************************************
        // Competing predictors must have the following methods:
        // ************************************************************

        // get_prediction() returns the prediction your predictor would like to make
        bool predicted_taken = predictor.get_prediction(&br, cbptr.osptr, &targetaddress);

        // predict_branch() tells the trace reader how you have predicted the branch and target address
        bool actual_taken    = cbptr.predict_branch(predicted_taken, &targetaddress);

        // finally, update_predictor() is used to update your predictor with the
        // correct branch result
        predictor.update_predictor(&br, cbptr.osptr, actual_taken, targetaddress);
    }

    //for (int i=0; i<16; i++)
    //    if (predictor.stats[i]) printf("stats[%x]=%i\n", i, predictor.stats[i]);
    double sum = predictor.stats[0] + predictor.stats[1] + predictor.stats[2] + predictor.stats[4] + predictor.stats[5] + predictor.stats[12];

    printf("PC-relative   conditional (jump): %lf\n", 100*(predictor.stats[2]/sum));
    printf("PC-relative unconditional (jump): %lf\n", 100*(predictor.stats[0]/sum));
    printf("PC-relative unconditional call:   %lf\n", 100*(predictor.stats[1]/sum));
    printf("Computed    unconditional return: %lf\n", 100*(predictor.stats[12]/sum));
    printf("Computed    unconditional call:   %lf\n", 100*(predictor.stats[5]/sum));
    printf("Computed    unconditional (jump): %lf\n", 100*(predictor.stats[4]/sum));

    printf("maxdisp = %x\n", predictor.maxdisp);
    printf("mindisp = %x\n", predictor.mindisp);
}

