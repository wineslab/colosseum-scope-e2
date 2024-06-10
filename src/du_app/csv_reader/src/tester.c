#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>


#define JSON_FORMAT 1
#define MAX_REPORT_PAYLOAD 300
#define LINES_TO_READ 2


extern char* get_tx_string_c(uint32_t lines_to_read, bool json_format);


// get and send metrics to xApp
int tester(void) {

  // char *payload = NULL;
  // int lines_to_read = LINES_TO_READ;
  // get_tx_string(&payload, lines_to_read);

  uint32_t lines_to_read = LINES_TO_READ;
  char *payload = get_tx_string_c(lines_to_read, JSON_FORMAT);

  if (payload != NULL) {
    int payload_len = strlen(payload);
    printf("Payload of len %d: %s\n", payload_len, payload);

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
      printf("String %d: %s\n", i, chunk);
    }

    printf("Sent RICIndicationReport\n");

    // if (chunk && chunk != NULL) {
    //   free(chunk);
    //   chunk = NULL;
    // }

    // if (payload != NULL) {
    //   free(payload);
    //   payload = NULL;
    // }
  }

  return 0;
}