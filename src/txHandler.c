//
// Created by Christopher Yarp on 10/19/19.
//

#include "txHandler.h"
#include "common.h"
#include <uhd.h>

void* txHandler(void* argsUncast) {
    txHandlerArgs_t* args = (txHandlerArgs_t*) argsUncast;
    bool* terminateStatus = args->terminateStatus;
    char* txPipeName = args->txPipeName;
    char* txFeedbackPipeName = args->txFeedbackPipeName;
    uhd_tx_streamer_handle tx_streamer = args->tx_streamer;
    uhd_tx_metadata_handle tx_md = args->tx_md;
    int samplesPerTransactTx = args->samplesPerTransactTx;
    bool forceFullTxBuffer = args->forceFullTxBuffer;
    bool verbose = args->verbose;

    size_t samps_per_buff;
    uhd_error status = uhd_tx_streamer_max_num_samps(tx_streamer, &samps_per_buff);
    if(status){
        printf("Could not retrieve max number of Tx samples ... exiting\n");
        return NULL;
    }

    fprintf(stderr, "Buffer size in samples: %zu\n", samps_per_buff);
    float *buff = calloc(sizeof(float), samps_per_buff * 2);
    const void **buffs_ptr = (const void **) &buff;

    //Note: the samples are complex floats which have a real component followed by an imagionary component

    // Set up pipes
    FILE *txPipe = fopen(txPipeName, "rb");

    FILE *txFeedbackPipe = NULL;
    if(txFeedbackPipeName != NULL){
        txFeedbackPipe = fopen(txFeedbackPipeName, "wb");
    }

    float* pipeSamples = malloc(samplesPerTransactTx*2*sizeof(float));
    float* pipeSamplesRe = pipeSamples;
    float* pipeSamplesIm = pipeSamples+samps_per_buff;
    float* samplesRemainder = malloc(samps_per_buff*2*sizeof(float));
    int numRemainingSamples = 0;

    int terminateCheckCounter = 0;

    bool running = true;
    while(running) {
        if (terminateCheckCounter > TERMINATE_CHECK_ITTERATIONS) {
            terminateCheckCounter = 0;
            if (*terminateStatus == true) {
                running = false; //not actually needed
                break;
            }
        } else {
            terminateCheckCounter++;
        }

        int elementsRead = fread(pipeSamples, sizeof(float)*2, samplesPerTransactTx, txPipe);
        if(feof(txPipe)){
            running = false; //Not actually needed
            *terminateStatus = true; //Inform other threads to stop (Tx pipe closed)
            break;
        }else if(elementsRead != samplesPerTransactTx && ferror(txPipe)){
            printf("An error was encountered while reading the Tx pipe\n");
            perror(NULL);
            *terminateStatus = true; //Inform other threads to stop (Tx pipe error)
            running = false; //Not actually needed
            break;
        }else if(elementsRead != samplesPerTransactTx){
            printf("An unknown error was encountered while reading the Tx pipe\n");
            *terminateStatus = true; //Inform other threads to stop (Tx pipe error)
            running = false; //Not actually needed
            break;
        }

        //Find number of tx transactions per block
        int numTransmissions = (samplesPerTransactTx+numRemainingSamples)/samps_per_buff;
        int sampsReamining = (samplesPerTransactTx+numRemainingSamples)%samps_per_buff;
        int srcSampleInd = 0;

        for(int block = 0; block < numTransmissions; block++){
            //Copy any remaining samples (if any)
            if(numRemainingSamples>0) {
                memcpy(buff, samplesRemainder, sizeof(float)*2*numRemainingSamples);
            }
            int dstIndOffset = numRemainingSamples;
            numRemainingSamples = 0;

            //Copy samples from pipe block
            int samplesToTransferFromSrcArray = samps_per_buff-dstIndOffset;
            for(int i = 0; i<samplesToTransferFromSrcArray; i++){
                buff[dstIndOffset+2*i] = pipeSamplesRe[srcSampleInd+i];
                buff[dstIndOffset+2*i+1] = pipeSamplesIm[srcSampleInd+i];
            }
            srcSampleInd += samplesToTransferFromSrcArray;

            size_t num_samps_sent = 0;
            uhd_error status = uhd_tx_streamer_send(tx_streamer, buffs_ptr, samps_per_buff, &tx_md, 10, &num_samps_sent);
            if(status){
                running = false; //not actually needed
                *terminateStatus = true;
                printf("Error sending to USRP\n");
                break;
            }
            if(num_samps_sent != samps_per_buff){
                running = false; //not actually needed
                *terminateStatus = true;
                printf("Unable to send complete Tx block to the FPGA within the timeout\n");
                break;
            }

            if(verbose){
                fprintf(stderr, "Sent %zu samples\n", num_samps_sent);
            }
        }

        //TODO: Handle the remaining samples
        //Either partially fill another buffer or place it in the remainder
        if(forceFullTxBuffer){
            //Copy remaining samples to remainder buffer
            for(int i = 0; i<sampsReamining; i++){
                samplesRemainder[numRemainingSamples*2+i*2] = pipeSamplesRe[srcSampleInd+i];
                samplesRemainder[numRemainingSamples*2+i*2+1] = pipeSamplesRe[srcSampleInd+i];
            }
            numRemainingSamples += sampsReamining; //This is += to handle the case when the number of received samples is less than the block size
        }else{
            //Partially fill a buffer and send it
            //Remainder cannot exist
            for(int i = 0; i<sampsReamining; i++){
                buff[2*i] = pipeSamplesRe[srcSampleInd+i];
                buff[2*i+1] = pipeSamplesIm[srcSampleInd+i];
            }
            //Do not need to incremnet srcSampleInd since this is the last transmission for this block and it will be reset on the next iteration
            size_t num_samps_sent = 0;
            uhd_error status = uhd_tx_streamer_send(tx_streamer, buffs_ptr, sampsReamining, &tx_md, 10, &num_samps_sent);
            if(status){
                running = false; //not actually needed
                *terminateStatus = true;
                printf("Error sending to USRP\n");
                break;
            }
            if(num_samps_sent != sampsReamining){
                running = false; //not actually needed
                *terminateStatus = true;
                printf("Unable to send complete Tx block to the FPGA within the timeout\n");
                break;
            }

            numTransmissions++; //Increment numTransmissions for reporting on the feedback pipe
        }

        //Report Feedback if Pipe Exists
        if(txFeedbackPipe != NULL) {
            FEEDBACK_DATATYPE fbVal = numTransmissions;
            fwrite(&fbVal, sizeof(FEEDBACK_DATATYPE), 1, txFeedbackPipe);
        }
    }

    free(buff);
    free(samplesRemainder);

    return NULL;
}