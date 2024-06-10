use fs2::FileExt;
use std::ffi::CStr;
use std::fs::File;
use std::io::{Read, Seek, Write};
use std::os::raw::c_char;
use std::path::Path;


static SLICE_NUM: u8 = 10;  // total number of slices to write in the config file
static CONFIG_PATH: &str = "/root/radio_code/scope_config/slicing/";
static SCHEDULING_FILENAME: &str = "slice_scheduling_policy.txt";
static SLICING_BASE_FILENAME: &str = "slice_allocation_mask_tenant_";


#[no_mangle]
pub extern "C" fn write_control_policies_c(control_policies: *const c_char) {
    // convert received control policies into c string slice 
    let c_str = unsafe {
        assert!(!control_policies.is_null());
        CStr::from_ptr(control_policies)
    };

    if let Ok(rust_str) = c_str.to_str() {
        write_control_policies(rust_str);
    } else {
        println!("Invalid UTF-8 sequence received as control policies");
    }
}


// lock file, compare with running configuration, write policies if needed, unlock file
fn write_policy_file(to_write: String, filename: &str, file_path: &str) {
    if !Path::new(CONFIG_PATH).exists() {
        println!("Configuration path {} does not exist. Not writing policies", CONFIG_PATH);
        return;
    }

    let mut policies_file;
    let file_to_write = file_path.to_owned() + "/" + filename;

    policies_file = File::options()
        .create(true)
        .read(true)
        .write(true)
        .open(file_to_write.clone())
        .expect("Failed to open policy file");

    policies_file.lock_exclusive().expect("Failed to lock policy file");

    // read running policies and compare them with new policies
    let mut running_policies = String::new();
    policies_file.read_to_string(&mut running_policies).expect("Failed to read policy file");
    
    if to_write.eq(&running_policies) {
        println!("Same policy(ies) as running configuration");
    }
    else {
        // rewind and truncate file, and write updated policies
        policies_file.rewind().unwrap();
        policies_file.set_len(0).unwrap();
        policies_file.write(to_write.as_bytes()).expect("Failed to write policy on file");
        println!("Writing policies on file {}\n{}", file_to_write, to_write);
    }

    policies_file.unlock().expect("Failed to unlock policy file");
}


// input_policy is a comma-separated list of scheduling policies, one for each slice, e.g., "2,0,1"
fn write_scheduling_policy(input_policy: &str) {
    const FILE_HEADER: &str = "# slice::scheduling policy\n\
                               # 0 = default srsLTE round-robin\n\
                               # 1 = waterfilling\n\
                               # 2 = proportional\n";
    const POLICIES_DELIMITER: &str = ",";
    const CONFIG_DELIMITER: &str = "::";
    const DEFAULT_POLICY: &str = "0";

    println!("Processing scheduling policies");

    // split input policy
    let mut policies_num = SLICE_NUM as usize;
    let mut policy_vector: Vec<&str> = input_policy.split(POLICIES_DELIMITER).collect();
    if policy_vector.len() < SLICE_NUM as usize {
        //  pad with default policy
        while policy_vector.len() < SLICE_NUM as usize {
            policy_vector.push(DEFAULT_POLICY);
        }
    }
    else {
        policies_num = policy_vector.len();
    }

    // assemble policy to write
    let mut scheduling_policy: String = FILE_HEADER.to_string();
    for i in 0..policies_num {
        let slice_scheduler_number = policy_vector[i as usize];
        let slice_policy = format!("{i}{CONFIG_DELIMITER}{slice_scheduler_number}\n");
        scheduling_policy.push_str(slice_policy.as_str());
    }

    write_policy_file(scheduling_policy, SCHEDULING_FILENAME, CONFIG_PATH);
}


// input_policy is a comma-separated list of slicing RBG, one for each slice, e.g., "5,10,2"
fn write_slicing_policy(input_policy: &str) {
    const RBG_NUM: u8 = 25;
    const POLICIES_DELIMITER: &str = ",";

    println!("Processing slicing policies");

    let mut prev_idx = 0;
    let policy_vector: Vec<&str> = input_policy.split(POLICIES_DELIMITER).collect();
    for (slice_idx, slice_val) in policy_vector.iter().enumerate() {
        let mut slicing_mask = String::new();

        // set places occupied by other policies to 0
        for _ in 0..prev_idx {
            slicing_mask.push('0');
        }

        // write 1 for current policy
        match slice_val.parse::<u8>() {
          Ok(slice_rbg) => {
            for _ in 0..slice_rbg {
                slicing_mask.push('1');
            }
            prev_idx += slice_rbg;
          },
          Err(e) => {
            println!("Error parsing slice RBG. Exiting. Error is: {:?}", e);
            return;
          },
        }

        if slicing_mask.len() < RBG_NUM as usize {
            // fill the rest with 0
            while slicing_mask.len() < RBG_NUM as usize {
                slicing_mask.push('0');
            }
        }
        else {
            // cut to RBG_NUM if policy has more elements
            slicing_mask = slicing_mask[..RBG_NUM as usize].to_string();
        }

        // println!("Policy of slice {} is {}", slice_idx, slicing_mask);

        let policy_filename = SLICING_BASE_FILENAME.to_owned() + &slice_idx.to_string() + ".txt";
        write_policy_file(slicing_mask, policy_filename.as_str(), CONFIG_PATH);
    }
}


// input_policy is a string of scheduling policies and slicing policies
// separated by a newline character, e.g., "2,0,1\n5,10,2",
// with scheduling on the first line and slicing on the second line
fn write_control_policies(input_policy: &str) {
    const POLICIES_DELIMITER: &str = "\n";

    // divide into scheduling and slicing policies
    println!("Received control policies\n{}", input_policy);
    let policy_vector: Vec<&str> = input_policy.split(POLICIES_DELIMITER).collect();

    let scheduling_policy: &str;
    let slicing_policy: &str;
    match policy_vector.len() {
        0 => {
            println!("No policies received. Exiting");
            return;
        },
        1 => {
            scheduling_policy = policy_vector[0];
            slicing_policy = "";
        },
        2 => {
            scheduling_policy = policy_vector[0];
            slicing_policy = policy_vector[1];
        },
        _ => {
            println!("Unexpected number of policies received. Exiting");
            return;
        },
    }
    
    if scheduling_policy.len() > 0 {
        write_scheduling_policy(scheduling_policy);
    }
    else {
        println!("No scheduling policy was received");
    }


    if slicing_policy.len() > 0 {
        write_slicing_policy(slicing_policy);
    }
    else {
        println!("No slicing policy was received");
    }
}


#[cfg(test)]
mod tests {
    // Note this useful idiom: importing names from outer (for mod tests) scope.
    use super::*;
    use std::{thread, time::Duration};

    #[test]
    fn write_scheduling_policy() {
        let control_policy = "10,20,30";
        write_control_policies(control_policy);
    }

    #[test]
    fn write_slicing_policy() {
        let control_policy = "\n5,10,2";
        write_control_policies(control_policy);
    }

    #[test]
    fn write_single_policy() {
        let control_policy = "10,20,30\n5,10,2";
        write_control_policies(control_policy);
    }

    #[test]
    fn write_multiple_policies() {
        let first_control_policy = "10,20,30\n5,10,2";
        let second_control_policy = "30,20,10\n2,10,5";

        for i in 1..100 {
            println!("Loop {}", i);
            if i % 2 == 0 {
                write_control_policies(first_control_policy);
            }
            else {
                write_control_policies(second_control_policy);
            }

            thread::sleep(Duration::from_millis(1000));
        }
    }
}
