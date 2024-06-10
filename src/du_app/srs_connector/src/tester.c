extern void write_control_policies_c(char* control_policies);

char last_scheduling_policy[256] = { '\0' };
char last_slicing_policy[256] = { '\0' };


int main(void) {
    char* control_policies = "1,20,30\n7,11,3";
    write_control_policies_c(control_policies);

    write_control_policies_c("1,20,30\n1,2,3");
    write_control_policies_c("1,20,30\n4,5,6");
    return 0;
}
