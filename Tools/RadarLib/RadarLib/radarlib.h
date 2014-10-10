#ifndef __RadarLib__radarlib__
#define __RadarLib__radarlib__

#include <stdio.h>

typedef enum { RADAR_LEVEL2, RADAR_LEVEL3 } radar_format_t;
typedef enum { RADAR_NOMEM, RADAR_INVALID_DATA, RADAR_NOT_IMPLEMENTED, RADAR_OK } radar_status_t;

typedef struct radarBuffer {
    char * data;
    size_t length;
} RadarBuffer;

typedef struct radarContext  {
    RadarBuffer input;
    RadarBuffer output;
    RadarBuffer compressed_output;
    char last_error[255];
    radar_format_t format;
} RadarContext;

typedef struct __attribute__((packed)) radarHeader  {
    char callsign[4];
    int32_t scan_type;
    uint32_t scan_date; // epoch seconds. this will overflow in 2038. fix before then.
    float latitude;
    float longitude;
    uint32_t number_of_radials;
    uint32_t number_of_bins;
    float first_bin_distance;
    float each_bin_distance;
} RadarHeader;

radar_status_t create_context(RadarContext ** context);
radar_status_t process(RadarContext * context, radar_format_t format);
radar_status_t create_output_data(RadarContext * context);
radar_status_t compress_output_data(RadarContext * context);
radar_status_t destroy_context(RadarContext * context);

#endif
