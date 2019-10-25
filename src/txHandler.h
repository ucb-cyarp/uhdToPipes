//
// Created by Christopher Yarp on 10/19/19.
//

#ifndef UHDTOPIPES_TXHANDLER_H
#define UHDTOPIPES_TXHANDLER_H

#include <uhd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct{
    bool* terminateStatus; //Used to periodically check if thread should terminate
    char* txPipeName;
    char* txFeedbackPipeName;
    uhd_tx_streamer_handle tx_streamer; //This is a pointer
    uhd_tx_metadata_handle tx_md; //This is a pointer
    int samplesPerTransactTx;
    bool forceFullTxBuffer;
    bool txRateLimit;
    int txRate;

    bool verbose;
} txHandlerArgs_t;

//The input is a block of real samples concatinated with a block of imagionary samples
void* txHandler(void* argsUncast);

#endif //UHDTOPIPES_TXHANDLER_H
