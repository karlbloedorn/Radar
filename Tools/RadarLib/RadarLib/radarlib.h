#ifndef __RadarLib__radarlib__
#define __RadarLib__radarlib__

#include <stdio.h>

typedef enum { RADAR_NOMEM, RADAR_INVALID_DATA, RADAR_OK } radar_status_t;

typedef struct __attribute__((packed)) radarContext  {
    
} RadarContext;

typedef struct __attribute__((packed)) radarHeader  {
    char callsign[4];
    int32_t scan_type;
    int32_t scan_date; // epoch seconds
    float latitude;
    float longitude;
    uint32_t number_of_radials;
    uint32_t number_of_bins;
    float first_bin_distance;
    float each_bin_distance;
} RadarHeader;

radar_status_t create_context();
radar_status_t process();
radar_status_t create_output_data();
radar_status_t compress_output_data();
radar_status_t destroy_context();



#endif
