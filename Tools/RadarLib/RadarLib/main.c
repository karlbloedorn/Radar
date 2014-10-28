#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include "radarlib.h"
#include "radarclientlib.h"

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

void dumpHeader(RadarHeader *in) {
     time_t scan_date = (time_t)(in->scan_date);
     struct tm *scan_date_tm;
     char time_buffer[128];
     memset(time_buffer, 0, 128);
     scan_date_tm = gmtime(&scan_date);
     strftime(time_buffer, 128, "%x - %X %Z", scan_date_tm);

     char callsign_terminated[5];
     callsign_terminated[4] = '\0';
     strncpy(callsign_terminated, in->callsign, 4);
     printf("CRDF v%i:\n", in->version);
     printf("\tCallsign: %s\n", callsign_terminated);
     printf("\tTime: %s (%i)\n", time_buffer, in->scan_date);
     printf("\tLat/Long: %f/%f\n", in->latitude, in->longitude);
     printf("\tCRC32: %x\n", in->crc32);
}

radar_status_t checkHeaderAction(RadarContext *context) {
     RadarHeader *header;
     radar_status_t status = loadRadarHeader(context, &header);
     if(status != RADAR_OK) {
          printf("Invalid header: %s Is this a CRDF file?\n", context->last_error);
          return status;
     }
     dumpHeader(header);
     status = verifyCRC32(context, header);
     if(status == RADAR_OK) {
          printf("File checksum matches!\n");
     } else {
          printf("File checksum mismatch!\n");
     }
     return status;
}

radar_status_t processAction(RadarContext *context, const char *output_file) {
     radar_status_t status = process(context, RADAR_LEVEL2);
     if(status != RADAR_OK){
          printf("Process error: %s\n", context->last_error);
          return status;
     }
     int write_fd = open(output_file, O_WRONLY|O_CREAT|O_TRUNC, S_IWUSR|S_IRUSR);
     write(write_fd, context->output.data, context->output.length);
     close(write_fd);
     printf("Wrote %lu bytes to %s\n", context->output.length, output_file);
     return checkHeaderAction(context);
}

int main(int argc, const char * argv[]) {
    int suggested_concurrency = sysconf(_SC_NPROCESSORS_ONLN);
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
    RadarContext *context;
    radar_status_t action_status = RADAR_OK;
    switch (selected_action) {
        case ACTION_DEFAULT:
        case ACTION_PROCESS:
            CheckArgumentLength(2)
            printf("Processing...\n");
            create_context(&context);

            context->input.length = file_to_ptr(argv[2], &(context->input.data));
            if(context->input.length == 0) {
                perror("Failed to open input file");
                return -1;
            }
            printf("Loaded %lu bytes from %s.\n", context->input.length, argv[2]);
            action_status = processAction(context, argv[3]);
            destroy_context(context);
            break;
        case ACTION_CHECK:
            CheckArgumentLength(1)
            printf("Checking header...\n");
            create_context(&context);
            context->output.length = file_to_ptr(argv[2], &(context->output.data));
            action_status = checkHeaderAction(context);
            // This data wasn't created from the heap so
            // destroy_context shouldn't free it
            context->output.data = NULL;
            destroy_context(context);
            break;
        case ACTION_COMPRESS:
            CheckArgumentLength(2)
            printf("Compressing output\n");
            break;
        case ACTION_TRIANGLES:
            CheckArgumentLength(1)
            printf(
                 "Computing triangles with %i thread%s...\n",
                 suggested_concurrency,
                 suggested_concurrency > 1 ? "s" : ""
                 );
            char *input_data;
            file_to_ptr(argv[2], &input_data);
            int32_t radial_count_ref;
            int32_t * gate_counts_ref;
            GateData ** gate_data_ref;
            int a = parse(input_data, suggested_concurrency, &gate_counts_ref, &radial_count_ref, &gate_data_ref);
            printf("%i\n", a);
            break;
        default:
            return 255;
    }
    switch(action_status) {
        case RADAR_OK:
             return 0;
        case RADAR_INVALID_DATA:
             return 1;
        case RADAR_NOMEM:
             return 2;
        case RADAR_NOT_IMPLEMENTED:
             return 3;
    }
    return 255;
}
