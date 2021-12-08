#ifndef __SRS_CONNECTOR_H__
#define __SRS_CONNECTOR_H__

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// total number of slices to write in the config file
#define SLICE_NUM 10

#define CONFIG_PATH "/root/radio_code/scope_config/slicing/"
#define SCHEDULING_FILENAME "slice_scheduling_policy.txt"
#define SLICING_BASE_FILENAME "slice_allocation_mask_tenant_"

extern char last_policy[256];

void write_scheduling_policy(char* policies);
void write_slicing_policy(char* new_policy);
void write_control_policies(char* control_msg);
void printf_neat(char* msg, char* dbg_str);

#endif
