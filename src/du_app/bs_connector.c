#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <inttypes.h>

#include "bs_connector.h"

int report_data_nrt_ric = 1;


// handle timer got from RIC Subscription Request
// timer is in seconds
void handleTimer(float* timer, uint32_t* ric_req_id) {

  pthread_t thread;
  printf("Handle timer %f seconds, ricReqId %" PRIu32 "\n", timer[0], ric_req_id[0]);

  // populate thread arguments
  thread_args *t_args = (thread_args*) calloc(1, sizeof(thread_args));
  t_args->timer = timer;
  t_args->ric_req_id = ric_req_id;

  // start thread
  report_data_nrt_ric = 1;
  if (pthread_create(&thread, NULL, periodicDataReportThread, (void *) t_args) != 0) {
    printf("Error creating thread\n");
  }

  printf("periodicDataReport thread created successfully\n");
}


void *periodicDataReportThread(void* arg) {

  thread_args *t_args = (thread_args*) arg;

  // retrieve timer
  float* timer = t_args->timer;
  float timer_deref = timer[0];
  printf("timer expired, timer_deref %f\n", timer_deref);

  // retrieve ricReqId
  uint32_t* ric_req_id = t_args->ric_req_id;
  uint32_t ric_req_id_deref = ric_req_id[0];
  printf("ricReqId %" PRIu32 "\n", ric_req_id_deref);

  while (report_data_nrt_ric) {
    periodicDataReport(ric_req_id_deref);
    usleep(timer_deref * 1000000);
  }
}


// function to periodically report data
void periodicDataReport(uint32_t ric_req_id_deref) {

  if (DEBUG) {
    // debug
    printf("Debug mode\n");
    char* payload = NULL;
    if (JSON_FORMAT) {
      payload = (char*) "{\"timestamp\":1602706183796,\"slice_id\":0,\"dl_bytes\":53431,\"dl_thr_mbps\":2.39,\"ratio_granted_req_prb\":0.02,\"slice_prb\":6,\"dl_pkts\":200}";
    }
    else {
      payload = (char*) "0,1,2,3,4,5\n1,6,7,8,9,10\n2,11,12,13,14,15";
    }

    BuildAndSendRicIndicationReport(payload, strlen(payload), ric_req_id_deref);
  }
  else {
    sendMetricsXapp(ric_req_id_deref);
  }
}


// get and send metrics to xApp
void sendMetricsXapp(uint32_t ric_req_id) {

  uint32_t lines_to_read = LINES_TO_READ;
  char *payload = get_tx_string_c(lines_to_read, JSON_FORMAT);

  if (payload != NULL) {
    int payload_len = strlen(payload);

    // split if more than maximum payload for ric indication report
    char *chunk = NULL;
    chunk = (char*) calloc(MAX_REPORT_PAYLOAD + 1, sizeof(char));

    // send in chunks, append
    for (int i = 0; i < payload_len; i += MAX_REPORT_PAYLOAD) {
      memset(chunk, 0, MAX_REPORT_PAYLOAD + 1);

      int offset = 0;
      // add random string at the beginning to indicate there are more chunks
      // NOTE: need to handle this at the xApp side
      char* more_data_signal = "mJQCx";
      int more_data_signal_len = strlen(more_data_signal);
      if (i + MAX_REPORT_PAYLOAD < payload_len) {
        strcpy(chunk, more_data_signal);
        offset = more_data_signal_len;
      }

      strncpy(chunk + offset, payload + i, MAX_REPORT_PAYLOAD);

      BuildAndSendRicIndicationReport(chunk, strlen(chunk), ric_req_id);
    }

    printf("Sent RICIndicationReport\n");

    if (chunk != NULL) {
      free(chunk);
      chunk = NULL;
    }
  }
}


// log message on file
void log_message(char* message, char* message_type, int len) {

  FILE *fp;
  char filename[100] = "/logs/du_l2.log";

  char buffer[26];
  int millisec;
  struct tm* tm_info;
  struct timeval tv;

  gettimeofday(&tv, NULL);

  millisec = lrint(tv.tv_usec/1000.0); // Round to nearest millisec
  if (millisec>=1000) { // Allow for rounding up to nearest second
    millisec -=1000;
    tv.tv_sec++;
  }

  tm_info = localtime(&tv.tv_sec);

  strftime(buffer, 26, "%Y:%m:%d %H:%M:%S", tm_info);

  fp = fopen(filename, "a+");

  if (fp == NULL) {
    printf("ERROR: fp is NULL\n");
    return;
  }

  const int msg_len = len;
  char msg_copy[msg_len];
  strcpy(msg_copy, message);

  for (int i = 0; i < msg_len; i++)
  {
    if (message[i] == '\n') {
       msg_copy[i] = 'n';
    }
  }

  // print to console and log on file
  printf("%s,%03d\t%s\t%d\t%s\n", buffer, millisec, message_type, len, msg_copy);
  fprintf(fp, "%s,%03d\t%s\t%d\t%s\n", buffer, millisec, message_type, len, msg_copy);

  fclose(fp);
}


// terminate periodic thread that reports data to near real-time RIC
void stop_data_reporting_nrt_ric(void) {
  printf("Terminating data reporting to non real-time RIC\n");
  report_data_nrt_ric = 0;
}
