#include "srs_connector.h"

char last_scheduling_policy[256] = { '\0' };
char last_slicing_policy[256] = { '\0' };


// write scheduling policies on config file
void write_scheduling_policy(char* new_policy) {

  FILE *fp;
  char filename[1000];
  char* file_header = "# slice::scheduling policy\n"
                      "# 0 = default srsLTE round-robin\n"
                      "# 1 = waterfilling\n"
                      "# 2 = proportional";
  const char policies_delimiter[2] = ",";
  const char config_delimiter[3] = "::";
  const char default_policy[2] = "0";
  char* slice_policy;

  // form filename
  strcpy(filename, CONFIG_PATH);
  strcat(filename, SCHEDULING_FILENAME);

  // copy new_policy so that it can be modified
  char *policies = strdup(new_policy);

  fp = fopen(filename, "w+");

  if (fp == NULL) {
    printf("ERROR: fp is NULL\n");
    return;
  }

  // write header
  fprintf(fp, "%s\n", file_header);

  // get first policy
  slice_policy = strtok(policies, policies_delimiter);

  for (int i = 0; i < SLICE_NUM; ++i) {
    if (slice_policy != NULL) {
      fprintf(fp, "%d%s%s\n", i, config_delimiter, slice_policy);

      // get next policy
      slice_policy = strtok(NULL, policies_delimiter);
    }
    else {
      fprintf(fp, "%d%s%s\n", i, config_delimiter, default_policy);
    }
  }

  // save policy
  strcpy(last_scheduling_policy, new_policy);

  fclose(fp);
}


// write slicing policy on config file
void write_slicing_policy(char* new_policy) {

  FILE *fp;
  char base_filename[1000];

  const int rbg_num = 25;
  const char policies_delimiter[2] = ",";

  // copy control message so it can be modified
  char *policies = strdup(new_policy);

  // form filename
  strcpy(base_filename, CONFIG_PATH);
  strcat(base_filename, SLICING_BASE_FILENAME);

  // count how many policies were received
  int p_num = 0;
  for (int i = 0; i < strlen(new_policy); ++i) {
    if (new_policy[i] == policies_delimiter[0]) {
      p_num++;
    }
  }

  // increase to account for last policy
  p_num++;

  // write scheduling files
  int m_ptr = 0;
  for (int s_idx = 0; s_idx < p_num; ++s_idx) {
    // get slice number and form slicing filename
    char slice_num[2];
    sprintf(slice_num, "%d", s_idx);

    char filename[1000];
    strcpy(filename, base_filename);
    strcat(filename, slice_num);
    strcat(filename, ".txt");

    // get current slicing policy
    char* rbg_policy_str = NULL;
    if (s_idx == 0) {
      rbg_policy_str = strtok(policies, policies_delimiter);
    }
    else {
      rbg_policy_str = strtok(NULL, policies_delimiter);
    }

    int rbg_policy = atoi(rbg_policy_str);
    char slicing_mask[rbg_num];

    // initialize slicing mask values to NULL
    for (int i = 0; i < rbg_num; ++i) {
      slicing_mask[i] = '\0';
    }

    int m_idx;
    for (m_idx = 0; m_idx < m_ptr; ++m_idx) {
      strcat(slicing_mask, "0");
    }

    for (int i = 0; i < rbg_policy && m_idx < rbg_num; ++i, ++m_idx) {
      strcat(slicing_mask, "1");
    }
    m_ptr = m_idx;

    for (; m_idx < rbg_num; ++m_idx) {
      strcat(slicing_mask, "0");
    }

    printf("%s\n", slicing_mask);

    // write mask on file
    fp = fopen(filename, "w");

    if (fp == NULL) {
      printf("ERROR: fp is NULL\n");
      return;
    }

    fprintf(fp, "%s", slicing_mask);
    fclose(fp);
  }
}


// receive agent control and write it on config files
// expected control looks like: '1,0,0\n5,10,3' --> scheduling on first, slicing on second line
void write_control_policies(char* control_msg) {

  // copy control message so it can be modified
  char *control = strdup(control_msg);
  
  char* scheduling_control = NULL;
  char* slicing_control = NULL;;

  // printf_neat("Received control message: ", control);
  printf_neat("Received control message: ", control);


  // divide scheduling and slicing control
  if (control[0] == '\n') {
    slicing_control = strtok(control, "\n");
  }
  else {
    scheduling_control = strtok(control, "\n");
    slicing_control = strtok(NULL, "\n");
  }

  // write scheduling control
  if (scheduling_control) {
    if (strcmp(scheduling_control, last_scheduling_policy) == 0) {
      printf("Scheduling policies are the same as last ones\n");
    }
    else {
      printf_neat("Writing new scheduling policies on config file ", scheduling_control);
      write_scheduling_policy(scheduling_control);

      // update last policy
      strcpy(last_scheduling_policy, scheduling_control);
    }
  }
  else {
    printf("No scheduling control received\n");
  }

  // write slicing control
  if (slicing_control) {
    if (strcmp(slicing_control, last_slicing_policy) == 0) {
      printf("Slicing policies are the same as last ones\n");
    }
    else{
      printf_neat("Writing new slicing policies on config file ", slicing_control);
      write_slicing_policy(slicing_control);

      // update last policy
      strcpy(last_slicing_policy, slicing_control);
    }
  }
  else {
    printf("No slicing control received\n");
  }
}

// debug print: print \n literally
void printf_neat(char* msg, char* dbg_str) {

    printf("%s", msg);

    for (int i = 0; i < strlen(dbg_str); ++i) {
      if (dbg_str[i] == '\n') {
        printf("\\n");
      }
      else {
        printf("%c", dbg_str[i]);
      }
    }

    printf("\n");
}


// tester function
int tester(void) {

  uint8_t* msg = "1,0,1\n4,5,7\n";
  uint8_t* msg2 = "\n10,5,4";

  // write_scheduling_policy((char*) msg);
  write_control_policies((char*) msg);

  return 0;
}
