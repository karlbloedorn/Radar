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
#include <string.h>


#define RADIUS_OF_EARTH 6378137
const double degrees90 = 90* M_PI / 180.0;
const double degrees360 = 360* M_PI / 180.0;
const double degrees180 = M_PI;

typedef struct projectionData{
    double * cosBearing;
    double * sinBearing;
    double * dist;
    double * cosDist;
    double * sinDist;
    float * xCoords;
    float * yCoords;
    float * azimuths;
    uint8_t * calculate;
} ProjectionData;

typedef struct projectionThreadArgs {
    int threadID;
    int numThreads;
    RadarHeader * header;
    ProjectionData * data;
} ProjectionThreadArgs;

void * process_projection_subset(void * input){
    ProjectionThreadArgs * args = input;
    RadarHeader * header = args->header;
    ProjectionData * data = args->data;
    
    for(int bin = args->threadID; bin < header->number_of_bins+1; bin += args->numThreads){
        data->dist[bin] = (header->first_bin_distance + bin*header->each_bin_distance )/ (RADIUS_OF_EARTH);
        data->cosDist[bin] = cos(data->dist[bin]);
        data->sinDist[bin] = sin(data->dist[bin]);
    }
    double cosLatitude = cos( header->latitude* M_PI / 180.0 );
    double sinLatitude = sin( header->latitude* M_PI / 180.0 );
    double radianLongitude = header->latitude* M_PI / 180.0;

    for(int radial = args->threadID; radial < header->number_of_radials; radial+=args->numThreads){
        double azimuthRadians = ntohf(data->azimuths[radial])* M_PI / 180;
        data->cosBearing[radial] = cos(azimuthRadians);
        data->sinBearing[radial] = sin(azimuthRadians);
        
        for(int bin = 0; bin < header->number_of_bins+1; bin++){
            
            if( data->calculate[radial*(header->number_of_bins+1)+ bin] ){
                moveWithBearing(radianLongitude,
                            sinLatitude,
                            cosLatitude,
                            data->yCoords + radial*header->number_of_bins + bin,
                            data->xCoords + radial*header->number_of_bins + bin,
                            data->cosBearing[radial],
                            data->sinBearing[radial],
                            data->dist[bin],
                            data->cosDist[bin],
                            data->sinDist[bin]);
            }
        }
             
    }
    return NULL;
}

int parse(char * pointer, int splits, int32_t ** gate_counts_ref, int32_t * radial_count_ref, GateData *** gate_data_ref ) {

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

    ProjectionData projectionData;
    projectionData.cosBearing = malloc(sizeof(double)* header->number_of_radials);
    projectionData.sinBearing = malloc(sizeof(double)* header->number_of_radials);
    projectionData.dist = malloc(sizeof(double)* (header->number_of_bins +1) );
    projectionData.cosDist = malloc(sizeof(double)* (header->number_of_bins +1) );
    projectionData.sinDist = malloc(sizeof(double)* (header->number_of_bins +1) );
    projectionData.xCoords = malloc(sizeof(float) * ( header->number_of_radials * (header->number_of_bins+1) ));
    projectionData.yCoords = malloc(sizeof(float) * ( header->number_of_radials * (header->number_of_bins+1) ));
    projectionData.calculate = malloc (sizeof(uint8_t) * ( header->number_of_radials * (header->number_of_bins+1) ) );
    projectionData.azimuths = azimuths;
    
    *radial_count_ref = header->number_of_radials;
    int32_t * gate_counts = malloc(sizeof(int32_t)*header->number_of_radials);
    *gate_counts_ref = gate_counts;
    //int32_t gate_counts[header->number_of_radials];
    memset(gate_counts, 0, sizeof(uint32_t)*header->number_of_radials);

    memset(projectionData.calculate, 0, sizeof(uint8_t) * ( header->number_of_radials * (header->number_of_bins+1) ));
    
    for(int radial = 0; radial < header->number_of_radials; radial++){
        for(int bin = 0; bin < header->number_of_bins; bin++){
            int8_t cur = data[radial*header->number_of_bins + bin];
            if(cur > 0){
                int radialNumberAfter = radial+1;
                if (radialNumberAfter == header->number_of_radials) {
                    radialNumberAfter = 0;
                }
                // bin and bin+1
                // radial and radialNumberAfter
                projectionData.calculate[radial*(header->number_of_bins+1)+ bin] = 1;
                projectionData.calculate[radial*(header->number_of_bins+1)+ bin+1] = 1;
                projectionData.calculate[radialNumberAfter*(header->number_of_bins+1)+ bin] = 1;
                projectionData.calculate[radialNumberAfter*(header->number_of_bins+1)+ bin+1] = 1;
                gate_counts[radial]++;
            }
            
        }
    }
    
    pthread_t threads[splits];
    ProjectionThreadArgs thread_args[splits];
    
    for (int i = 0; i < splits; i++) {
        thread_args[i].threadID = i;
        thread_args[i].numThreads = splits;
        thread_args[i].data = &projectionData;
        thread_args[i].header = header;
        pthread_create(&threads[i], NULL, process_projection_subset, &thread_args[i]);
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
    
    for (int i = 0; i < splits; i++) {
        pthread_join(threads[i], NULL);
    }

    int bytes = 0;
    
    
    GateData ** gateData = malloc (header->number_of_radials * sizeof(GateData *));
    //GateData * gateData[header->number_of_radials];
    *gate_data_ref = gateData;
    
    for(int radial = 0; radial < header->number_of_radials; radial++){
        int curGate = 0;

        gateData[radial] = malloc(sizeof(GateData) * gate_counts[radial]);
        bytes+=sizeof(GateData) * gate_counts[radial];
        
        for(int bin = 0; bin < header->number_of_bins; bin++){
            int8_t cur = data[radial*header->number_of_bins + bin];
            if(cur > 0){
                int radialNumberAfter = radial+1;
                if (radialNumberAfter == header->number_of_radials) {
                    radialNumberAfter = 0;
                }

                GateData * curGateData = &gateData[radial][curGate];
    
                curGateData->vertices[0].position.x = projectionData.xCoords[radial*(header->number_of_bins+1) + bin+1]; //topLeft.x
                curGateData->vertices[0].position.y = projectionData.yCoords[radial*(header->number_of_bins+1) + bin+1]; //topLeft.y
                curGateData->vertices[1].position.x = projectionData.xCoords[radialNumberAfter*(header->number_of_bins+1)  + bin+1]; //topRight.x
                curGateData->vertices[1].position.y = projectionData.yCoords[radialNumberAfter*(header->number_of_bins+1)  + bin+1]; //topRight.y
                curGateData->vertices[2].position.x = projectionData.xCoords[radialNumberAfter*(header->number_of_bins+1)  + bin]; //bottomRight.x
                curGateData->vertices[2].position.y = projectionData.yCoords[radialNumberAfter*(header->number_of_bins+1)  + bin]; //bottomRight.y
                curGateData->vertices[3].position.x = projectionData.xCoords[radialNumberAfter*(header->number_of_bins+1)  + bin]; //bottomRight.x
                curGateData->vertices[3].position.y = projectionData.yCoords[radialNumberAfter*(header->number_of_bins+1)  + bin]; //bottomRight.y
                curGateData->vertices[4].position.x = projectionData.xCoords[radial*(header->number_of_bins+1)  + bin]; //bottomLeft.x
                curGateData->vertices[4].position.y = projectionData.yCoords[radial*(header->number_of_bins+1)  + bin];//bottomLeft.y
                curGateData->vertices[5].position.x = projectionData.xCoords[radial*(header->number_of_bins+1)  + bin+1]; //topLeft.x
                curGateData->vertices[5].position.y = projectionData.yCoords[radial*(header->number_of_bins+1)  + bin+1]; //topLeft.y

                for (int z = 0; z < 6; z++) {
                    curGateData->vertices[z].color.r = colors[cur][0];
                    curGateData->vertices[z].color.g = colors[cur][1];
                    curGateData->vertices[z].color.b = colors[cur][2];
                    curGateData->vertices[z].color.a = 255;
                }
                curGate++;
            }
        }
    }
    free(projectionData.cosBearing);
    free(projectionData.sinBearing);
    free(projectionData.dist);
    free(projectionData.cosDist);
    free(projectionData.sinDist);
    free(projectionData.yCoords);
    free(projectionData.xCoords);
    return bytes;
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
    *outLatitude =  projectLatitudeMercator( lat2);
    *outLongitude = projectLongitudeMercator( lon2);
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
