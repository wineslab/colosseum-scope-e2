#ifndef __BS_CONNECTOR_H__
#define __BS_CONNECTOR_H__

#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>
#include <stdio.h>

// send dummy data instead of reading BS metrics
#define DEBUG 0
#define LINES_TO_READ 2

typedef struct {
    float* timer;
    uint32_t* ric_req_id;
} thread_args;

void handleTimer(float* timer, uint32_t* ric_req_id);
void *periodicDataReport(void* arg);
void log_message(char* message, char* message_type, int len);
void stop_data_reporting_nrt_ric(void) ;

#endif
