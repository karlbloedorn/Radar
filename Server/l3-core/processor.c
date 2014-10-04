#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>

typedef struct wmo_header_struct {
     char bulletin_code[3];
     char geo_code[3];
     char distribution[3];
     char icao_generator[5];
     char month[3];
     char hour[3];
     char minute[3];
     char corrections[4];
} wmo_header;

typedef struct message_header_block_struct {
     uint16_t message_code;
     uint16_t date;
     uint32_t time;
     uint32_t length;
     uint16_t s_id;
     uint16_t d_id;
     uint16_t block_count;
} message_header_block;


int main(int argc, char *argv[])
{
     int fd, offset;
     char *data;
     struct stat sbuf;

     if ((fd = open("sn.last", O_RDONLY)) == -1) {
          perror("open");
          exit(1);
     }

     if (stat("sn.last", &sbuf) == -1) {
          perror("stat");
          exit(1);
     }

     data = mmap((caddr_t)0, sbuf.st_size, PROT_READ|PROT_WRITE, MAP_PRIVATE, fd, 0);
     process(data);
     return 0;

}
void load_product_description_block(message_header_block *in) {
     in->message_code = ntohs(in->message_code);
     in->date = ntohs(in->date);
     in->time = ntohl(in->time);
     in->length = ntohl(in->length);
     in->s_id = ntohs(in->s_id);
     in->d_id = ntohs(in->d_id);
     in->block_count = ntohs(in->block_count);
}


char *parse_wmo(char *data, wmo_header *in) {
     strncpy(in->bulletin_code, data, 2);
     in->bulletin_code[2] = '\0';
     data += 2;
     strncpy(in->geo_code, data, 2);
     data += 2;
     in->geo_code[2] = '\0';
     strncpy(in->distribution, data, 2);
     data += 3;
     in->distribution[2] = '\0';
     strncpy(in->icao_generator, data, 4);
     in->icao_generator[4] = '\0';
     data += 5;
     strncpy(in->month, data, 2);
     in->month[2] = '\0';
     data += 2;
     strncpy(in->hour, data, 2);
     data += 2;
     in->hour[2] = '\0';
     strncpy(in->minute, data, 2);
     data += 2;
     strncpy(in->corrections, data, 3);
     in->corrections[3] = '\0';
     data += 3;
     data += 9;
     return data;
}

void process(char *data) {
     wmo_header test;
     message_header_block *test2 = NULL;
     data = parse_wmo(data, &test);
     test2 = (message_header_block *)data;
     load_product_description_block(test2);
     
}
