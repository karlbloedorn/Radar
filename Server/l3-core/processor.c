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

typedef enum product_data_types_enum {
     RADIAL,
     RASTER,
     ALPHANUMERIC
} product_data_types;

typedef struct product_types_struct {
     uint16_t product_code;
     uint16_t rotational_resolution;
     uint16_t gate_interval_distance;
     uint16_t range;
     uint16_t levels;
     product_data_types type;
} product_types;

product_types known_product_types[] = {
     {
          .product_code = 1,
          
     },
     {
          .product_code = 1,
          .range = 4,
     },
};

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

typedef struct  __attribute__((packed))  message_header_block_struct {
     uint16_t message_code;
     uint16_t date;
     uint32_t time;
     uint32_t length;
     uint16_t s_id;
     uint16_t d_id;
     uint16_t block_count;
} message_header_block;

typedef struct  __attribute__((packed)) product_description_block_struct {
     uint16_t divider;
     int32_t latitude;
     int32_t longitude;
     int16_t fsl;
     uint16_t product_code;
     uint16_t op_mode;
     uint16_t volume_scan_pattern;
     uint16_t seq_number;
     uint16_t volume_scan_number;
     uint16_t volume_scan_days;
     uint32_t volume_scan_time;
     uint16_t product_generation_days;
     uint32_t product_generation_time;
     uint16_t p1;
     uint16_t p2;
     uint16_t elevation_number;
     uint16_t p3;
     uint16_t data_thresholds[16];
     uint16_t p4;
     uint16_t p5;
     uint16_t p6;
     uint16_t p7;
     uint16_t p8;
     uint16_t p9;
     uint16_t p10;
     uint16_t number_of_maps;
     uint32_t offset_to_symbology_block;
     uint32_t offset_to_graphic_block;
     uint32_t offset_to_tabular_block;
} product_description_block;

typedef struct __attribute__((packed)) product_symbology_block_struct {
     uint16_t divider;
     uint16_t block_id;
     uint32_t block_length;
     uint16_t number_of_layers;
} product_symbology_block;

typedef struct __attribute__((packed)) symbology_layer_struct {
     uint16_t divider;
     uint32_t layer_length;
} symbology_layer;

typedef struct __attribute__((packed)) radial_data_packet_struct {
     uint16_t packet_code;
     uint16_t index_of_range_bin;
     uint16_t number_of_range_bins;
     int16_t i_center_sweep;
     int16_t j_center_sweep;
     uint16_t scale_factor;
     uint16_t number_of_radials;
} radial_data_packet;

typedef struct __attribute__((packed)) radial_struct {
     uint16_t number_of_rle;
     uint16_t start_angle;
     uint16_t delta_angle;
} radial;


#define LOAD_RUN(all) ((all & 0xF0) >> 4)
#define LOAD_CODE(all) (all & 0x0F)

typedef struct rle_struct {
     uint8_t run;
     uint8_t code;
} rle;

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

void load_radial(radial *in) {
     in->number_of_rle = ntohs(in->number_of_rle);
     in->start_angle = ntohs(in->start_angle);
     in->delta_angle = ntohs(in->delta_angle);
}

void load_symbology_layer(symbology_layer *in) {
     in->divider = ntohs(in->divider);
     in->layer_length = ntohl(in->layer_length);
}

void load_radial_data_packet(radial_data_packet *in) {
     in->packet_code = ntohs(in->packet_code);
     in->index_of_range_bin = ntohs(in->index_of_range_bin);
     in->number_of_range_bins = ntohs(in->number_of_range_bins);
     in->i_center_sweep = ntohs(in->i_center_sweep);
     in->j_center_sweep = ntohs(in->j_center_sweep);
     in->scale_factor = ntohs(in->scale_factor);
     in->number_of_radials = ntohs(in->number_of_radials);
}

void load_product_symbology_block(product_symbology_block *in) {
     in->divider = ntohs(in->divider);
     in->block_id = ntohs(in->block_id);
     in->block_length = ntohl(in->block_length);
     in->number_of_layers = ntohs(in->number_of_layers);
}

void load_product_description_block(product_description_block *in) {
     in->divider = ntohs(in->divider);
     in->latitude = ntohl(in->latitude);
     in->longitude = ntohl(in->longitude);
     in->fsl = ntohs(in->fsl);
     in->product_code = ntohs(in->product_code);
     in->op_mode = ntohs(in->op_mode);
     in->volume_scan_pattern = ntohs(in->volume_scan_pattern);
     in->seq_number = ntohs(in->seq_number);
     in->volume_scan_number = ntohs(in->volume_scan_number);
     in->volume_scan_days = ntohs(in->volume_scan_days);
     in->volume_scan_time = ntohl(in->volume_scan_time);
     in->product_generation_days = ntohs(in->product_generation_days);
     in->product_generation_time = ntohl(in->product_generation_time);
     in->p1 = ntohs(in->p1);
     in->p2 = ntohs(in->p2);
     in->p3 = ntohs(in->p3);
     in->p4 = ntohs(in->p4);
     in->p5 = ntohs(in->p5);
     in->p6 = ntohs(in->p6);
     in->p7 = ntohs(in->p7);
     in->p8 = ntohs(in->p8);
     in->p9 = ntohs(in->p9);
     in->p10 = ntohs(in->p10);
     in->elevation_number = ntohs(in->elevation_number);
     for(int i = 0; i < 16; i++) {
          in->data_thresholds[i] = ntohs(in->data_thresholds[i]);
     }
     in->number_of_maps = ntohs(in->number_of_maps);
     in->offset_to_symbology_block = ntohl(in->offset_to_symbology_block);
     in->offset_to_tabular_block = ntohl(in->offset_to_tabular_block);
     in->offset_to_graphic_block = ntohl(in->offset_to_graphic_block);
}

void load_message_header_block(message_header_block *in) {
     in->message_code = ntohs(in->message_code);
     in->date = ntohs(in->date);
     in->time = ntohl(in->time);
     in->length = ntohl(in->length);
     in->s_id = ntohs(in->s_id);
     in->d_id = ntohs(in->d_id);
     in->block_count = ntohs(in->block_count);
}


char *parse_wmo(char *data, wmo_header *in) {
     //T1T2A1A2ii CCCC YYGGgg [BBB](cr)(cr)(lf)NNNxxx(cr)(cr)(lf)
     const char wmo_delimiter[] = " ";
     char *save;
     char *awips_line_delimiter = strstr(data, "\r\r\n");
     char *wmo_message_delimiter = strstr(awips_line_delimiter + 3, "\r\r\n");
     memset(awips_line_delimiter, 0, 3);
     memset(wmo_message_delimiter, 0, 3);

     char *wmo_line = data;
     char *awips_line = awips_line_delimiter + 3;
     char *wmo_message_end = wmo_message_delimiter + 3;


     char *wmo_line_parts[4];
     for(int i = 0; i < 4; i++) {
          wmo_line_parts[i] = strtok_r(i ? NULL : wmo_line, wmo_delimiter, &save);
     }

     return wmo_message_end;
}

void process(char *data) {
     char *start = data;
     wmo_header test;
     message_header_block *test2 = NULL;
     product_description_block *test3 = NULL;
     product_symbology_block *test4 = NULL;
     symbology_layer *test5 = NULL;
     radial_data_packet *test6 = NULL;
     radial *test7 = NULL;

     data = parse_wmo(data, &test);
     test2 = (message_header_block *)data;
     load_message_header_block(test2);
     test3 = (product_description_block *)(data + sizeof(message_header_block));
     load_product_description_block(test3);
     test4 = (product_symbology_block *)(((char *)test3) + sizeof(product_description_block));
     load_product_symbology_block(test4);
     test5 = (symbology_layer *)(((char *)test4) + sizeof(product_symbology_block));
     load_symbology_layer(test5);
     test6 = (radial_data_packet *)(((char *)test5) + sizeof(symbology_layer));
     load_radial_data_packet(test6);
     test6->packet_code == 0xAF1F;
     char *radial_start = (((char *)test6) + sizeof(radial_data_packet));
     fprintf(stderr, "Index of start: %i\n", test6->index_of_range_bin);
     for(int i = 0; i < test6->number_of_radials; i++) {
          int total = 0;
          test7 = (radial *)radial_start;
          load_radial(test7);
          if(test7->delta_angle != 10) {
               fprintf(stderr, "Oh no! Angle (%i) != 10: %i\n", test7->delta_angle, radial_start - start);
               //exit(1);
          } else {
               fprintf(stderr, "Made it! %i: %i \n", test7->delta_angle, radial_start - start);
          }
          radial_start += sizeof(radial);
          uint8_t *rles = (uint8_t *)radial_start;
          for(int j = 0; j < test7->number_of_rle * 2; j++) {
               rle radial_data;
               radial_data.run = LOAD_RUN(rles[j]);
               radial_data.code = LOAD_CODE(rles[j]);
               total += radial_data.run;
               printf("Run: %i Code: %i\n", radial_data.run, radial_data.code);
               radial_start++;
          }
          printf("Total: %i\n", total);
     }
}
