//
// Created by Christopher Yarp on 10/18/19.
//

#ifndef UHDTOPIPES_RXHANDLER_H
#define UHDTOPIPES_RXHANDLER_H

#include <uhd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct{
    bool* terminateStatus; //Used to periodically check if thread should terminate
    char* rxPipeName;
    uhd_rx_streamer_handle rx_streamer; //This is a pointer
    uhd_rx_metadata_handle rx_md; //This is a pointer
    uhd_stream_cmd_t rx_stream_start_cmd; //This is NOT a pointer
    uhd_stream_cmd_t rx_stream_stop_cmd; //This is NOT a pointer
    bool sendStopCmd;
    int samplesPerTransactRx;
    bool verbose;

    bool* wasRunning; //Used for feedback when exiting.  Tells if it was running
} rxHandlerArgs_t;

//The output format is a block of real samples concatinated with a block of imagionary samples
void* rxHandler(void* args);

#endif //UHDTOPIPES_RXHANDLER_H
