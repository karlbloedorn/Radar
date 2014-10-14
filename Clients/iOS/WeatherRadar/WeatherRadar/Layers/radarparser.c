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
#define RADIUS_OF_EARTH 6371000.0

void parse(char * pointer){
    int offset = 0;
    RadarHeader * header = (RadarHeader *)(pointer + offset);
    printf("%c%c%c%c\n", header->callsign[0], header->callsign[1],header->callsign[2],header->callsign[3]);
    
    
    header->each_bin_distance = ntohf(header->each_bin_distance);
    header->first_bin_distance = ntohf(header->first_bin_distance);
    header->latitude = ntohf(header->latitude);
    header->longitude = ntohf(header->longitude);
    header->number_of_bins = ntohl(header->number_of_bins);
    header->number_of_radials = ntohl(header->number_of_radials);
    
    float siteLatitude = header->latitude;
    float siteLongitude = header->longitude;
    
    offset += sizeof(RadarHeader);
    
    float * azimuths = (float *)( pointer +offset);
    offset += sizeof(float) * header->number_of_radials;
    
    int8_t * data = (int8_t *) (pointer + offset);
    GateCoordinates gateCoordin;

    GateCoordinates * curgateCoordinates = &gateCoordin;

    
    
    for (int i = 0; i < header->number_of_radials; i++) {
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
            moveWithBearing(siteLatitude, siteLongitude, distEnd, azimuthAfter, &topLeft.y, &topLeft.x);
            VertexPosition topRight;
            moveWithBearing(siteLatitude, siteLongitude, distEnd, azimuth,  &topRight.y, &topRight.x);
            VertexPosition bottomRight;
            moveWithBearing(siteLatitude, siteLongitude, distBegin, azimuth, &bottomRight.y, &bottomRight.x);
            VertexPosition bottomLeft;
            moveWithBearing(siteLatitude, siteLongitude, distBegin, azimuthAfter, &bottomLeft.y, &bottomLeft.x);
            
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
