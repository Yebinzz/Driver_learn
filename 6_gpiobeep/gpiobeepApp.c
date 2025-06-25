#include "stdio.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "fcntl.h"
#include "stdlib.h"
#include "string.h"

int main(int argc, char **argv)
{
    int fd,retvalue;
    char *filename = argv[1];
    char writebuf[1];

    if(argc != 3)
    {
        printf("Error Usage!\r\n");
        return -1;
    }

    fd = open(filename, O_RDWR);
    if(fd < 0)
    {
        printf("file %s open failed!\r\n", filename);
        return -1;
    }

    writebuf[0] = atoi(argv[2]);
    retvalue = write(fd, writebuf ,sizeof(writebuf));
    if(retvalue < 0)
    {
        printf("BEEP control failed!\r\n");
        close(fd);
        return -1;
    }

    close(fd);
    return 0;

}