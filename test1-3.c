#include "types.h"
#include "stat.h"
#include "user.h"

int main(void) {
    printf(1, "start scheduler_test\n");

    int pid1 = fork();
    if (pid1 == 0) {
        set_proc_info(2, 0, 0, 0, 300); 
	while(1);
        exit();
    }

    int pid2 = fork();
    if (pid2 == 0) {
        set_proc_info(2, 0, 0, 0, 300);
	while(1);
        exit();
    }

    int pid3 = fork();
    if (pid3 == 0) {
        set_proc_info(2, 0, 0, 0, 300); 
	while(1);
        exit();
    }

    for (int i = 0; i < 3; i++) {
        wait();
    }

    printf(1, "end of scheduler_test\n");
    exit();
}
