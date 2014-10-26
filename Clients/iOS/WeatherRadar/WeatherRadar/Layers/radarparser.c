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
#include <stdlib.h>

#define RADIUS_OF_EARTH 6378137

typedef struct processThreadArgs {
    //int startAzimuth;
    //int endAzimuth;
    int threadID;
    int threadCount;
    int8_t * data;
    RadarHeader * header;
    float * azimuths;
    uint32_t * gate_counts;
    GateCoordinates ** position_pointers;
    GateColors ** color_pointers;
} ProcessThreadArgs;

void *process_radial_subset(void * input) {
    
    ScaleBucket buckets[9];
    
    int scaleValues[9][7] = {
        {80, 128, 128, 128, 0, 0, 0},
        {70, 255, 255, 255, 0, 0, 0},
        {60, 255, 0, 255, 128, 0, 128},
        {50, 255, 0, 0, 160, 0, 0},
        {40, 255, 255, 0, 255, 128, 0},
        {30, 0, 255, 0, 0, 128, 0},
        {20, 64, 128, 255, 32, 64, 128},
        {10, 164, 164, 255, 100, 100, 192},
        {-10, 64, 64, 64, 164, 164, 164}
    };
    
    for (int i = 0; i < 9; i++) {
        buckets[i].dbz = scaleValues[i][0];
        for (int j = 0; j < 6; j++) {
            buckets[i].range[j] = scaleValues[i][j + 1];
        }
    }
    uint8_t colors[255][3];
    
    for(int cur = 0; cur < 255; cur++){
        int bucket = -1;
        for (int j = 0; j < 9; j++) {
            if (cur > buckets[j].dbz) {
                bucket = j;
                break;
            }
        }
        colors[cur][0] = 0;
        colors[cur][1] = 0;
        colors[cur][2] = 0;
        
        switch (bucket) {
            case 0:
            case -1:
                //off scale.
                break;
            case 1:
            default:
            {
                int *lowColor;
                int *highColor;
                ScaleBucket *scaleBucket = &buckets[bucket];
                if (bucket == 1) {
                    lowColor = &scaleBucket->range[0];
                    highColor = &scaleBucket->range[0];
                } else {
                    lowColor = &scaleBucket->range[3];
                    highColor = &scaleBucket->range[0];
                }
                float range = buckets[bucket - 1].dbz - buckets[bucket].dbz;
                float percentForLowColor = (cur - buckets[bucket].dbz) / range;
                float percentForHighColor = 1 - percentForLowColor;
                colors[cur][0] = (percentForHighColor * highColor[0] + percentForLowColor * lowColor[0]);
                colors[cur][1] = (percentForHighColor * highColor[1] + percentForLowColor * lowColor[1]);
                colors[cur][2] = (percentForHighColor * highColor[2] + percentForLowColor * lowColor[2]);
                break;
            }
        }
    }
    
    
    
    ProcessThreadArgs * args = input;
    int8_t * data  = args->data;
    RadarHeader * header = args->header;
    float * azimuths = args->azimuths;
    
    for( int i = args->threadID; i < header->number_of_radials;i+= args->threadCount){
    //for (int i = 0; i < header->number_of_radials; i++) {
        //if( (i %  args->threadCount) != args->threadID ){
        //    continue;
        //}
        int gateCount = 0;
        for (int j = 0; j < header->number_of_bins; j++) {
            int8_t cur = data[i*header->number_of_bins + j];
            if(cur > 0){
                gateCount++;
            }
        }
        args->color_pointers[i] = malloc(gateCount*sizeof(GateColors));
        args->position_pointers[i] = malloc(gateCount*sizeof(GateCoordinates));
        args->gate_counts[i] = gateCount;
    }
    //printf("startAzimuth: %i: %i, %f MB\n",args->startAzimuth,gates, (gates*4*3*6 )/1024.0/1024.0 );
    
    for( int i = args->threadID; i < header->number_of_radials;i+= args->threadCount){
    //for (int i = 0; < header->number_of_radials; i++) {
        //if( (i %  args->threadCount) != args->threadID ){
        //    continue;
        //}
        int gateCount = 0;
        int azimuthNumberAfter = i+1;
        if (azimuthNumberAfter == header->number_of_radials) {
            azimuthNumberAfter = 0;
        }
        float azimuth = azimuths[i];
        float azimuthAfter = azimuths[azimuthNumberAfter];
        for (int j = 0; j < header->number_of_bins; j++) {
            
            int8_t cur = data[i*header->number_of_bins + j];
            if(cur <= 0){
                continue;
            }
            
            float distBegin = header->first_bin_distance + j*header->each_bin_distance;
            float distEnd = header->first_bin_distance + (j+1)*header->each_bin_distance;;
            VertexPosition topLeft;
            VertexPosition topRight;
            VertexPosition bottomRight;
            VertexPosition bottomLeft;
            moveWithBearing(header->latitude, header->longitude, distEnd, azimuthAfter, &topLeft.y, &topLeft.x);
            moveWithBearing(header->latitude, header->longitude, distEnd, azimuth,  &topRight.y, &topRight.x);
            moveWithBearing(header->latitude, header->longitude, distBegin, azimuth, &bottomRight.y, &bottomRight.x);
            moveWithBearing(header->latitude, header->longitude, distBegin, azimuthAfter, &bottomLeft.y, &bottomLeft.x);
            GateCoordinates * curgateCoordinates = &args->position_pointers[i][gateCount];
            GateColors * curgateColors = &args->color_pointers[i][gateCount];            
            curgateCoordinates->positions[0].x = projectLongitudeMercator(topLeft.x);
            curgateCoordinates->positions[0].y =  projectLatitudeMercator(topLeft.y);
            curgateCoordinates->positions[1].x =  projectLongitudeMercator(topRight.x);
            curgateCoordinates->positions[1].y =  projectLatitudeMercator(topRight.y);
            curgateCoordinates->positions[2].x = projectLongitudeMercator(bottomRight.x);
            curgateCoordinates->positions[2].y = projectLatitudeMercator(bottomRight.y);
            curgateCoordinates->positions[3].x = curgateCoordinates->positions[2].x;
            curgateCoordinates->positions[3].y = curgateCoordinates->positions[2].y;
            curgateCoordinates->positions[4].x = projectLongitudeMercator(bottomLeft.x);
            curgateCoordinates->positions[4].y = projectLatitudeMercator(bottomLeft.y);
            curgateCoordinates->positions[5].x = curgateCoordinates->positions[0].x;
            curgateCoordinates->positions[5].y = curgateCoordinates->positions[0].y;
            for (int z = 0; z < 6; z++) {
                curgateColors->colors[z].r = colors[cur][0];
                curgateColors->colors[z].g = colors[cur][1];
                curgateColors->colors[z].b = colors[cur][2];
                curgateColors->colors[z].a = 255;
            }
            gateCount++;
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
    
    GateCoordinates * position_pointers[header->number_of_radials];
    GateColors * color_pointers[header->number_of_radials];
    uint32_t gate_counts[header->number_of_radials];
    offset += sizeof(RadarHeader);
    
    float * azimuths = (float *)( pointer +offset);
    offset += sizeof(float) * header->number_of_radials;
    
    int8_t * data = (int8_t *) (pointer + offset);
    pthread_t threads[splits];
    ProcessThreadArgs thread_args[splits];
    
    //int split_size = header->number_of_radials / splits;
    
    for (int i = 0; i < splits; i++) {
        //thread_args[i].startAzimuth = i*split_size;
        /*
        if(splits - 1 == i){
            // last thread
            thread_args[i].endAzimuth = header->number_of_radials - 1;
        } else {
            thread_args[i].endAzimuth = (i+1) * split_size - 1;
        }
         */
       // printf("split: %i %i\n", thread_args[i].startAzimuth, thread_args[i].endAzimuth);
        
        thread_args[i].threadID = i;
        thread_args[i].threadCount = splits;
        thread_args[i].data = data;
        thread_args[i].header = header;
        thread_args[i].azimuths = azimuths;
        thread_args[i].color_pointers = color_pointers;
        thread_args[i].position_pointers = position_pointers;
        thread_args[i].gate_counts = gate_counts;
        pthread_create(&threads[i], NULL, process_radial_subset, &thread_args[i]);
    }
    
    for (int i = 0; i < splits; i++) {
        pthread_join(threads[i], NULL);
    }
    //process_radial_subset(&thread_args[0]);
    
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

double projectLatitudeMercator(double latitude)
{
    // convert from degrees to radians
    float latRad = latitude * M_PI / 180;
    
    // get y value
    float mercN = log(tan((M_PI / 4) + (latRad / 2)));
    return 90 - (360 * mercN / (2 * M_PI));
}

double projectLongitudeMercator(double longitude)
{
    return (longitude + 180);
}
