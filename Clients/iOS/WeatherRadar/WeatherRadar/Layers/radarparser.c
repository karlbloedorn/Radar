//
//  radarparser.c
//  WeatherRadar
//
//  Created by Karl Bloedorn on 10/13/14.
//  Copyright (c) 2014 Karl Bloedorn. All rights reserved.
//

#include "radarparser.h"
#include <math.h>
#include <sys/time.h>
#include <pthread.h>

#define RADIUS_OF_EARTH 6371000.0

typedef struct processThreadArgs {
    int startAzimuth;
    int endAzimuth;
    int8_t * data;
    GateCoordinates * curgateCoordinates;
    RadarHeader * header;
    float * azimuths;
} ProcessThreadArgs;

void *process_radial_subset(void * input) {
    ProcessThreadArgs * args = input;
    int8_t * data  = args->data;
    GateCoordinates * curgateCoordinates = args->curgateCoordinates;
    RadarHeader * header = args->header;
    float * azimuths = args->azimuths;
    
    int gates = 0;
    for (int i = args->startAzimuth; i < args->endAzimuth; i++) {
        for (int j = 0; j < header->number_of_bins; j++) {
            int8_t cur = data[i*header->number_of_bins + j];
            if(cur > 0){
                gates++;
            }
        }
    }
    printf("startAzimuth: %i: %i, %f MB\n",args->startAzimuth,gates, (gates*4*3*6 )/1024.0/1024.0 );
    
    
    
    for (int i = args->startAzimuth; i < args->endAzimuth; i++) {
        for (int j = 0; j < header->number_of_bins; j++) {
            
            int8_t cur = data[i*header->number_of_bins + j];
            if(cur <= 0){
                continue;
            }
            int azimuthNumberAfter = i+1;
            if (azimuthNumberAfter == header->number_of_radials) {
                azimuthNumberAfter = 0;
            }
            float azimuth = azimuths[i];
            float azimuthAfter = azimuths[azimuthNumberAfter];
            float distBegin = header->first_bin_distance + j*header->each_bin_distance;
            float distEnd = header->first_bin_distance + j+1*header->each_bin_distance;;
            
            VertexPosition topLeft;
            moveWithBearing(header->latitude, header->longitude, distEnd, azimuthAfter, &topLeft.y, &topLeft.x);
            VertexPosition topRight;
            moveWithBearing(header->latitude, header->longitude, distEnd, azimuth,  &topRight.y, &topRight.x);
            VertexPosition bottomRight;
            moveWithBearing(header->latitude, header->longitude, distBegin, azimuth, &bottomRight.y, &bottomRight.x);
            VertexPosition bottomLeft;
            moveWithBearing(header->latitude, header->longitude, distBegin, azimuthAfter, &bottomLeft.y, &bottomLeft.x);
            
            projectLongitudeMercator(topLeft.x,      &curgateCoordinates->positions[0].x);
            projectLatitudeMercator(topLeft.y,  &curgateCoordinates->positions[0].y);
            projectLongitudeMercator(topRight.x,   &curgateCoordinates->positions[1].x);
            projectLatitudeMercator(topRight.y, &curgateCoordinates->positions[1].y);
            projectLongitudeMercator(bottomRight.x, &curgateCoordinates->positions[2].x);
            projectLatitudeMercator(bottomRight.y,    &curgateCoordinates->positions[2].y);
            projectLongitudeMercator(bottomRight.x,    &curgateCoordinates->positions[3].x);
            projectLatitudeMercator(bottomRight.y,   &curgateCoordinates->positions[3].y);
            projectLongitudeMercator(bottomLeft.x,  &curgateCoordinates->positions[4].x);
            projectLatitudeMercator(bottomLeft.y, &curgateCoordinates->positions[4].y);
            projectLongitudeMercator(topLeft.x,    &curgateCoordinates->positions[5].x);
            projectLatitudeMercator(topLeft.y,  &curgateCoordinates->positions[5].y);
            
            
        }
    }
    return NULL;
    
}


void parse(char * pointer, int splits) {
    int offset = 0;
    RadarHeader * header = (RadarHeader *)(pointer + offset);
    printf("%c%c%c%c\n", header->callsign[0], header->callsign[1],header->callsign[2],header->callsign[3]);
    
    
    header->each_bin_distance = ntohf(header->each_bin_distance);
    header->first_bin_distance = ntohf(header->first_bin_distance);
    header->latitude = ntohf(header->latitude);
    header->longitude = ntohf(header->longitude);
    header->number_of_bins = ntohl(header->number_of_bins);
    header->number_of_radials = ntohl(header->number_of_radials);
    
    
    offset += sizeof(RadarHeader);
    
    float * azimuths = (float *)( pointer +offset);
    offset += sizeof(float) * header->number_of_radials;
    
    int8_t * data = (int8_t *) (pointer + offset);
    GateCoordinates gateCoordin;
    
    GateCoordinates * curgateCoordinates = &gateCoordin;

    pthread_t threads[splits];
    ProcessThreadArgs thread_args[splits];
    
    
    int split_size = header->number_of_radials / splits;
    
    for (int i = 0; i < splits; i++) {
        thread_args[i].startAzimuth = i*split_size;
        if(splits - 1 == i){
            // last thread
            thread_args[i].endAzimuth = header->number_of_radials - 1;
        } else {
            thread_args[i].endAzimuth = (i+1) * split_size - 1;
        }
        printf("split: %i %i\n", thread_args[i].startAzimuth, thread_args[i].endAzimuth);
        
        thread_args[i].data = data;
        thread_args[i].curgateCoordinates = curgateCoordinates;
        thread_args[i].header = header;
        thread_args[i].azimuths = azimuths;
        pthread_create(&threads[i], NULL, process_radial_subset, &thread_args[i]);
    }
    
    for (int i = 0; i < splits; i++) {
        pthread_join(threads[i], NULL);
    }
}






float ntohf(float input)
{
    int32_t converted = ntohl(*((int32_t *) & input));
    return *(float *) &converted;
}

float htonf(float input)
{
    int32_t converted = htonl(*((int32_t *) & input));
    return *(float *) &converted;
}

void moveWithBearing(float originLatitude, float originLongitude,
                     float distanceMeters, float bearingDegrees,
                     float *outLatitude, float *outLongitude)
{
    float bearing = bearingDegrees * M_PI / 180.0;
    
    const double distRadians = distanceMeters / (RADIUS_OF_EARTH);
    float lat1 = originLatitude * M_PI / 180;
    float lon1 = originLongitude * M_PI / 180;
    float lat2 =
    asin(sin(lat1) * cos(distRadians) +
         cos(lat1) * sin(distRadians) * cos(bearing));
    float lon2 = lon1 + atan2(sin(bearing) * sin(distRadians) * cos(lat1),
                              cos(distRadians) - sin(lat1) * sin(lat2));
    *outLatitude = lat2 * 180 / M_PI;
    *outLongitude = lon2 * 180 / M_PI;
}

void projectLatitudeMercator(double latitude, float *projectedLatitude)
{
    const int mapWidth = 360;
    const int mapHeight = 180;
    
    // convert from degrees to radians
    float latRad = latitude * M_PI / 180;
    
    // get y value
    float mercN = log(tan((M_PI / 4) + (latRad / 2)));
    *projectedLatitude = (mapHeight / 2) - (mapWidth * mercN / (2 * M_PI));
}

void projectLongitudeMercator(double longitude, float *projectedLongitude)
{
    const int mapWidth = 360;
    //const int mapHeight = 180;
    
    // get x value
    *projectedLongitude = (longitude + 180) * (mapWidth / 360.0);
}
