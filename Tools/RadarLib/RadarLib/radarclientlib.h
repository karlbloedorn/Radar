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
#include <stdint.h>

typedef struct scaleBucketStruct {
     float dbz;
     int range[6];
} ScaleBucket;

typedef struct vertexColorStruct {
     uint8_t r;
     uint8_t g;
     uint8_t b;
     uint8_t a;
} VertexColor;

typedef struct gateColorsStruct {
     VertexColor colors[6];
} GateColors;

typedef struct vertexPositionStruct {
     float x;
     float y;
} VertexPosition;

typedef struct gateCoordinatesStruct {
     VertexPosition positions[6];
} GateCoordinates;

typedef struct vertexData{
     VertexPosition position;
     VertexColor color;
} VertexData;
typedef struct gateData{
     VertexData vertices[6];
} GateData;

int parse(char * pointer, int splits, int32_t ** gate_counts_ref, int32_t * radial_count_ref, GateData *** gate_data_ref );
void moveWithBearing(float lon1,
                     float sinLatitude,
                     float cosLatitude,
                     float * outLatitude,
                     float * outLongitude,
                     double cosBearing,
                     double sinBearing,
                     double dist,
                     double cosDist,
                     double sinDist
     );
double projectLatitudeMercator(double latitude);
double projectLongitudeMercator(double longitude);

#endif
