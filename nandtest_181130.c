#include <sys/ioctl.h>
#include <stdio.h>
#include <mtd/mtd-user.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <linux/kernel.h>
#include <errno.h>
#include <sys/unistd.h>
#include <time.h>
#include <sys/time.h>

#define PRINT_N 2048
#define OFS (0)
#define block_size (128*1024)
#define page_size (2048)

/* For array to generate random number */
void RandomData(unsigned char *temp, unsigned int size)
{
    int i = 0;
    srand((unsigned)time(NULL));
    printf("The following random data, size is:%d\n", size);
    for(i = 0; i < PRINT_N; ++i)
    {
        temp[i] = rand()%100;
        if(0 == i%16)
        {
            printf("\n");
        }
        printf("%2x\t", temp[i]);
    }
    printf("\n");
}

int main(int argc, char const *argv[])
{
    int fd = 0, flag = 0;
    int i = 0, j = 0;

    unsigned int timeuse;
    struct timeval start, end;

    /* Define data buffer*/
    unsigned char oob_data[2048];
    unsigned char *oob;
    oob = malloc(sizeof(char)*512*2048);
    RandomData(oob, 512*2048);

    /*Try to open device*/
    fd = open("/dev/mtdblock3", O_RDWR);     
    if(fd < 0)
    {
        perror("fail to opne\n");
        exit(-1);
    }

    /* According to the page to write data */
    // bgntime = time(NULL);
    // c_bgn = clock();
    // printf("bgntime:%lf\n", bgntime);
    gettimeofday(&start, NULL);
    if((flag = pwrite(fd, oob, 2048, 2048)) < 0)
    {
        printf("errno=%d\n", errno);
        printf("Mseg:%s\n", strerror(errno));
    }
    gettimeofday(&end, NULL);
    timeuse = 1000000 * ( end.tv_sec - start.tv_sec ) + end.tv_usec - start.tv_usec;
    printf("time: %d us\n", timeuse);
    // endtime = time(NULL);
    // c_end = clock();
    // printf("endtime:%lf\n", endtime);
    // printf("Written used %lf ms\n", difftime(endtime, bgntime));
    // printf("ms:%lf\n", c_end-c_bgn);
    memset(oob_data, 0, 2048);
	
    /* According to the page to read data */
    if((flag = pread(fd, oob_data, 2048, 2048)) < 0)
    {
        printf("errno=%d\n", errno);
        printf("Mseg:%s\n", strerror(errno));
    }

    for(i = 0; i < PRINT_N; ++i)
    {
        if(0 == i%16)
            printf("\n");
        printf("%2x\t", oob_data[i]);
    }

    printf("\n");

    return 0;
}
