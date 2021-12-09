#ifndef __CSV_READER_H__
#define __CSV_READER_H__

#define DELTA_TS_S 2
#define MAX_BUF_SIZE 1024
#define MAX_REPORT_PAYLOAD 300
// #define METRICS_DIR "./"
#define METRICS_DIR "/root/radio_code/scope_config/metrics/csv/"
#define METRICS_PRESET 1
#define CSV_DEBUG 0

// metrics structure
typedef struct bs_metrics {
    // configuration
    long unsigned int timestamp;
    int num_ues;
    long long unsigned int imsi;
    uint16_t rnti;
    uint8_t slicing_enabled;
    uint8_t slice_id;
    uint8_t slice_prb;
    float power_multiplier;
    uint8_t scheduling_policy;

    // downlink
    float dl_mcs;
    uint8_t dl_n_samples;
    uint32_t dl_buffer_bytes;
    double tx_brate_downlink_Mbps;
    uint16_t tx_pkts_downlink;
    float tx_errors_downlink_perc;
    float dl_cqi;

    // uplink
    float ul_mcs;
    uint8_t ul_n_samples;
    uint32_t ul_buffer_bytes;
    double rx_brate_downlink_Mbps;
    uint16_t rx_pkts_downlink;
    float rx_errors_downlink_perc;
    float ul_rssi;
    float ul_sinr;
    uint8_t phr;

    // prb
    uint16_t sum_requested_prbs;
    uint16_t sum_granted_prbs;

    // other
    uint8_t dl_pmi;
    uint8_t dl_ri;
    uint8_t ul_n;
    float ul_turbo_iters;

} bs_metrics_t;


void readMetrics(FILE *fp, bs_metrics_t *metrics);
void readMetricsInteractive(FILE *fp, char (*output_string)[MAX_BUF_SIZE], int metrics_preset);
void readLastMetricsLines(char *file_name, int to_read, char **output_string, int skip_header);
int getDirContent(char *directory_name, char (*dir_content)[MAX_BUF_SIZE]);
void get_tx_string(char **send_metrics, int lines_to_read);
unsigned long int get_time_milliseconds(void);
void remove_substr (char *string, char *sub);


#endif
