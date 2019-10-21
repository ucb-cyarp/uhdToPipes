//
// Created by Christopher Yarp on 10/18/19.
//

#include "rxHandler.h"
#include "common.h"

void* rxHandler(void* argsUncast) {
    rxHandlerArgs_t* args = (rxHandlerArgs_t*) argsUncast;
    bool* terminateStatus = args->terminateStatus;
    char* rxPipeName = args->rxPipeName;
    uhd_rx_streamer_handle rx_streamer = args->rx_streamer;
    uhd_rx_metadata_handle rx_md = args->rx_md;
    uhd_stream_cmd_t rx_stream_start_cmd = args->rx_stream_start_cmd;
    uhd_stream_cmd_t rx_stream_stop_cmd = args->rx_stream_stop_cmd;
    int samplesPerTransactRx = args->samplesPerTransactRx;
    bool sendStopCmd = args->sendStopCmd;
    bool verbose = args->verbose;
    bool* wasRunning = args->wasRunning;

    *wasRunning = false;

    size_t samps_per_buff;
    float *buff = NULL;
    void **buffs_ptr = NULL;

    UHD_API uhd_error status = uhd_rx_streamer_max_num_samps(rx_streamer, &samps_per_buff);
    if(status){
        printf("Could not retrieve max number of samples ... exiting\n");
        return NULL;
    }

    fprintf(stderr, "Buffer size in samples: %zu\n", samps_per_buff);
    buff = malloc(samps_per_buff * 2 * sizeof(float)); //Note, each sample consists of 2
    buffs_ptr = (void **) &buff;

    float* remainingSamplesRe = malloc(samplesPerTransactRx * sizeof(float));
    float* remainingSamplesIm = malloc(samplesPerTransactRx * sizeof(float));
    float* samples = malloc(samplesPerTransactRx * 2 *sizeof(float));
    float* samplesRe = samples;
    float* samplesIm = samplesRe+samplesPerTransactRx;
    int numRemainingSamples = 0;

    // Issue stream command
    fprintf(stderr, "Issuing stream command.\n");
    status = uhd_rx_streamer_issue_stream_cmd(rx_streamer, &rx_stream_start_cmd);
    *wasRunning = true;
    if(!status) {
        // Set up file output
        FILE *rxPipe = fopen(rxPipeName, "wb");

        // Actual streaming
        bool running = true;
        int terminateCheckCounter = 0;
        while (running) {
            //Check if thread should exit
            if (terminateCheckCounter > TERMINATE_CHECK_ITTERATIONS) {
                terminateCheckCounter = 0;
                if (*terminateStatus = true) {
                    running = false; //not actually needed
                    break;
                }
            } else {
                terminateCheckCounter++;
            }

            size_t num_rx_samps = 0;
            status = uhd_rx_streamer_recv(rx_streamer, buffs_ptr, samps_per_buff, &rx_md, 3.0, false, &num_rx_samps);
            if(status){
                running = false; //not actually needed
                *terminateStatus = true;
                printf("Error receiving Rx samples from USRP\n");
                break;
            }

            uhd_rx_metadata_error_code_t error_code;
            status = uhd_rx_metadata_error_code(rx_md, &error_code);
            if(status){
                running = false; //not actually needed
                *terminateStatus = true;
                printf("Error receiving Rx metadata from USRP ... exiting\n");
                break;
            }
            if (error_code != UHD_RX_METADATA_ERROR_CODE_NONE) {
                running = false; //not actually needed
                *terminateStatus = true;
                fprintf(stderr, "Error code 0x%x was returned during streaming. Aborting.", error_code);
                break;
            }

            // Handle data (each sample comes in a pair of 2 floats, 1 for the real component and 1 for the imag component)
            //  The underlying C++ type is std::complex<float>

            int numBlocks = (num_rx_samps+numRemainingSamples)/samplesPerTransactRx;
            int numRemaining = (num_rx_samps + numRemainingSamples) % samplesPerTransactRx;
            int srcSampleInd = 0;

            for(int block = 0; block<numBlocks; block++){
                //Use remaining samples (if any)
                if(numRemainingSamples>0) {
                    memcpy(samplesRe, remainingSamplesRe, numRemainingSamples * sizeof(float));
                    memcpy(samplesIm, remainingSamplesIm, numRemainingSamples * sizeof(float));
                }
                int destIndOffset = numRemainingSamples;
                numRemainingSamples = 0;
                int samplesToTransferFromSrcArray = samplesPerTransactRx-destIndOffset;
                for(int i = 0; i<samplesToTransferFromSrcArray; i++){
                    samplesRe[destIndOffset+i] = buff[srcSampleInd+2*i];
                    samplesRe[destIndOffset+i] = buff[srcSampleInd+2*i+1];
                }
                srcSampleInd += samplesToTransferFromSrcArray*2; //*2 because each sample has 2 components

                //samples is samplesRe::samplesIm
                fwrite(samples, sizeof(float), samplesPerTransactRx*2, rxPipe);  //Note:
            }
            //Copy remaining samples
            for(int i = 0; i < numRemaining; i++){
                remainingSamplesRe[numRemainingSamples+i] = buff[srcSampleInd+2*i];
                remainingSamplesIm[numRemainingSamples+i] = buff[srcSampleInd+2*i+1];
            }
            numRemainingSamples += numRemaining; //This is += to handle the case when the number of received samples is less than the block size

            if (verbose) {
                int64_t full_secs;
                double frac_secs;
                uhd_rx_metadata_time_spec(rx_md, &full_secs, &frac_secs);
                fprintf(stderr, "Received packet: %zu samples, %.f full secs, %f frac secs\n",
                        num_rx_samps,
                        difftime(full_secs, (int64_t) 0),
                        frac_secs);
            }

        }

        fclose(rxPipe);
    }else{
        printf("Could not send streaming Rx command to USRP\n");
        *terminateStatus = true;
    }

    //Cleanup
    if(*wasRunning && sendStopCmd){
        //Send the stop command
        int stopStatus = uhd_rx_streamer_issue_stream_cmd(rx_streamer, &rx_stream_stop_cmd);
        if(stopStatus) {
            printf("Could not send streaming Rx stop command to USRP\n");
        }
        *wasRunning = true;
    }

    free(buff);
    free(remainingSamplesRe);
    free(remainingSamplesIm);
    free(samples);

    return NULL;
}