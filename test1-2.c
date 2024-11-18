#include "types.h"
#include "stat.h"
#include "user.h"

int main(void) {
    printf(1, "start scheduler_test\n");

    int pid1 = fork();
    if (pid1 == 0) {

        set_proc_info(1, 0, 0, 0, 500); 
        while(1); 
        exit();
    }
    wait();
    printf(1, "end of scheduler_test\n");
    exit();
}

