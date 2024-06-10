#ifndef __BS_CONNECTOR_H__
#define __BS_CONNECTOR_H__

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>


// send dummy data instead of reading BS metrics
#define DEBUG 0
#define JSON_FORMAT 0
#define MAX_REPORT_PAYLOAD 300
#define LINES_TO_READ 2

typedef struct {
    float* timer;
    uint32_t* ric_req_id;
} thread_args;

void handleTimer(float* timer, uint32_t* ric_req_id);
void *periodicDataReportThread(void* arg);
void periodicDataReport(uint32_t ric_req_id_deref);
void log_message(char* message, char* message_type, int len);
void stop_data_reporting_nrt_ric(void);

// declare the rust functions
extern char* get_tx_string_c(uint32_t lines_to_read, bool json_format);

#endif
