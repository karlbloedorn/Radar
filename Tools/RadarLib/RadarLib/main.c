#include <stdio.h>
#include "radarlib.h"
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, const char * argv[]) {
    
    int fd = open("/Users/karlbloedorn/Desktop/Radar/Radar/Tools/RadarLib/RadarLib/KSGF_20141010_0020", O_RDONLY);
    struct stat sb;
    if (fstat(fd, &sb) == -1){
        return 1;
    }
    char *addr = mmap(NULL, sb.st_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    if (addr == MAP_FAILED){
        return 1;
    }
    
    RadarContext * context;
    create_context(&context);

    context->input.data = addr;
    context->input.length = sb.st_size;
    
    
    radar_status_t status = process(context, RADAR_LEVEL2);
    if(status != RADAR_OK){
        printf("error: %s\n", context->last_error );
    }
    int write_fd = open("output.bin", O_WRONLY|O_CREAT|O_TRUNC, S_IWUSR|S_IRUSR);
    write(write_fd, context->output.data, context->output.length);
    
    destroy_context(context);
    
    return 0;
}
