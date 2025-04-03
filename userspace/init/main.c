#include <string.h>
#include <syscall.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>

int main(void) {
    // TODO: Open stdin and stderr, tty0 (stdout) becomes fd 1 instead of 0
    stdout = fopen("/dev/tty0", "w");
    setvbuf(stdout, NULL, _IOLBF, 0);
    puts("Hello world from a userspace application loaded from an ELF file, writing to a stdout device!\n");
    printf("The number is %d and %d so yeah. Testing timer...\n", atoi("69"), atoi("\t\n  -420abc"));
    struct timespec start, finish;
    clock_gettime(CLOCK_REALTIME, &start);
    struct timespec wait_time = {.tv_sec = 1, .tv_nsec = 0};
    nanosleep(&wait_time);
    clock_gettime(CLOCK_REALTIME, &finish);
    printf("Start is %zu:%zu, end is %zu:%zu\n",
            start.tv_sec, start.tv_nsec, finish.tv_sec, finish.tv_nsec);
    int fb_file = open("/dev/fb0", 0, 0);
    void *fb = mmap(NULL, 0, 0, 0, fb_file, 0);
    printf("Got framebuffer vaddr = %p\n", fb);
    return 0;
}
