//
//  radarparser.h
//  WeatherRadar
//
//  Created by Karl Bloedorn on 10/13/14.
//  Copyright (c) 2014 Karl Bloedorn. All rights reserved.
//

#ifndef __WeatherRadar__radarparser__
#define __WeatherRadar__radarparser__

#include <stdio.h>

typedef struct vertexPositionStruct {
    float x;
    float y;
} VertexPosition;

typedef struct __attribute__((packed)) radarHeader  {
    char magic[4];
    uint16_t version;
    char callsign[4];
    uint8_t op_mode;
    uint8_t radar_status;
    int32_t scan_type;
    uint32_t scan_date; // epoch seconds. this will overflow in 2038. fix before then.
    float latitude;
    float longitude;
    uint32_t number_of_radials;
    uint32_t number_of_bins;
    float first_bin_distance;
    float each_bin_distance;
    uint32_t crc32;
} RadarHeader;
typedef struct gateCoordinatesStruct {
    VertexPosition positions[6];
} GateCoordinates;

void parse(char * pointer, int splits);
float ntohf(float input);
float htonf(float input);
void moveWithBearing(float originLatitude, float originLongitude,
                     float distanceMeters, float bearingDegrees,
                     float *outLatitude, float *outLongitude);
void projectLatitudeMercator(double latitude, float *projectedLatitude);
void projectLongitudeMercator(double longitude, float *projectedLongitude);

#endif
