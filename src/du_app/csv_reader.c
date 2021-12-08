#include <dirent.h>
#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "csv_reader.h"


// read line into structure
void readMetrics(FILE *fp, bs_metrics_t *metrics) {

  fscanf(fp, "%lu,%d,%llu,%" PRIu16 ",,\
    %" PRIu8 ",%" PRIu8 ",%" PRIu8 ",%f,%" PRIu8 ",,\
    %f,%" PRIu8 ",%" PRIu32 ",%lf,%" PRIu16 ",%f,%f,,\
    %f,%" PRIu8 ",%" PRIu32 ",%lf,%" PRIu16 ",%f,%f,%f,%" PRIu8 ",,\
    %" PRIu16 ",%" PRIu16 ",,\
    %" PRIu8 ",%" PRIu8 ",%" PRIu8 ",%f",
    &metrics->timestamp, &metrics->num_ues, &metrics->imsi, &metrics->rnti,
    &metrics->slicing_enabled, &metrics->slice_id, &metrics->slice_prb, &metrics->power_multiplier, &metrics->scheduling_policy,
    &metrics->dl_mcs, &metrics->dl_n_samples, &metrics->dl_buffer_bytes, &metrics->tx_brate_downlink_Mbps, &metrics->tx_pkts_downlink, &metrics->tx_errors_downlink_perc, &metrics->dl_cqi,
    &metrics->ul_mcs, &metrics->ul_n_samples, &metrics->ul_buffer_bytes, &metrics->rx_brate_downlink_Mbps, &metrics->rx_pkts_downlink, &metrics->rx_errors_downlink_perc, &metrics->ul_rssi, &metrics->ul_sinr, &metrics->phr,
    &metrics->sum_requested_prbs, &metrics->sum_granted_prbs,
    &metrics->dl_pmi, &metrics->dl_ri, &metrics->ul_n, &metrics->ul_turbo_iters);
}


// read metrics into bs_metrics structure and assemble line to send
// done for future extension in which metrics are selectively sent
// metrics_preset selects which metrics to send. E.g., 0: send all metrics
void readMetricsInteactive(FILE *fp, char (*output_string)[MAX_BUF_SIZE], int metrics_preset) {

  bs_metrics_t metrics;
  readMetrics(fp, &metrics);

  float ratio_granted_req_prb;
  char selected_metrics[MAX_BUF_SIZE];

  // only send if sum_requested_prbs > 0 or if prbs are granted anyway
  if (metrics.sum_requested_prbs > 0 || (metrics.sum_requested_prbs == 0 && metrics.sum_granted_prbs > 0) || CSV_DEBUG) {
    switch(metrics_preset) {
      case 0:
        sprintf(selected_metrics, "%lu,%d,%llu,%" PRIu16 ","\
          "%" PRIu8 ",%" PRIu8 ",%" PRIu8 ",%.2f,%" PRIu8 ","\
          "%.2f,%" PRIu8 ",%" PRIu32 ",%.2lf,%" PRIu16 ",%.2f,%.2f,"\
          "%.2f,%" PRIu8 ",%" PRIu32 ",%.2lf,%" PRIu16 ",%.2f,%.2f,%.2f,%" PRIu8 ","\
          "%" PRIu16 ",%" PRIu16 ","\
          "%" PRIu8 ",%" PRIu8 ",%" PRIu8 ",%.2f",
          metrics.timestamp, metrics.num_ues, metrics.imsi, metrics.rnti,
          metrics.slicing_enabled, metrics.slice_id, metrics.slice_prb, metrics.power_multiplier, metrics.scheduling_policy,
          metrics.dl_mcs, metrics.dl_n_samples, metrics.dl_buffer_bytes, metrics.tx_brate_downlink_Mbps, metrics.tx_pkts_downlink, metrics.tx_errors_downlink_perc, metrics.dl_cqi,
          metrics.ul_mcs, metrics.ul_n_samples, metrics.ul_buffer_bytes, metrics.rx_brate_downlink_Mbps, metrics.rx_pkts_downlink, metrics.rx_errors_downlink_perc, metrics.ul_rssi, metrics.ul_sinr, metrics.phr,
          metrics.sum_requested_prbs, metrics.sum_granted_prbs,
          metrics.dl_pmi, metrics.dl_ri, metrics.ul_n, metrics.ul_turbo_iters);

          break;
      case 1:
        if (metrics.sum_requested_prbs > 0) {
          ratio_granted_req_prb = ((float) metrics.sum_granted_prbs) / ((float) metrics.sum_requested_prbs);
        }
        else {
          ratio_granted_req_prb = 1;
        }

        if (ratio_granted_req_prb > 1) {
          ratio_granted_req_prb = 1;
        }
        else if (CSV_DEBUG && isnan(ratio_granted_req_prb)) {
          ratio_granted_req_prb = 0;
        }

        //////////////////////////////////////////////////////////////////////////
        //
        // ordering metrics in same order as agent parser (numbers mark the order)
        // timestamp is only used for metrics freshness and removed before sending data
        //
        // metric_dict = {"dl_buffer [bytes]": 1, "tx_brate downlink [Mbps]": 2,
        //                "ratio_req_granted": 3, "slice_id": 0, "slice_prb": 4}
        //
        //////////////////////////////////////////////////////////////////////////

        sprintf(selected_metrics, "%lu,%" PRIu8 ",%" PRIu32 ",%.2lf,%.2f,%" PRIu8 ",%" PRIu16 "",
          metrics.timestamp, metrics.slice_id, metrics.dl_buffer_bytes, metrics.tx_brate_downlink_Mbps,
          ratio_granted_req_prb, metrics.slice_prb, metrics.tx_pkts_downlink);

        printf("selected_metrics %s\n", selected_metrics);

        break;
      default:
        printf("readMetricsInteactive: Preset %d unknown\n", metrics_preset);
    }
  }

  strcpy(*output_string, selected_metrics);
}


// read last lines from file
void readLastMetricsLines(char *file_name, int to_read, char **output_string, int skip_header) {

  FILE *fp;
  fp = fopen(file_name, "r");

  int max_metrics_buf = 1000;
  int min_metrics_to_send = 1;

  long unsigned int curr_ts;

  if (CSV_DEBUG) {
    curr_ts = 1602173692928;
  }
  else {
    curr_ts = get_time_milliseconds();
  }

  if (!fp) {
    printf("fp is NULL, filename %s\n", file_name);
    return;
  }
  else {
    printf("%lu: reading %s\n", curr_ts, file_name);
  }

  int tot_lines;
  for (tot_lines = 0; feof(fp) == 0; ++tot_lines) {
    fscanf(fp, "%*[^\n]\n", NULL);
  }

  // rewind file pointer
  rewind(fp);

  // initialize array
  // + 1 to also account for header
  int array_len = tot_lines;
  if (!skip_header) {
    array_len += 1;
  }
  char metrics_array[array_len][max_metrics_buf];

  // skip first lines_num - to_read lines
  // and read last to_read lines
  int j = 0;
  int tot_len = 0;
  for (int i = 0; i < tot_lines; ++i) {
    if ((i == 0 && !skip_header) || i >= tot_lines - to_read) {
      if (i == 0) {
        fscanf(fp, "%[^\n]\n", metrics_array[j]);
      }
      else {
        readMetricsInteactive(fp, &(metrics_array[j]), METRICS_PRESET);
      }

      int line_len = strlen(metrics_array[j]);
      metrics_array[j][line_len] = '\n';
      metrics_array[j][line_len + 1] = '\0';

      tot_len += strlen(metrics_array[j++]);
    }
    else {
      fscanf(fp, "%*[^\n]\n", NULL);
    }
  }

  fclose(fp);

  *output_string = (char*) calloc(tot_len + 1, sizeof(char*));

  int curr_pos = 0;
  int valid_metrics = 0;

  // copy header
  if (!skip_header) {
    strcpy(*output_string, metrics_array[0]);
    curr_pos += strlen(metrics_array[0]);
  }

  //for (int i = 0; i < to_read + 1; ++i) {
  for (int i = 0; i < j; ++i) {
    if (i == 0 && !skip_header) {
      continue;
    }

    // get metric timestamp
    long unsigned int metric_ts = 0;
    sscanf(metrics_array[i], "%lu", &metric_ts);

    // printf("i %d, timestamp %lu, metrics_array[i] %s\n", i, metric_ts, metrics_array[i]);

    // save it if recent enough
    if ((curr_ts - metric_ts) / 1000.0 <= DELTA_TS_S) {
      // skip if empty line
      if (strcmp(metrics_array[i], "\n") == 0 || strlen(metrics_array[i]) == 0) {
        continue;
      }

      // strip timestamp if METRICS_PRESET is 1
      if (METRICS_PRESET == 1) {
        char tmp_ts[100];
        sprintf(tmp_ts, "%lu", metric_ts);
        strcat(tmp_ts, ",");
        remove_substr(metrics_array[i], tmp_ts);
        // printf("tmp_ts %s\nmetrics_array[i] %s\n", tmp_ts, metrics_array[i]);
      }

      if (skip_header && valid_metrics == 0) {
        strcpy(*output_string, metrics_array[i]);
      }
      else {
        //strcat(&(*output_string)[curr_pos], metrics_array[i]);
        strcat(*output_string + curr_pos, metrics_array[i]);
      }

      curr_pos += strlen(metrics_array[i]);
      valid_metrics += 1;

      printf("len metrics_array[i]: %d\n", strlen(metrics_array[i]));
    }
  }

  printf("valid_metrics %d\noutput_string\n---%s---\n", valid_metrics, output_string);

  if (valid_metrics < 1) {
    printf("Freeing inside readLastMetricsLines\n");
    printf("to_read %d, j %d\n", to_read, j);
    free(*output_string);
    *output_string = NULL;
    printf("Freed\n");
  }
  else if (valid_metrics < to_read) {
    printf("Reallocating inside readLastMetricsLines\n");
    // reallocate output_string accordingly
    *output_string = (char*) realloc(*output_string, (strlen(*output_string) + 1) * sizeof(char*));
    printf("Reallocated\n");
  }
}


// get content of specified directory
int getDirContent(char *directory_name, char (*dir_content)[MAX_BUF_SIZE]) {

  DIR *ptr;
  struct dirent *directory;

  ptr = opendir(directory_name);

  int num_el = 0;
  while((directory = readdir(ptr)) != NULL) {
    char tmp_str[MAX_BUF_SIZE];
    sscanf(directory->d_name, "%s", tmp_str);

    char* token = strtok(tmp_str, "_");

    char imsi[MAX_BUF_SIZE];
    strcpy(imsi, token);
    token = strtok(NULL, "_");
    if (token != NULL) {
      if (strlen(imsi) > 4 && strcmp(token, "metrics.csv") == 0) {
        strcpy(dir_content[num_el++], directory->d_name);
      }
    }
  }

  closedir(ptr);

  return num_el;
}


// read and assemble metrics to send
void get_tx_string(char **send_metrics, int lines_to_read) {

  int curr_pos = 0;

  char dir_content[1000][MAX_BUF_SIZE];
  int dir_el;
  dir_el = getDirContent(METRICS_DIR, dir_content);

  char *metrics_string = NULL;
  for (int i = 0; i < dir_el; ++i) {
    // assemble path of file to read
    char file_path[1000] = METRICS_DIR;
    strcat(file_path, dir_content[i]);

    // read metrics, always skip header
    readLastMetricsLines(file_path, lines_to_read, &metrics_string, 1);

    if (metrics_string) {
      int metrics_size = strlen(metrics_string);

      if (!(*send_metrics)) {
        *send_metrics = (char*) calloc(metrics_size, sizeof(char*));
        strcpy(*send_metrics, metrics_string);
      }
      else {
        *send_metrics = (char*) realloc(*send_metrics, (strlen(*send_metrics) + metrics_size) * sizeof(char*));
        memset(*send_metrics + curr_pos, '\0', metrics_size * sizeof(char*));

        strcat(*send_metrics, metrics_string);
      }

      curr_pos += metrics_size;

      free(metrics_string);
      metrics_string = NULL;
    }
  }
}


// return current time in milliseconds since the EPOCH
long unsigned int get_time_milliseconds(void) {

  long unsigned int time_ms;
  struct timespec spec;

  clock_gettime(CLOCK_REALTIME, &spec);

  time_ms  = spec.tv_sec * 1000;
  time_ms += round(spec.tv_nsec / 1.0e6);

  return time_ms;
}


// remove substring from string
void remove_substr (char *string, char *sub) {
    char *match;
    int len = strlen(sub);
    while ((match = strstr(string, sub))) {
        *match = '\0';
        strcat(string, match+len);
    }
}

// tester function
int csv_tester(void) {

  char *send_metrics = NULL;
  int lines_to_read = 2;

  get_tx_string(&send_metrics, lines_to_read);

  if (send_metrics) {
    printf("len %d\n%s", strlen(send_metrics), send_metrics);

    // split if more than maximum payload for ric indication report
    if (strlen(send_metrics) > MAX_REPORT_PAYLOAD) {
      char *tmp_buf = NULL;
      tmp_buf = (char*) calloc(MAX_REPORT_PAYLOAD + 1, sizeof(char));

      // send in chunks, append
      for (int i = 0; i < strlen(send_metrics); i += MAX_REPORT_PAYLOAD) {
        memset(tmp_buf, 0, MAX_REPORT_PAYLOAD + 1);

        // add 'm' at the beginning to indicate there are more chunks
        int offset = 0;
        if (i + MAX_REPORT_PAYLOAD < strlen(send_metrics)) {
          strcpy(tmp_buf, "m");
          offset = 1;
        }

        strncpy(tmp_buf + offset, send_metrics + i, MAX_REPORT_PAYLOAD);

        printf("Chunk\n%s\n\n", tmp_buf);
      }

      free(tmp_buf);
      tmp_buf = NULL;
    }
  }

  return 0;
}
