#include <stdio.h>
#include "radarlib.h"
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define CheckArgumentLength(needed) if(remaining_args < needed) { fprintf(stderr, "Not enough args\n"); return 1; }

enum action_enums_t { ACTION_PROCESS, ACTION_CHECK, ACTION_COMPRESS, ACTION_TRIANGLES, ACTION_DEFAULT };
char *actions[] = { "process", "check", "compress", "triangles" };
enum action_enums_t action_map[4] = { ACTION_PROCESS, ACTION_CHECK, ACTION_COMPRESS, ACTION_TRIANGLES };

size_t file_to_ptr(const char *filename, char **output) {
    int fd = open(filename, O_RDONLY);
    if(fd < 0) {
        return 0;
    }
    struct stat sb;
    if (fstat(fd, &sb) == -1){
        return 0;
    }
    void *addr = mmap(NULL, sb.st_size, PROT_READ|PROT_WRITE, MAP_PRIVATE, fd, 0);
    if (addr == MAP_FAILED){
        return 0;
    }
    *output = addr;
    return sb.st_size;
}

int main(int argc, const char * argv[]) {
    enum action_enums_t selected_action = ACTION_DEFAULT;
    int remaining_args = argc;
    remaining_args--;

    if(argc > 1) {
        for(int i = 0; i < 4; i++) {
            if(strcmp(actions[i], argv[1]) == 0) {
                selected_action = action_map[i];
                break;
            }
        }
        remaining_args--;
    }
    
    switch (selected_action) {
        case ACTION_DEFAULT:
        case ACTION_PROCESS:
            CheckArgumentLength(2)
            printf("Processing...\n");
            RadarContext *context;
            create_context(&context);

            context->input.length = file_to_ptr(argv[2], &(context->input.data));
            if(context->input.length == 0) {
                perror("Failed to open input file");
                return -1;
            }
            printf("Loaded %lu bytes from %s.\n", context->input.length, argv[2]);
            
            radar_status_t status = process(context, RADAR_LEVEL2);
            if(status != RADAR_OK){
                printf("Process error: %s\n", context->last_error);
            }
            int write_fd = open(argv[3], O_WRONLY|O_CREAT|O_TRUNC, S_IWUSR|S_IRUSR);
            write(write_fd, context->output.data, context->output.length);
            RadarHeader *header = (RadarHeader *)context->output.data;
            printf("Wrote %lu bytes to %s\n", context->output.length, argv[3]);
            char callsign_terminated[5];
            strncpy(callsign_terminated, header->callsign, 4);
            printf("\tCallsign: %s\n", callsign_terminated);
            printf("\tTime: %i\n", header->scan_date);
            printf("\tLat/Long: %f/%f\n", header->latitude, header->longitude);
            printf("\tCRC32: %u\n", header->crc32);
            destroy_context(context);
            break;
        case ACTION_CHECK:
            CheckArgumentLength(1)
            printf("Checking header\n");
            break;
        case ACTION_COMPRESS:
            CheckArgumentLength(2)
            printf("Compressing output\n");
            break;
        case ACTION_TRIANGLES:
            CheckArgumentLength(2)
            printf("Computing triangles\n");
            break;
        default:
            return 255;
    }
    
    
    return 0;
}
