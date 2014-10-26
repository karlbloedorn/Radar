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
const double degrees90 = 90* M_PI / 180.0;
const double degrees360 = 360* M_PI / 180.0;
const double degrees180 = M_PI;






typedef struct processThreadArgs {
    pthread_mutex_t * lock;
    int * curAzimuth;
    int8_t * data;
    RadarHeader * header;
    float * azimuths;
    uint32_t * gate_counts;
    GateCoordinates ** position_pointers;
    GateColors ** color_pointers;
} ProcessThreadArgs;

void *process_radial_subset(void * input) {
    ProcessThreadArgs * args = input;
    int8_t * data  = args->data;
    RadarHeader * header = args->header;
    float * azimuths = args->azimuths;
    
    double cosBearing[header->number_of_radials];
    double sinBearing[header->number_of_radials];
    double dist[header->number_of_bins+1];
    double cosDist[header->number_of_bins+1];
    double sinDist[header->number_of_bins+1];
    
    for(int i = 0; i < header->number_of_radials; i++){
        double azimuthRadians = azimuths[i]* M_PI / 180;
        cosBearing[i] = cos(azimuthRadians);
        sinBearing[i] = sin(azimuthRadians);
    }
    for(int i = 0; i < header->number_of_bins+1; i++){
        dist[i] = (header->first_bin_distance + i*header->each_bin_distance )/ (RADIUS_OF_EARTH);
        cosDist[i] = cos(dist[i]);
        sinDist[i] = sin(dist[i]);
    }
    
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
    
    
    

    
    while(1){
        // critical section
        pthread_mutex_lock(args->lock);
        int i = *args->curAzimuth;
        *args->curAzimuth = i+1;
        pthread_mutex_unlock(args->lock);

        // end critical section
        if ( i >= header->number_of_radials){
            break;
        }
        
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

        gateCount = 0;
        int azimuthNumberAfter = i+1;
        if (azimuthNumberAfter == header->number_of_radials) {
            azimuthNumberAfter = 0;
        }
        
        /*
        float azimuth = azimuths[i];
        float azimuthAfter = azimuths[azimuthNumberAfter];
        
        float azimuthRadians = azimuth * M_PI / 180;
        float azimuthAfterRadians = azimuthAfter * M_PI / 180;*/
        
        for (int j = 0; j < header->number_of_bins; j++) {
            
            int8_t cur = data[i*header->number_of_bins + j];
            if(cur <= 0){
                continue;
            }
            
            //float distBegin = header->first_bin_distance + j*header->each_bin_distance;
            //float distEnd = header->first_bin_distance + (j+1)*header->each_bin_distance;
            
            
            
            VertexPosition topLeft;
            VertexPosition topRight;
            VertexPosition bottomRight;
            VertexPosition bottomLeft;
            
            
            /*
             (float originLatitude,
             float originLongitude,
             float * outLatitude,
             float * outLongitude,
             double cosBearing,
             double sinBearing,
             double dist,
             double cosDist,
             double sinDist*/
            
            /*
            moveWithBearing(header->latitude,
                            header->longitude,
                            &topLeft.y,
                            &topLeft.x,
                            cosBearing[azimuthNumberAfter],
                            sinBearing[azimuthNumberAfter],
                            dist[j+1],
                            cosDist[j+1],
                            sinDist[j+1]);
            
            moveWithBearing(header->latitude,
                            header->longitude,
                            &topRight.y,
                            &topRight.x,
                            cosBearing[i],
                            sinBearing[i],
                            dist[j+1],
                            cosDist[j+1],
                            sinDist[j+1]);
            
            moveWithBearing(header->latitude,
                            header->longitude,
                            &topRight.y,
                            &topRight.x,
                            cosBearing[i],
                            sinBearing[i],
                            dist[j],
                            cosDist[j],
                            sinDist[j]);
            
            moveWithBearing(header->latitude,
                            header->longitude,
                            &topLeft.y,
                            &topLeft.x,
                            cosBearing[azimuthNumberAfter],
                            sinBearing[azimuthNumberAfter],
                            dist[j],
                            cosDist[j],
                            sinDist[j]);
            */
           // moveWithBearing(header->latitude, header->longitude, distBegin, azimuthRadians, &bottomRight.y, &bottomRight.x);
           // moveWithBearing(header->latitude, header->longitude, distBegin, azimuthAfterRadians, &bottomLeft.y, &bottomLeft.x);
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


float parse(char * pointer, int splits) {
    
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
    
    double * cosBearing = malloc(sizeof(double)* header->number_of_radials);
    double * sinBearing = malloc(sizeof(double)* header->number_of_radials);
    double * dist = malloc(sizeof(double)* (header->number_of_bins +1) );
    double * cosDist = malloc(sizeof(double)* (header->number_of_bins +1) );
    double * sinDist = malloc(sizeof(double)* (header->number_of_bins +1) );
    float * xCoords = malloc(sizeof(float) * ( header->number_of_radials * header->number_of_bins ));
    float * yCoords = malloc(sizeof(float) * ( header->number_of_radials * header->number_of_bins ));
    
    for(int i = 0; i < header->number_of_radials; i++){
        double azimuthRadians = ntohf(azimuths[i])* M_PI / 180;
        cosBearing[i] = cos(azimuthRadians);
        sinBearing[i] = sin(azimuthRadians);
    }
    for(int i = 0; i < header->number_of_bins+1; i++){
        dist[i] = (header->first_bin_distance + i*header->each_bin_distance )/ (RADIUS_OF_EARTH);
        cosDist[i] = cos(dist[i]);
        sinDist[i] = sin(dist[i]);
    }
    double cosLatitude = cos( header->latitude* M_PI / 180.0 );
    double sinLatitude = sin( header->latitude* M_PI / 180.0 );

    for(int i = 0; i < header->number_of_radials; i++){
        for(int j = 0; j < header->number_of_bins+1; j++){
            moveWithBearing(header->longitude* M_PI / 180.0,
                            sinLatitude,
                            cosLatitude,
                            yCoords + i*header->number_of_radials + j,
                            xCoords + i*header->number_of_radials + j,
                            cosBearing[i],
                            sinBearing[i],
                            dist[j],
                            cosDist[j],
                            sinDist[j]);
        }
    }
    float a = xCoords[0]+ yCoords[0];
    free(cosBearing);
    free(sinBearing);
    free(dist);
    free(cosDist);
    free(sinDist);
    free(yCoords);
    free(xCoords);

    return a;
    

    GateCoordinates * position_pointers[header->number_of_radials];
    GateColors * color_pointers[header->number_of_radials];
    uint32_t gate_counts[header->number_of_radials];

    
    int8_t * data = (int8_t *) (pointer + offset);
    pthread_t threads[splits];
    ProcessThreadArgs thread_args[splits];
    
    int curAzimuth = 0;
    pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_init(&lock, NULL);
    for (int i = 0; i < splits; i++) {
        thread_args[i].curAzimuth = &curAzimuth;
        thread_args[i].lock = &lock;
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

    pthread_mutex_destroy(&lock);
    return 0.0f;
}

inline float ntohf(float input)
{
    int32_t converted = ntohl(*((int32_t *) & input));
    return *(float *) &converted;
}

inline float htonf(float input)
{
    int32_t converted = htonl(*((int32_t *) & input));
    return *(float *) &converted;
}

inline void moveWithBearing(float lon1,
                            float sinLatitude,
                            float cosLatitude,
                            float * outLatitude,
                            float * outLongitude,
                            double cosBearing,
                            double sinBearing,
                            double dist,
                            double cosDist,
                            double sinDist
                            ){
    float lat2 = asin(sinLatitude * cosDist + cosLatitude * sinDist * cosBearing);
    float lon2 = lon1 + atan2(sinBearing * sinDist * cosLatitude, cosDist - sinLatitude * sin(lat2));
    *outLatitude = lat2;
    *outLongitude = lon2;
}

inline double projectLatitudeMercator(double latRad)
{
   
    float mercN = log(tan((M_PI / 4) + (latRad / 2)));
    return degrees90 - (degrees360 * mercN / (2 * M_PI));
}

inline double projectLongitudeMercator(double longitude)
{
    return (longitude + degrees180);
}
