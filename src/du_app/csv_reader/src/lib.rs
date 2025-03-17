use std::cmp;
use std::collections::HashMap;
use std::ffi::CString;
use std::fs;
use std::fs::File;
use std::io;
use std::io::{BufRead, BufReader};
use std::os::raw::c_char;
use std::path::Path;
use std::time::{SystemTime, UNIX_EPOCH};


static CSV_METRICS_NUM: u8 = 31;
static DELTA_TS_S: u8 = 2;
static METRICS_DIR: &str = "/root/radio_code/scope_config/metrics/csv/";
static METRICS_PRESET: u32 = 1;
static TIMESTAMP_DEBUG: bool = false;


#[repr(u8)]
enum ReportMetrics {
  Ts = 0,               // timestamp
  UeNum = 1,            // num_ues
  Imsi = 2,             // imsi
  Rnti = 3,             // rnti
  SlEn = 5,             // slicing_enabled
  SlId = 6,             // slice_id
  SlPrb = 7,            // slice_prb
  PowMult = 8,          // power_multiplier
  SchedPol = 9,         // scheduling_policy
  DlMcs = 11,            // dl_mcs
  DlSam = 12,           // dl_n_samples
  DlBytes = 13,         // dl_buffer_bytes
  DlThr = 14,           // tx_brate_downlink_Mbps
  DlPkts = 15,          // tx_pkts_downlink
  DlErr = 16,           // tx_errors_downlink_perc
  DlCqi = 17,           // dl_cqi
  UlMcs = 19,           // ul_mcs
  UlSam = 20,           // ul_n_samples
  UlBytes = 21,         // ul_buffer_bytes
  UlThr = 22,           // rx_brate_uplink_mbps
  UlPkts = 23,          // rx_pkts_uplink
  UlErr = 24,           // rx_errors_uplink_perc
  UlRssi = 25,          // ul_rssi
  UlSinr = 26,          // ul_sinr
  Phr = 27,             // phr
  SumReqPrb = 29,       // sum_requested_prbs
  SumGraPrb = 30,       // sum_granted_prbs
  DlPmi = 32,           // dl_pmi
  DlRi = 33,            // dl_ri
  UlN = 34,             // ul_n
  UlTurboIt = 35,       // ul_turbo_iters
  RatioReqGraPrb = 99,  // sum_granted_prbs / sum_requested_prbs, in [0, 1], actually not in report but computed
}


#[derive(Default)]
struct BsMetrics {
    timestamp: u128,
    num_ues: u16,
    imsi: u64,
    rnti: u16,
    slicing_enabled: u8,
    slice_id: u32,
    slice_prb: u32,
    power_multiplier: f32,
    scheduling_policy: u32,

    // downlink
    dl_mcs: f32,
    dl_n_samples: i32,
    dl_buffer_bytes: i32,
    tx_brate_downlink_mbps: f32,
    tx_pkts_downlink: i32,
    tx_errors_downlink_perc: f32,
    dl_cqi: f32,

    // uplink
    ul_mcs: f32,
    ul_n_samples: i32,
    ul_buffer_bytes: i32,
    rx_brate_uplink_mbps: f32,
    rx_pkts_uplink: i32,
    rx_errors_uplink_perc: f32,
    ul_rssi: f32,
    ul_sinr: f32,
    phr: f32,

    // prb
    sum_requested_prbs: i32,
    sum_granted_prbs: i32,

    // other
    dl_pmi: f32,
    dl_ri: f32,
    ul_n: f32,
    ul_turbo_iters: f32,
}


#[no_mangle]
pub extern "C" fn get_tx_string_c(lines_to_read: u32, json_format: bool) -> *mut c_char {
    let tx_string = get_tx_string(lines_to_read, json_format);

    if tx_string.len() == 0 {
        return std::ptr::null_mut();
    }
    
    let c_str_tx_string = CString::new(tx_string).unwrap();
    c_str_tx_string.into_raw()
}


// Function to get the nth line from a file
// line count starts from 0, with 0 being the header
fn get_nth_line(filename: &str, n: usize) -> io::Result<String> {
    let file = File::open(filename)?;
    let buffer = BufReader::new(file);

    // Skip `n-1` lines and attempt to read the nth line
    let lines_to_skip = cmp::max(0, n);
    buffer.lines().nth(lines_to_skip).unwrap_or_else(|| Ok(String::new()))
}


// Function to count the number of lines in a file
fn count_lines(filename: &str) -> io::Result<usize> {
    let file = File::open(filename)?;
    let buffer = BufReader::new(file);
    Ok(buffer.lines().count())
}


fn get_dir_content(directory_path: &str) -> Vec<String> {
    let mut metric_files_vector = Vec::new();

    let all_paths;
    if Path::new(directory_path).exists() {
        all_paths = fs::read_dir(directory_path).unwrap();
    }
    else {
        println!("Path does not exist: {}", directory_path);
        return metric_files_vector;
    }

    for p in all_paths {
        let path = p.unwrap().path().display().to_string();
        let path_vector: Vec<&str> = path.split("/").collect();
        let metrics_filename = path_vector.last();
        
        let filename_vector: Vec<&str> = metrics_filename.unwrap().split("_").collect();
        let imsi = filename_vector.first();

        if imsi.unwrap().len() >= 10 {
            metric_files_vector.push(path);
        }
    }

    metric_files_vector
}


fn get_current_timestamp_ms() -> u128 {
    let start = SystemTime::now();
    let since_the_epoch = start
        .duration_since(UNIX_EPOCH)
        .expect("Time went backwards");
    since_the_epoch.as_millis()
}


fn read_metrics(filename: &str, line_no: usize, bs_metrics: &mut BsMetrics) -> i32 {
    let current_line = get_nth_line(filename, line_no).unwrap();
    println!("Read line {}: {}", line_no, current_line);

    let current_line_vec: Vec<&str> = current_line.split(",").collect();
    let mut read_items = 0;
    for (el_idx, el) in current_line_vec.iter().enumerate() {
        if el.len() <= 0 {
            continue;
        }

        match el_idx {
            el_idx if el_idx == ReportMetrics::Ts as usize => bs_metrics.timestamp = el.parse::<u128>().unwrap(),
            el_idx if el_idx == ReportMetrics::UeNum as usize => bs_metrics.num_ues = el.parse::<u16>().unwrap(),
            el_idx if el_idx == ReportMetrics::Imsi as usize => bs_metrics.imsi = el.parse::<u64>().unwrap(),
            el_idx if el_idx == ReportMetrics::Rnti as usize => bs_metrics.rnti = el.parse::<u16>().unwrap(),
            el_idx if el_idx == ReportMetrics::SlEn as usize => bs_metrics.slicing_enabled = el.parse::<u8>().unwrap(),
            el_idx if el_idx == ReportMetrics::SlId as usize => bs_metrics.slice_id = el.parse::<u32>().unwrap(),
            el_idx if el_idx == ReportMetrics::SlPrb as usize => bs_metrics.slice_prb = el.parse::<u32>().unwrap(),
            el_idx if el_idx == ReportMetrics::PowMult as usize => bs_metrics.power_multiplier = el.parse::<f32>().unwrap(),
            el_idx if el_idx == ReportMetrics::SchedPol as usize => bs_metrics.scheduling_policy = el.parse::<u32>().unwrap(),
            
            // downlink
            el_idx if el_idx == ReportMetrics::DlMcs as usize => bs_metrics.dl_mcs = el.parse::<f32>().unwrap(),
            el_idx if el_idx == ReportMetrics::DlSam as usize => bs_metrics.dl_n_samples = el.parse::<i32>().unwrap(),
            el_idx if el_idx == ReportMetrics::DlBytes as usize => bs_metrics.dl_buffer_bytes = el.parse::<i32>().unwrap(),
            el_idx if el_idx == ReportMetrics::DlThr as usize => bs_metrics.tx_brate_downlink_mbps = el.parse::<f32>().unwrap(),
            el_idx if el_idx == ReportMetrics::DlPkts as usize => bs_metrics.tx_pkts_downlink = el.parse::<i32>().unwrap(),
            el_idx if el_idx == ReportMetrics::DlErr as usize => bs_metrics.tx_errors_downlink_perc = el.parse::<f32>().unwrap(),
            el_idx if el_idx == ReportMetrics::DlCqi as usize => bs_metrics.dl_cqi = el.parse::<f32>().unwrap(),
            
            // uplink
            el_idx if el_idx == ReportMetrics::UlMcs as usize => bs_metrics.ul_mcs = el.parse::<f32>().unwrap(),
            el_idx if el_idx == ReportMetrics::UlSam as usize => bs_metrics.ul_n_samples = el.parse::<i32>().unwrap(),
            el_idx if el_idx == ReportMetrics::UlBytes as usize => bs_metrics.ul_buffer_bytes = el.parse::<i32>().unwrap(),
            el_idx if el_idx == ReportMetrics::UlThr as usize => bs_metrics.rx_brate_uplink_mbps = el.parse::<f32>().unwrap(),
            el_idx if el_idx == ReportMetrics::UlPkts as usize => bs_metrics.rx_pkts_uplink = el.parse::<i32>().unwrap(),
            el_idx if el_idx == ReportMetrics::UlErr as usize => bs_metrics.rx_errors_uplink_perc = el.parse::<f32>().unwrap(),
            el_idx if el_idx == ReportMetrics::UlRssi as usize => bs_metrics.ul_rssi = el.parse::<f32>().unwrap(),
            el_idx if el_idx == ReportMetrics::UlSinr as usize => bs_metrics.ul_sinr = el.parse::<f32>().unwrap(),
            el_idx if el_idx == ReportMetrics::Phr as usize => bs_metrics.phr = el.parse::<f32>().unwrap(),
            
            // prb
            el_idx if el_idx == ReportMetrics::SumReqPrb as usize => bs_metrics.sum_requested_prbs = el.parse::<i32>().unwrap(),
            el_idx if el_idx == ReportMetrics::SumGraPrb as usize => bs_metrics.sum_granted_prbs = el.parse::<i32>().unwrap(),
            
            // other
            el_idx if el_idx == ReportMetrics::DlPmi as usize => bs_metrics.dl_pmi = el.parse::<f32>().unwrap(),
            el_idx if el_idx == ReportMetrics::DlRi as usize => bs_metrics.dl_ri = el.parse::<f32>().unwrap(),
            el_idx if el_idx == ReportMetrics::UlN as usize => bs_metrics.ul_n = el.parse::<f32>().unwrap(),
            el_idx if el_idx == ReportMetrics::UlTurboIt as usize => bs_metrics.ul_turbo_iters = el.parse::<f32>().unwrap(),

            // default
            _ => println!("Unexpected additional data at index {}: {:?}", el_idx, el),
        }

        read_items += 1;
    }

    read_items
}


fn read_metrics_interactive(filename: &str, line_no: usize, curr_ts: u128, json_format: bool, metrics_preset: u32) -> String {
    let bs_metrics = &mut BsMetrics { ..Default::default() };
    let read_items = read_metrics(filename, line_no, bs_metrics);
    println!("Read items: {}", read_items);

    let mut selected_metrics = String::new();

    if read_items < CSV_METRICS_NUM.into() {
        println!("Read unexpected number of metrics");
        return selected_metrics;
    }

    // check if metric is fresh enough
    let timestamp_difference_s = (curr_ts as i128 - bs_metrics.timestamp as i128) as f32 / 1000.0;
    if timestamp_difference_s > DELTA_TS_S as f32 && !TIMESTAMP_DEBUG {
        println!("Read metric is too old, skipping it. Time difference is {} s", timestamp_difference_s);
        return selected_metrics;
    }

    // build metric names map
    let report_metric_names: HashMap<usize, &str> = HashMap::from([
        (ReportMetrics::Ts as usize, "timestamp"),
        (ReportMetrics::UeNum as usize, "num_ue"),
        (ReportMetrics::Imsi as usize, "imsi"),
        (ReportMetrics::Rnti as usize, "rnti"),
        (ReportMetrics::SlEn as usize, "slicing_enabled"),
        (ReportMetrics::SlId as usize, "slice_id"),
        (ReportMetrics::SlPrb as usize, "slice_prb"),
        (ReportMetrics::PowMult as usize, "power_multiplier"),
        (ReportMetrics::SchedPol as usize, "scheduling_policies"),
        (ReportMetrics::DlMcs as usize, "dl_mcs"),
        (ReportMetrics::DlSam as usize, "dl_samples"),
        (ReportMetrics::DlBytes as usize, "dl_bytes"),
        (ReportMetrics::DlThr as usize, "dl_thr_mbps"),
        (ReportMetrics::DlPkts as usize, "dl_pkts"),
        (ReportMetrics::DlErr as usize, "dl_errors"),
        (ReportMetrics::DlCqi as usize, "dl_cqi"),
        (ReportMetrics::UlMcs as usize, "ul_mcs"),
        (ReportMetrics::UlSam as usize, "ul_samples"),
        (ReportMetrics::UlBytes as usize, "ul_bytes"),
        (ReportMetrics::UlThr as usize, "ul_thr_mbps"),
        (ReportMetrics::UlPkts as usize, "ul_pkts"),
        (ReportMetrics::UlErr as usize, "ul_errors"),
        (ReportMetrics::UlRssi as usize, "ul_rssi"),
        (ReportMetrics::UlSinr as usize, "ul_sinr"),
        (ReportMetrics::Phr as usize, "phr"),
        (ReportMetrics::SumReqPrb as usize, "sum_requested_prbs"),
        (ReportMetrics::SumGraPrb as usize, "sum_granted_prbs"),
        (ReportMetrics::DlPmi as usize, "dl_pmi"),
        (ReportMetrics::DlRi as usize, "dl_ri"),
        (ReportMetrics::UlN as usize, "ul_n"),
        (ReportMetrics::UlTurboIt as usize, "ul_turbo_iterations"),
        (ReportMetrics::RatioReqGraPrb as usize, "ratio_granted_req_prb"),
    ]);

    // only send if sum_requested_prbs > 0 or if prbs are granted anyway
    if bs_metrics.sum_requested_prbs > 0 || (bs_metrics.sum_requested_prbs == 0 && bs_metrics.sum_granted_prbs > 0) {
        match metrics_preset {
            0 => {
                // send all metrics in this preset
                if !json_format {
                    // just concatenate the string
                    selected_metrics = format!("{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{}",
                        bs_metrics.timestamp, bs_metrics.num_ues, bs_metrics.imsi, bs_metrics.rnti,
                        bs_metrics.slicing_enabled, bs_metrics.slice_id, bs_metrics.slice_prb, bs_metrics.power_multiplier,
                        bs_metrics.scheduling_policy, bs_metrics.dl_mcs, bs_metrics.dl_n_samples, bs_metrics.dl_buffer_bytes,
                        bs_metrics.tx_brate_downlink_mbps, bs_metrics.tx_pkts_downlink, bs_metrics.tx_errors_downlink_perc, bs_metrics.dl_cqi,
                        bs_metrics.ul_mcs, bs_metrics.ul_n_samples, bs_metrics.ul_buffer_bytes, bs_metrics.rx_brate_uplink_mbps,
                        bs_metrics.rx_pkts_uplink, bs_metrics.rx_errors_uplink_perc, bs_metrics.ul_rssi, bs_metrics.ul_sinr,
                        bs_metrics.phr, bs_metrics.sum_requested_prbs, bs_metrics.sum_granted_prbs, bs_metrics.dl_pmi, bs_metrics.dl_ri,
                        bs_metrics.ul_n, bs_metrics.ul_turbo_iters);
                    println!("{}", selected_metrics);
                }
                else {
                    // organize string in json format
                    selected_metrics = format!("{{\"{}\":{},\"{}\":{},\"{}\":{},\"{}\":{},\"{}\":{},\"{}\":{},\"{}\":{},\
                        \"{}\":{},\"{}\":{},\"{}\":{},\"{}\":{},\"{}\":{},\"{}\":{},\"{}\":{},\"{}\":{},\
                        \"{}\":{},\"{}\":{},\"{}\":{},\"{}\":{},\"{}\":{},\"{}\":{},\"{}\":{},\"{}\":{},\
                        \"{}\":{},\"{}\":{},\"{}\":{},\"{}\":{},\"{}\":{},\"{}\":{},\"{}\":{},\"{}\":{}}}",
                        report_metric_names[&(ReportMetrics::Ts as usize)], bs_metrics.timestamp,
                        report_metric_names[&(ReportMetrics::UeNum as usize)], bs_metrics.num_ues,
                        report_metric_names[&(ReportMetrics::Imsi as usize)], bs_metrics.imsi,
                        report_metric_names[&(ReportMetrics::Rnti as usize)], bs_metrics.rnti,
                        report_metric_names[&(ReportMetrics::SlEn as usize)], bs_metrics.slicing_enabled,
                        report_metric_names[&(ReportMetrics::SlId as usize)], bs_metrics.slice_id,
                        report_metric_names[&(ReportMetrics::SlPrb as usize)], bs_metrics.slice_prb,
                        report_metric_names[&(ReportMetrics::PowMult as usize)], bs_metrics.power_multiplier,
                        report_metric_names[&(ReportMetrics::SchedPol as usize)], bs_metrics.scheduling_policy,
                        report_metric_names[&(ReportMetrics::DlMcs as usize)], bs_metrics.dl_mcs,
                        report_metric_names[&(ReportMetrics::DlSam as usize)], bs_metrics.dl_n_samples,
                        report_metric_names[&(ReportMetrics::DlBytes as usize)], bs_metrics.dl_buffer_bytes,
                        report_metric_names[&(ReportMetrics::DlThr as usize)], bs_metrics.tx_brate_downlink_mbps,
                        report_metric_names[&(ReportMetrics::DlPkts as usize)], bs_metrics.tx_pkts_downlink,
                        report_metric_names[&(ReportMetrics::DlErr as usize)], bs_metrics.tx_errors_downlink_perc,
                        report_metric_names[&(ReportMetrics::DlCqi as usize)], bs_metrics.dl_cqi,
                        report_metric_names[&(ReportMetrics::UlMcs as usize)], bs_metrics.ul_mcs,
                        report_metric_names[&(ReportMetrics::UlSam as usize)], bs_metrics.ul_n_samples,
                        report_metric_names[&(ReportMetrics::UlBytes as usize)], bs_metrics.ul_buffer_bytes,
                        report_metric_names[&(ReportMetrics::UlThr as usize)], bs_metrics.rx_brate_uplink_mbps,
                        report_metric_names[&(ReportMetrics::UlPkts as usize)], bs_metrics.rx_pkts_uplink,
                        report_metric_names[&(ReportMetrics::UlErr as usize)], bs_metrics.rx_errors_uplink_perc,
                        report_metric_names[&(ReportMetrics::UlRssi as usize)], bs_metrics.ul_rssi,
                        report_metric_names[&(ReportMetrics::UlSinr as usize)], bs_metrics.ul_sinr,
                        report_metric_names[&(ReportMetrics::Phr as usize)], bs_metrics.phr,
                        report_metric_names[&(ReportMetrics::SumReqPrb as usize)], bs_metrics.sum_requested_prbs,
                        report_metric_names[&(ReportMetrics::SumGraPrb as usize)], bs_metrics.sum_granted_prbs,
                        report_metric_names[&(ReportMetrics::DlPmi as usize)], bs_metrics.dl_pmi,
                        report_metric_names[&(ReportMetrics::DlRi as usize)], bs_metrics.dl_ri,
                        report_metric_names[&(ReportMetrics::UlN as usize)], bs_metrics.ul_n,
                        report_metric_names[&(ReportMetrics::UlTurboIt as usize)], bs_metrics.ul_turbo_iters);
                    // println!("{}", selected_metrics);
                }
            },
            1 => {
                //////////////////////////////////////////////////////////////////////////
                //
                // ordering metrics in same order as agent parser (numbers mark the order)
                // timestamp is only used for metrics freshness and removed before sending data
                //
                // metric_dict = {"dl_buffer [bytes]": 1, "tx_brate downlink [Mbps]": 2,
                //                "ratio_req_granted": 3, "slice_id": 0, "slice_prb": 4}
                //
                //////////////////////////////////////////////////////////////////////////

                // compute radio between granted and requested prb
                let mut ratio_granted_req_prb: f32 = 1.0;
                if bs_metrics.sum_requested_prbs > 0 {
                    ratio_granted_req_prb = bs_metrics.sum_granted_prbs as f32 / bs_metrics.sum_requested_prbs as f32;
                }
                if ratio_granted_req_prb > 1.0 {
                    ratio_granted_req_prb = 1.0;
                }

                if !json_format {
                    // just concatenate the string
                    // do not include timestamp in this case
                    selected_metrics = format!("{},{},{},{},{},{}",
                        bs_metrics.slice_id, bs_metrics.dl_buffer_bytes, bs_metrics.tx_brate_downlink_mbps,
                        ratio_granted_req_prb, bs_metrics.slice_prb, bs_metrics.tx_pkts_downlink);
                    // println!("{}", selected_metrics);
                }
                else {
                    selected_metrics = format!("{{\"{}\":{},\"{}\":{},\"{}\":{},\"{}\":{},\"{}\":{},\"{}\":{},\"{}\":{}}}",
                        report_metric_names[&(ReportMetrics::Ts as usize)], bs_metrics.timestamp,
                        report_metric_names[&(ReportMetrics::SlId as usize)], bs_metrics.slice_id,
                        report_metric_names[&(ReportMetrics::DlBytes as usize)], bs_metrics.dl_buffer_bytes,
                        report_metric_names[&(ReportMetrics::DlThr as usize)], bs_metrics.tx_brate_downlink_mbps,
                        report_metric_names[&(ReportMetrics::RatioReqGraPrb as usize)], ratio_granted_req_prb,
                        report_metric_names[&(ReportMetrics::SlPrb as usize)], bs_metrics.slice_prb,
                        report_metric_names[&(ReportMetrics::DlPkts as usize)], bs_metrics.tx_pkts_downlink);
                    // println!("{}", selected_metrics);
                }
            },
            _ => println!("Preset {} unknown while reading metrics interactively", metrics_preset),
        }
    }

    selected_metrics
}


// read last lines from file
// NOTE: not porting the `skip_header` flag as it is not used anymore
fn read_metrics_lines(filename: &str, lines_to_read: u32, curr_ts: u128, output_metrics_vector: &mut Vec<String>, json_format: bool, metrics_preset: u32) {    
    if lines_to_read <= 0 {
        println!("Lines to read is set to 0");
        return;
    }

    let total_lines = count_lines(filename).unwrap();
    let number_of_lines_to_read: usize;

    if total_lines > lines_to_read as usize {
        number_of_lines_to_read = lines_to_read as usize;
    }
    else {
        print!("Not enough lines to read in metrics file");
        if total_lines > 0 {
            // the first line is the header, we do not want to read it
            number_of_lines_to_read = total_lines - 1;
            println!("reading {} lines", number_of_lines_to_read);
        }
        else {
            println!("skipping reading");
            number_of_lines_to_read = 0;
        }
    }

    // read lines
    for i in 0..number_of_lines_to_read {
        let current_line_no = total_lines - number_of_lines_to_read + i;
        let output_metrics = read_metrics_interactive(filename, current_line_no, curr_ts, json_format, metrics_preset);

        // skip if no valid metrics were returned
        if output_metrics.len() <= 0 {
            println!("Read metrics were not valid");
            continue;
        }
        
        // println!("Output metrics: {}", output_metrics);

        // push metrics in output vector
        output_metrics_vector.push(output_metrics);
    }
}


fn get_tx_string(lines_to_read: u32, json_format: bool) -> String {
    let curr_ts = get_current_timestamp_ms();
    let metric_files = get_dir_content(METRICS_DIR);

    let mut output_metrics_vector: Vec<String> = Vec::new();
    for m in metric_files {
        println!("Reading file: {}", m);
        read_metrics_lines(m.as_str(), lines_to_read, curr_ts, &mut output_metrics_vector, json_format, METRICS_PRESET);
    }

    // concatenate metrics to transmit
    let mut final_metrics = String::new();
    for el in output_metrics_vector {
        final_metrics.push_str(el.as_str());
        final_metrics.push_str("\n");
    }

    if final_metrics.len() > 0 {
        println!("Final metrics to transmit\n---{}---\n", final_metrics);
    }
    else {
        println!("No valid metrics to transmit\n");
    }

    final_metrics
}


// fn main() {
//     let test_string = get_tx_string(2, true);
//     println!("Test string\n---{}---\n", test_string)
// }
