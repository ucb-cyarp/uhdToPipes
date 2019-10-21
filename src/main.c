/**
 * This file is based heavily on the rx_samples_c.c and tx_samples_c.c UHD examples by Ettus Research
 * The modifications were not created, reviewed, or endorsed by Ettus Research or National Instruments.
 */

#define _GNU_SOURCE
#include <uhd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sched.h>
#include <signal.h>
#include "txHandler.h"
#include "rxHandler.h"

//Global (for sig handler)
bool terminateStatus = false;

void print_help(void){
    fprintf(stderr, "rx_samples_c - A simple RX example using UHD's C API\n\n"

                    "Options: (with conflicting arguments, last argument issued has precidence)\n"
                    "    -a (device args)\n"
                    "    -f (frequency in Hz)\n"
                    "    -r (sample rate in Hz)\n"
                    "    -g (Tx & Rx gain)\n"
                    "    --txgain (Tx gain)\n"
                    "    --rxgain (Rx gain)\n"
                    "    -c (CPU for Tx and Rx streaming - defaults to don't care)\n"
                    "    --txcpu (CPU for Rx streaming - defaults to don't care)\n"
                    "    --rxcpu (CPU for Rx streaming - defaults to don't care)\n"
                    "    --rxpipe (path to the Rx pipe)\n"
                    "    --txpipe (path to the Tx pipe)\n"
                    "    --txfeedbackpipe (path to the Tx feedback pipe - only applies when txpipe is supplied\n)"
                    "    --samppertransactrx (samples per rx transaction\n)"
                    "    --samppertransacttx (samples per tx transaction\n)"
                    "    --forcefulltxbuffer (forces a full tx buffer for each transmission to the tx)"
                    "    -v (enable verbose prints)\n"
                    "    -h (print this help message)\n"
                    "    --help (print this help message)\n");
};

void cleanup(char* device_args, uhd_usrp_handle usrp, uhd_rx_streamer_handle rx_streamer, uhd_rx_metadata_handle rx_md,
            uhd_tx_streamer_handle tx_streamer, uhd_tx_metadata_handle tx_md, bool verbose, int return_code){
    //---- Cleanup Rx ----
    if(rx_streamer != NULL) {
        if (verbose) {
            fprintf(stderr, "Cleaning up RX streamer.\n");
        }
        uhd_rx_streamer_free(&rx_streamer);
    }

    if(rx_md != NULL) {
        if (verbose) {
            fprintf(stderr, "Cleaning up RX metadata.\n");
        }
        uhd_rx_metadata_free(&rx_md);
    }

    //---- Cleanup Tx ----
    if(tx_streamer != NULL) {
        if (verbose) {
            fprintf(stderr, "Cleaning up TX streamer.\n");
        }
        uhd_tx_streamer_free(&tx_streamer);
    }

    free_tx_metadata:
    if(tx_md != NULL) {
        if (verbose) {
            fprintf(stderr, "Cleaning up TX metadata.\n");
        }
        uhd_tx_metadata_free(&tx_md);
    }

    char error_string[512];

    //---- Cleanup USRP ----
    if(usrp != NULL) {
        if (verbose) {
            fprintf(stderr, "Cleaning up USRP.\n");
        }
        if (return_code != EXIT_SUCCESS) {
            uhd_usrp_last_error(usrp, error_string, 512);
            fprintf(stderr, "USRP reported the following error: %s\n", error_string);
        }
        uhd_usrp_free(&usrp);
    }

    if(device_args != NULL) {
        free(device_args);
    }

    fprintf(stderr, (return_code ? "Failure\n" : "Success\n"));
    exit(return_code);
}

void sigint_handler(int code){
    (void)code;
    terminateStatus = true;
}

int main(int argc, char* argv[])
{
    if(uhd_set_thread_priority(uhd_default_thread_priority, true)){
        fprintf(stderr, "Unable to set thread priority. Continuing anyway.\n");
    }

    // ==== Parse CLI Options ====
    // Set Default Options
    int option = 0;
    double freq = 500e6;
    double rate = 1e6;
    int rxCPU = -1;
    int txCPU = -1;
    double txGain = 5.0;
    double rxGain = 5.0;
    char* device_args = NULL;
    size_t rxChannel = 0;
    size_t txChannel = 0;
    char* rxPipeName = NULL;
    char* txPipeName = NULL;
    char* txFeedbackPipeName = NULL;
    bool verbose = false;
    int return_code = EXIT_SUCCESS;
    int samplesPerTransactionRx;
    int samplesPerTransactionTx;
    bool forceFullTxBuffer = false;

    uhd_usrp_handle usrp = NULL;
    uhd_rx_streamer_handle rx_streamer = NULL;
    uhd_rx_metadata_handle rx_md = NULL;
    uhd_tx_streamer_handle tx_streamer = NULL;
    uhd_tx_metadata_handle tx_md = NULL;

    // Process options
    for(int i = 1; i<argc; i++){
        if(strcmp(argv[i], "-a") == 0) {
            i++;
            if(i<argc) {
                device_args = strdup(argv[i]);
            }else{
                print_help();
                return_code = EXIT_FAILURE;
                cleanup(device_args, usrp, rx_streamer, rx_md, tx_streamer, tx_md, verbose, return_code);
            }
        }else if(strcmp(argv[i], "-f") == 0) {
            i++;
            if(i<argc) {
                freq = atof(argv[i]);
            }else{
                print_help();
                return_code = EXIT_FAILURE;
                cleanup(device_args, usrp, rx_streamer, rx_md, tx_streamer, tx_md, verbose, return_code);
            }
        }else if(strcmp(argv[i], "-r") == 0) {
            i++;
            if(i<argc) {
                rate = atof(argv[i]);
            }else{
                print_help();
                return_code = EXIT_FAILURE;
                cleanup(device_args, usrp, rx_streamer, rx_md, tx_streamer, tx_md, verbose, return_code);
            }
        }else if(strcmp(argv[i], "-g") == 0) {
            //This sets both gains.
            i++;
            if(i<argc) {
                double gain = atof(argv[i]);
                txGain = gain;
                rxGain = gain;
            }else{
                print_help();
                return_code = EXIT_FAILURE;
                cleanup(device_args, usrp, rx_streamer, rx_md, tx_streamer, tx_md, verbose, return_code);
            }
        }else if(strcmp(argv[i], "--txgain") == 0 || strcmp(argv[i], "-txgain") == 0) {
            i++;
            if(i<argc) {
                txGain = atof(argv[i]);
            }else{
                print_help();
                return_code = EXIT_FAILURE;
                cleanup(device_args, usrp, rx_streamer, rx_md, tx_streamer, tx_md, verbose, return_code);
            }
        }else if(strcmp(argv[i], "--rxgain") == 0 || strcmp(argv[i], "-rxgain") == 0) {
            i++;
            if (i < argc) {
                rxGain = atof(argv[i]);
            } else {
                print_help();
                return_code = EXIT_FAILURE;
                cleanup(device_args, usrp, rx_streamer, rx_md, tx_streamer, tx_md, verbose, return_code);
            }
        }else if(strcmp(argv[i], "-c") == 0) {
            //This sets both CPUs.
            i++;
            if(i<argc) {
                int cpus = atoi(argv[i]);
                txCPU = cpus;
                rxCPU = cpus;
            }else{
                print_help();
                return_code = EXIT_FAILURE;
                cleanup(device_args, usrp, rx_streamer, rx_md, tx_streamer, tx_md, verbose, return_code);
            }
        }else if(strcmp(argv[i], "--txcpu") == 0 || strcmp(argv[i], "-txcpu") == 0 ) {
            //This sets both CPUs.
            i++;
            if(i<argc) {
                txCPU = atoi(argv[i]);
            }else{
                print_help();
                return_code = EXIT_FAILURE;
                cleanup(device_args, usrp, rx_streamer, rx_md, tx_streamer, tx_md, verbose, return_code);
            }
        }else if(strcmp(argv[i], "--rxcpu") == 0 || strcmp(argv[i], "-rxcpu") == 0 ) {
            //This sets both CPUs.
            i++;
            if(i<argc) {
                rxCPU = atoi(argv[i]);
            }else{
                print_help();
                return_code = EXIT_FAILURE;
                cleanup(device_args, usrp, rx_streamer, rx_md, tx_streamer, tx_md, verbose, return_code);
            }
        }else if(strcmp(argv[i], "--rxpipe") == 0 || strcmp(argv[i], "-rxpipe") == 0) {
            i++;
            if(i<argc) {
                rxPipeName = argv[i];
            }else{
                print_help();
                return_code = EXIT_FAILURE;
                cleanup(device_args, usrp, rx_streamer, rx_md, tx_streamer, tx_md, verbose, return_code);
            }
        }else if(strcmp(argv[i], "--txpipe") == 0 || strcmp(argv[i], "-txpipe") == 0) {
            i++;
            if(i<argc) {
                txPipeName = argv[i];
            }else{
                print_help();
                return_code = EXIT_FAILURE;
                cleanup(device_args, usrp, rx_streamer, rx_md, tx_streamer, tx_md, verbose, return_code);
            }
        }else if(strcmp(argv[i], "--txfeedbackpipe") == 0 || strcmp(argv[i], "-txfeedbackpipe") == 0) {
            i++;
            if(i<argc) {
                txFeedbackPipeName = argv[i];
            }else{
                print_help();
                return_code = EXIT_FAILURE;
                cleanup(device_args, usrp, rx_streamer, rx_md, tx_streamer, tx_md, verbose, return_code);
            }
        }else if(strcmp(argv[i], "--samppertransacttx") == 0 || strcmp(argv[i], "-samppertransacttx") == 0 ) {
            //This sets both CPUs.
            i++;
            if(i<argc) {
                samplesPerTransactionTx = atoi(argv[i]);
            }else{
                print_help();
                return_code = EXIT_FAILURE;
                cleanup(device_args, usrp, rx_streamer, rx_md, tx_streamer, tx_md, verbose, return_code);
            }
        }else if(strcmp(argv[i], "--samppertransactrx") == 0 || strcmp(argv[i], "-samppertransactrx") == 0 ) {
            //This sets both CPUs.
            i++;
            if(i<argc) {
                samplesPerTransactionRx = atoi(argv[i]);
            }else{
                print_help();
                return_code = EXIT_FAILURE;
                cleanup(device_args, usrp, rx_streamer, rx_md, tx_streamer, tx_md, verbose, return_code);
            }
        }else if(strcmp(argv[i], "--forcefulltxbuffer") == 0 || strcmp(argv[i], "-forcefulltxbuffer") == 0 ) {
            forceFullTxBuffer = true;
        }else if(strcmp(argv[i], "-v") == 0) {
            //No need to get the value of this argument
            verbose = true;
        }else if(strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            //No need to get the value of this argument
            print_help();
            cleanup(device_args, usrp, rx_streamer, rx_md, tx_streamer, tx_md, verbose, return_code);
        }else{
            //Default case
            print_help();
            return_code = EXIT_FAILURE;
            cleanup(device_args, usrp, rx_streamer, rx_md, tx_streamer, tx_md, verbose, return_code);
        }
    }

    if (!device_args) {
        device_args = strdup("");
    }

    //Check for required arguments
    if(rxPipeName == NULL & txPipeName == NULL){
        //Nothing to do, exit
        printf("No Rx or Tx pipe specified ... exiting\n");
        return_code = EXIT_FAILURE;
        cleanup(device_args, usrp, rx_streamer, rx_md, tx_streamer, tx_md, verbose, return_code);
    }

    //Set interrupt handler
    signal(SIGINT, &sigint_handler);

    //==== Setup USRP Connection ====

    // Create USRP
    fprintf(stderr, "Creating USRP with args \"%s\"...\n", device_args);
    UHD_API uhd_error uhdStatus = uhd_usrp_make(&usrp, device_args);
    if(uhdStatus){
        printf("Error Creating USRP\n");
        return_code = EXIT_FAILURE;
        cleanup(device_args, usrp, rx_streamer, rx_md, tx_streamer, tx_md, verbose, return_code);
    }

    // ++++ Setup ADC Side ++++
    uhd_stream_cmd_t rx_stream_start_cmd;
    uhd_stream_cmd_t rx_stream_stop_cmd;
    if(rxPipeName != NULL) {
        uhdStatus = uhd_rx_streamer_make(&rx_streamer);
        if(uhdStatus){
            printf("Error Creating Rx Streamer\n");
            return_code = EXIT_FAILURE;
            cleanup(device_args, usrp, rx_streamer, rx_md, tx_streamer, tx_md, verbose, return_code);
        }

        // Create RX metadata
        uhdStatus = uhd_rx_metadata_make(&rx_md);
        if(uhdStatus){
            printf("Error Creating Rx Metadata\n");
            return_code = EXIT_FAILURE;
            cleanup(device_args, usrp, rx_streamer, rx_md, tx_streamer, tx_md, verbose, return_code);
        }

        // Create other necessary structs
        uhd_tune_request_t tune_request = {
                .target_freq = freq,
                .rf_freq_policy = UHD_TUNE_REQUEST_POLICY_AUTO,
                .dsp_freq_policy = UHD_TUNE_REQUEST_POLICY_AUTO,
        };
        uhd_tune_result_t tune_result;

        uhd_stream_args_t stream_args = {
                .cpu_format = "fc32", //Want the received data to be converted to single precision complex floating point
                .otw_format = "sc16", //The actual "On the wire" format is a 16 bit complex integer -> this matches what the ADC supplies
                .args = "",
                .channel_list = &rxChannel,
                .n_channels = 1
        };

        rx_stream_start_cmd.stream_mode = UHD_STREAM_MODE_START_CONTINUOUS;
        rx_stream_start_cmd.stream_now = true;

        rx_stream_stop_cmd.stream_mode = UHD_STREAM_MODE_STOP_CONTINUOUS;
        rx_stream_stop_cmd.stream_now = true;

        // Set rate
        fprintf(stderr, "Setting RX Rate: %f...\n", rate);
        uhdStatus = uhd_usrp_set_rx_rate(usrp, rate, rxChannel);
        if(uhdStatus){
            printf("Error Setting Rx Rate\n");
            return_code = EXIT_FAILURE;
            cleanup(device_args, usrp, rx_streamer, rx_md, tx_streamer, tx_md, verbose, return_code);
        }

        // See what rate actually is
        uhdStatus = uhd_usrp_get_rx_rate(usrp, rxChannel, &rate);
        if(uhdStatus){
            printf("Error Getting Rx Rate\n");
            return_code = EXIT_FAILURE;
            cleanup(device_args, usrp, rx_streamer, rx_md, tx_streamer, tx_md, verbose, return_code);
        }
        fprintf(stderr, "Actual RX Rate: %f...\n", rate);

        // Set gain
        fprintf(stderr, "Setting RX Gain: %f dB...\n", rxGain);
        uhdStatus =  uhd_usrp_set_rx_gain(usrp, rxGain, rxChannel, "");
        if(uhdStatus){
            printf("Error Setting Rx Gain\n");
            return_code = EXIT_FAILURE;
            cleanup(device_args, usrp, rx_streamer, rx_md, tx_streamer, tx_md, verbose, return_code);
        }

        // See what gain actually is
        uhdStatus =  uhd_usrp_get_rx_gain(usrp, rxChannel, "", &rxGain);
        if(uhdStatus){
            printf("Error Getting Rx Gain\n");
            return_code = EXIT_FAILURE;
            cleanup(device_args, usrp, rx_streamer, rx_md, tx_streamer, tx_md, verbose, return_code);
        }
        fprintf(stderr, "Actual RX Gain: %f...\n", rxGain);

        // Set frequency
        fprintf(stderr, "Setting RX frequency: %f MHz...\n", freq / 1e6);
        uhdStatus = uhd_usrp_set_rx_freq(usrp, &tune_request, rxChannel, &tune_result);
        if(uhdStatus){
            printf("Error Setting Rx Frequency\n");
            return_code = EXIT_FAILURE;
            cleanup(device_args, usrp, rx_streamer, rx_md, tx_streamer, tx_md, verbose, return_code);
        }

        // See what frequency actually is
        uhdStatus = uhd_usrp_get_rx_freq(usrp, rxChannel, &freq);
        if(uhdStatus){
            printf("Error Getting Rx Frequency\n");
            return_code = EXIT_FAILURE;
            cleanup(device_args, usrp, rx_streamer, rx_md, tx_streamer, tx_md, verbose, return_code);
        }
        fprintf(stderr, "Actual RX frequency: %f MHz...\n", freq / 1e6);

        // Set up streamer
        stream_args.channel_list = &rxChannel;
        uhdStatus = uhd_usrp_get_rx_stream(usrp, &stream_args, rx_streamer);
        if(uhdStatus){
            printf("Error Getting Rx Stream\n");
            return_code = EXIT_FAILURE;
            cleanup(device_args, usrp, rx_streamer, rx_md, tx_streamer, tx_md, verbose, return_code);
        }
    }

    //+++ Setup DAC Side +++
    if(txPipeName != NULL){
        // Create TX streamer

        uhdStatus = uhd_tx_streamer_make(&tx_streamer);
        if(uhdStatus){
            printf("Error Creating Tx Streamer\n");
            return_code = EXIT_FAILURE;
            cleanup(device_args, usrp, rx_streamer, rx_md, tx_streamer, tx_md, verbose, return_code);
        }

        // Create TX metadata
        uhdStatus = uhd_tx_metadata_make(&tx_md, false, 0, 0.1, true, false);
        if(uhdStatus){
            printf("Error Creating Tx Metadata\n");
            return_code = EXIT_FAILURE;
            cleanup(device_args, usrp, rx_streamer, rx_md, tx_streamer, tx_md, verbose, return_code);
        }

        // Create other necessary structs
        uhd_tune_request_t tune_request = {
                .target_freq = freq,
                .rf_freq_policy = UHD_TUNE_REQUEST_POLICY_AUTO,
                .dsp_freq_policy = UHD_TUNE_REQUEST_POLICY_AUTO
        };
        uhd_tune_result_t tune_result;

        uhd_stream_args_t stream_args = {
                .cpu_format = "fc32", //Want the received data to be converted to single precision complex floating point
                .otw_format = "sc16", //The actual "On the wire" format is a 16 bit complex integer -> this matches what the DAC IP expects
                .args = "",
                .channel_list = &txChannel,
                .n_channels = 1
        };

        // Set rate
        fprintf(stderr, "Setting TX Rate: %f...\n", rate);
        uhdStatus = uhd_usrp_set_tx_rate(usrp, rate, txChannel);
        if(uhdStatus){
            printf("Error Setting Tx Rate\n");
            return_code = EXIT_FAILURE;
            cleanup(device_args, usrp, rx_streamer, rx_md, tx_streamer, tx_md, verbose, return_code);
        }

        // See what rate actually is
        uhdStatus = uhd_usrp_get_tx_rate(usrp, txChannel, &rate);
        if(uhdStatus){
            printf("Error Getting Tx Rate\n");
            return_code = EXIT_FAILURE;
            cleanup(device_args, usrp, rx_streamer, rx_md, tx_streamer, tx_md, verbose, return_code);
        }
        fprintf(stderr, "Actual TX Rate: %f...\n\n", rate);

        // Set gain
        fprintf(stderr, "Setting TX Gain: %f db...\n", txGain);
        uhdStatus = uhd_usrp_set_tx_gain(usrp, txGain, 0, "");
        if(uhdStatus){
            printf("Error Setting Tx Gain\n");
            return_code = EXIT_FAILURE;
            cleanup(device_args, usrp, rx_streamer, rx_md, tx_streamer, tx_md, verbose, return_code);
        }

        // See what gain actually is
        uhdStatus = uhd_usrp_get_tx_gain(usrp, txChannel, "", &txGain);
        if(uhdStatus){
            printf("Error Getting Tx Gain\n");
            return_code = EXIT_FAILURE;
            cleanup(device_args, usrp, rx_streamer, rx_md, tx_streamer, tx_md, verbose, return_code);
        }
        fprintf(stderr, "Actual TX Gain: %f...\n", txGain);

        // Set frequency
        fprintf(stderr, "Setting TX frequency: %f MHz...\n", freq / 1e6);
        uhdStatus = uhd_usrp_set_tx_freq(usrp, &tune_request, txChannel, &tune_result);
        if(uhdStatus){
            printf("Error Setting Tx Frequency\n");
            return_code = EXIT_FAILURE;
            cleanup(device_args, usrp, rx_streamer, rx_md, tx_streamer, tx_md, verbose, return_code);
        }

        // See what frequency actually is
        uhdStatus = uhd_usrp_get_tx_freq(usrp, txChannel, &freq);
        if(uhdStatus){
            printf("Error Getting Tx Frequency\n");
            return_code = EXIT_FAILURE;
            cleanup(device_args, usrp, rx_streamer, rx_md, tx_streamer, tx_md, verbose, return_code);
        }

        fprintf(stderr, "Actual TX frequency: %f MHz...\n", freq / 1e6);

        // Set up streamer
        stream_args.channel_list = &txChannel;
        uhdStatus = uhd_usrp_get_tx_stream(usrp, &stream_args, tx_streamer);
        if(uhdStatus){
            printf("Error Getting Tx Stream\n");
            return_code = EXIT_FAILURE;
            cleanup(device_args, usrp, rx_streamer, rx_md, tx_streamer, tx_md, verbose, return_code);
        }
    }

    //TODO: Create Signal Handler
    pthread_t txPThread;
    txHandlerArgs_t txArgs;
    pthread_attr_t txThreadAttributes;
    cpu_set_t txCPUSet;

    if(txPipeName != NULL){
        //Create and launch Tx Thread
        //Create Thread Parameters
        int attrStatus = pthread_attr_init(&txThreadAttributes);
        if(attrStatus != 0)
        {
            printf("Error creating Tx pthread attribute");
            return_code = EXIT_FAILURE;
            cleanup(device_args, usrp, rx_streamer, rx_md, tx_streamer, tx_md, verbose, return_code);
        }

        CPU_ZERO(&txCPUSet);
        CPU_SET(txCPU, &txCPUSet);
        int setAfinityStatus = pthread_attr_setaffinity_np(&txThreadAttributes, sizeof(cpu_set_t), &txCPUSet);
        if(setAfinityStatus != 0)
        {
            printf("Error creating Tx pthread core affinity");
            return_code = EXIT_FAILURE;
            cleanup(device_args, usrp, rx_streamer, rx_md, tx_streamer, tx_md, verbose, return_code);
        }

        //Create Tx Thread Args
        txArgs.terminateStatus = &terminateStatus; //Used to periodically check if thread should terminate
        txArgs.txPipeName = txPipeName;
        txArgs.txFeedbackPipeName = txFeedbackPipeName;
        txArgs.tx_streamer = tx_streamer;
        txArgs.tx_md = tx_md;
        txArgs.samplesPerTransactTx = samplesPerTransactionTx;
        txArgs.forceFullTxBuffer = forceFullTxBuffer;
        txArgs.verbose = verbose;

        int threadStartStatus = pthread_create(&txPThread, &txThreadAttributes, txHandler, &txArgs);
        if(threadStartStatus != 0)
        {
            printf("Error creating Tx thread");
            perror(NULL);
            return_code = EXIT_FAILURE;
            cleanup(device_args, usrp, rx_streamer, rx_md, tx_streamer, tx_md, verbose, return_code);
        }
    }

    pthread_t rxPThread;
    pthread_attr_t rxThreadAttributes;
    cpu_set_t rxCPUSet;
    rxHandlerArgs_t rxArgs;
    bool rxWasRunning = false;

    if(rxPipeName != NULL){
        //Create and launch Rx Thread
        //Create Thread Parameters
        int attrStatus = pthread_attr_init(&rxThreadAttributes);
        if(attrStatus != 0)
        {
            printf("Error creating Rx pthread attribute");
            return_code = EXIT_FAILURE;
            cleanup(device_args, usrp, rx_streamer, rx_md, tx_streamer, tx_md, verbose, return_code);
        }

        CPU_ZERO(&rxCPUSet);
        CPU_SET(rxCPU, &rxCPUSet);
        int setAfinityStatus = pthread_attr_setaffinity_np(&rxThreadAttributes, sizeof(cpu_set_t), &rxCPUSet);
        if(setAfinityStatus != 0)
        {
            printf("Error creating Rx pthread core affinity");
            return_code = EXIT_FAILURE;
            cleanup(device_args, usrp, rx_streamer, rx_md, tx_streamer, tx_md, verbose, return_code);
        }

        //Create Rx Thread Args
        rxArgs.terminateStatus=&terminateStatus;
        rxArgs.rxPipeName=rxPipeName;
        rxArgs.rx_streamer=rx_streamer;
        rxArgs.rx_md=rx_md;
        rxArgs.rx_stream_start_cmd=rx_stream_start_cmd;
        rxArgs.rx_stream_stop_cmd=rx_stream_stop_cmd;
        rxArgs.sendStopCmd=true;
        rxArgs.samplesPerTransactRx=samplesPerTransactionRx;
        rxArgs.verbose=verbose;
        rxArgs.wasRunning=&rxWasRunning;

        int threadStartStatus = pthread_create(&rxPThread, &rxThreadAttributes, rxHandler, &rxArgs);
        if(threadStartStatus != 0)
        {
            printf("Error creating Rx thread");
            perror(NULL);
            return_code = EXIT_FAILURE;
            cleanup(device_args, usrp, rx_streamer, rx_md, tx_streamer, tx_md, verbose, return_code);
        }
    }

    //Join threads
    if(txPipeName != NULL){
        void *result;
        int joinStatus = pthread_join(txPThread, &result);
        if(joinStatus != 0)
        {
            printf("Could not join Tx thread");
            perror(NULL);
            return_code = EXIT_FAILURE;
            cleanup(device_args, usrp, rx_streamer, rx_md, tx_streamer, tx_md, verbose, return_code);
        }
    }

    if(rxPipeName != NULL){
        void *result;
        int joinStatus = pthread_join(rxPThread, &result);
        if(joinStatus != 0)
        {
            printf("Could not join Rx thread");
            perror(NULL);
            return_code = EXIT_FAILURE;
            cleanup(device_args, usrp, rx_streamer, rx_md, tx_streamer, tx_md, verbose, return_code);
        }
    }

    // Cleanup
    cleanup(device_args, usrp, rx_streamer, rx_md, tx_streamer, tx_md, verbose, return_code);

    //Will not get here, will exit in cleanup
    return return_code;
}