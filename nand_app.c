#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <time.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <mtd/mtd-user.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <sys/unistd.h>
#include <sys/time.h>

#define PAGE_SIZE 2048
#define ERASE_SIZE 0x20000
#define MILLION 1000000
unsigned int timeused;
struct timeval sta_time, end_time;

/* For array to generate random number */
/*  temp:Need to generate a random data of array
    size:Size of array
 */
void Random_Data(unsigned char *temp, int size)
{
    int i = 0;
    srand((unsigned)time(NULL)); //Make the random data more random
    //printf("The following random data, size is:%ld\n", size);
    for (i = 0; i < size; ++i)
    {
        temp[i] = rand() % 100;
        if(i < 4096)
        {
            if (0 == i % 16)
            {
                printf("\n");
            }
            if(0 == i%2048)
                printf("\n--------------------------------------------\n");
            printf("%2x\t", temp[i]);
        }
    }
    printf("\n");
}

/* int region_erase(int Fd, int start, int count, int unlock, int regcount)
{
    int i, j;
    region_info_t * reginfo;
    reginfo = calloc(regcount, sizeof(region_info_t));

    for(i = 0; i < regcount; ++i)
    {
        reginfo[i].regionindex = i;
        if(ioctl(Fd, MEMGETREGIONINFO, &(reginfo[i])) != 0)
        {
            return 8;
        }
        else
        {
            printf("Region %d is at %d of %d sector and with sector "
            "size %x\n", i, reginfo[i].offset, reginfo[i].numblocks,
            reginfo[i].erasesize);
        }
    }

    //We have all the information about the chip we need.

    for(i = 0; i < regcount; ++i)
    {
        //Loop through the regions
        region_info_t * r = &(reginfo[i]);

        if((start >= reginfo[i].offset) &&
             (start < (r->offset + r->numblocks*r->erasesize)))
        break;
    }

    if(i >= regcount)
    {
        printf("Starting offset %x not within chip.\n", start);
        return 8;
    }

    //We are now positioned within region i of the chip, so start erasing 
    //count sectors from there

    for(j = 0; (j < count)&&(i < regcount); ++j)
    {
        erase_info_t erase;
        region_info_t * r = &(reginfo[i]);

        erase.start = start;
        erase.length = r->erasesize;

        if(unlock != 0)
        {
            //Unlock the sector first.
            if(ioctl(Fd, MEMUNLOCK, &erase) != 0)
            {
                perror("\nMTD Unlock failure");
                close(Fd);
                return 8;
            }
        }
        printf("\rPerforming Flash Erase of length %u at offset 0x%x",
                erase.length, erase.start);
        fflush(stdout);
        if(ioctl(Fd, MEMERASE, &erase) != 0)
        {
            perror("\nMTD Erase failure");
            close(Fd);
            return 8;
        }

        start += erase.length;
        if(start >= (r->offset + r->numblocks*r->erasesize))
        {
            //We finished region i so move to region i+1
            printf("\nMoving to region %d\n", i+1);
            ++i;
        }
    }

    printf("Done\n");

    return 0;
}
 */
/* Erase device */
/*  Fd: number of device
    start: start adress
    count: count blocks
    unlock: unlock device
 */
int non_region_erase(int Fd, int start, int count, int unlock)
{
    mtd_info_t meminfo;
    
    /* Get the device information */
    if(ioctl(Fd, MEMGETINFO, &meminfo) == 0)
    {
        erase_info_t erase;
        erase.start = start;
        erase.length = meminfo.erasesize;
        printf("count = %d\n", count);

        /* Erase device and calculate the erase time */
        gettimeofday(&sta_time, NULL);      //Get start time(us), the same below
        for(; count > 0; count--)
        {
            printf("\rPerforming Flash Erase of length %u at offset 0x%x \n",   //Printf erase adress
            erase.length, erase.start);
            fflush(stdout);     //Flush the output buffer, and the output buffer content

            if(unlock != 0)
            {
                //Unlock the sector first
                printf("\rPerforming Flash unlock at offset 0x%x\n", erase.start);
                if(ioctl(Fd, MEMUNLOCK, &erase) != 0)
                {
                    perror("\nMTD Unlock failure");
                    close(Fd);
                    return 8;
                }
            }

            if(ioctl(Fd, MEMERASE, &erase) != 0)
            {
                perror("\nMTD Erase failure");
                close(Fd);
                return 8;
            }
            erase.start += meminfo.erasesize;
        }
        gettimeofday(&end_time, NULL);      //Get finish time(us), the same below
        /* Calculate used time, the same below */
        timeused = MILLION*(end_time.tv_sec - sta_time.tv_sec) + end_time.tv_usec - sta_time.tv_usec;
        printf("Written using time: %d us\n", timeused);

        printf("Done\n");
    }
    return 0;
}

/* Write and Read */
/*  Fd: Number of device
    start: Start adress
    count: Number of blocks
 */
int WR_MTD(int Fd, int start, int count)
{
    int flag = 0, i = 0;
    int offset = start;
    int page = 64*8*32;     //Number of page
    int j = 0, k = 0;
    
    unsigned char *write_buf = malloc(sizeof(*write_buf) * 2048*64*8*32);
    unsigned char *read_buf = malloc(sizeof(*read_buf) * 2048*64*8*32);
    unsigned char *read_point = read_buf;

    /* Generate random data */
    Random_Data(write_buf, 2048*64*8*32);

    /* According to the page to write data and calculate written time */
    gettimeofday(&sta_time, NULL);      
    for(i = 0; i < page; ++i)
    {
        if((flag = pwrite(Fd, write_buf, PAGE_SIZE, offset)) < 0)
        {
            printf("errno = %d\n", errno);
            printf("Mseg:%s\n", strerror(errno));
        }
        write_buf += PAGE_SIZE;
        offset += PAGE_SIZE;
    }
    gettimeofday(&end_time, NULL);      
    timeused = MILLION*(end_time.tv_sec - sta_time.tv_sec) + end_time.tv_usec - sta_time.tv_usec;
    printf("Written using time: %d us\n", timeused);

    offset = start;     //Offset adress

    /* According to the page to read data and calculate read time */
    gettimeofday(&sta_time, NULL);
    for(i = 0; i < page; ++i)
    {
        if((flag = pread(Fd, read_buf, PAGE_SIZE, offset)) < 0)
        {
            printf("errno = %d\n", errno);
            printf("Mseg:%s\n", strerror(errno));
        }
        read_buf += PAGE_SIZE;
        offset += PAGE_SIZE;
    }
    gettimeofday(&end_time, NULL);
    timeused = MILLION*(end_time.tv_sec - sta_time.tv_sec) + end_time.tv_usec - sta_time.tv_usec;
    printf("Written using time: %d us\n", timeused);

    /* Display some data */
    for(i = 0; i < 4096; ++i)
    {
        if(0 == i%16)
            printf("\n");
        if(0 == i%2048)
            printf("\n--------------------------------------------\n");
        printf("%2x\t", read_point[i]);
    }
    return 0;
}

int main(int argc, char const *argv[])
{
    int regcount;
    int Fd;
    int start, count;
    int unlock;
    int res = 0;
    mtd_info_t info;

    if(1 >= argc)
    {
        fprintf(stderr, "You must specify a device");
        return 16;
    }
    
    if(argc > 2)
        start = strtol(argv[2], NULL, 0);
    else
        start = 0;

    if(argc > 3)
        count = strtol(argv[3], NULL, 0);
    else
        count = 1;


    //Open and size the device
    if((Fd = open(argv[1], O_RDWR)) < 0)
    {
        fprintf(stderr, "File open error\n");
        printf("errno=%d\n", errno);
        printf("Mseg:%s\n", strerror(errno));
        return 8;
    }
    else
    {
        ioctl(Fd, MEMGETINFO, &info);
        printf(" info.size = %ld\n info.erasesize = %ld\n info.writesize = %ld\n info.oobsize = %ld\n",
                info.size, info.erasesize, info.writesize, info.oobsize);
    }

    printf("Erase Total %d Units\n", count);

    if(ioctl(Fd, MEMGETREGIONCOUNT, &regcount) == 0)
    {
        printf("regcount = %d\n", regcount);
        if(0 == regcount)
        {
            res = non_region_erase(Fd, start, count, 0);
            printf("res = %d\n", res);
        }
    }

    if(0 == res)
    {
        printf("Write and Read!\n");
        WR_MTD(Fd, start, count);
    }


    printf("\n");
    return 0;
}
