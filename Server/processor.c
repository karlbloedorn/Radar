//
//  processor.c
//  RadarProcessor
//
//  Created by Karl Bloedorn and Peter Irish on 9/23/14.
//  Copyright (c) 2014 Karl Bloedorn and Peter Irish. All rights reserved.
//

#include <fcntl.h>
#include <math.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#include "bzlib.h"

#define RADIUS_OF_EARTH 6371000.0

typedef struct __attribute__((packed)) chunkHeaderStruct  {
    int32_t chunkSize;
    char compressedSignifier[2];
} ChunkHeader;

typedef struct messageHeaderStruct {
    char pad1[12];
    uint16_t messageSize;
    char idChannel;
    uint8_t messageType;
    char pad2[8];
    uint16_t numSegments;
    uint16_t curSegment;
} MessageHeader;

typedef struct scanHeaderStruct {
    char identifier[4];
    uint32_t collectionTime;
    uint16_t collectionDate;
    uint16_t azimuthNumber;
    float azimuthAngle;
    uint8_t compressedFlag;
    uint8_t sp;
    int16_t rlength;
    uint8_t ars;
    uint8_t rs;
    uint8_t elevationNum;
    uint8_t cut;
    float elevation;
    uint8_t rsbs;
    uint8_t aim;
    uint16_t dcount;
    uint32_t datapointers[9];
} ScanHeader;

typedef struct dataBlockHeaderStruct{
    char pad1[1];
    char tname[3];
    char pad2[4];
} DataBlockHeader;

typedef struct refRecordStruct{
    uint16_t numGates;
    uint16_t firstGateDistance;
    uint16_t gateDistanceInterval;
    uint8_t ref_rf_threshold;
    uint8_t ref_snr_threshold;
    uint8_t controlFlags;
    uint8_t dataWordSize;
    char pad3[2];
    float dataMomentScale;
    float dataMomentAddOffset;
} RefRecord;

typedef struct volRecordStruct{
    float latitude;
    float longitude;
} VolRecord;


typedef struct gateInfoStruct{
    float distBegin;
    float distEnd;
    int azimuthNumber;
    float reflectivity;
} GateInfo;

typedef struct vertexPositionStruct{
    float x;
    float y;
} VertexPosition;

typedef struct vertexColorStruct{
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} VertexColor;

typedef struct gateColorStruct{
    VertexColor colors[6];
} GateColors;

typedef struct gateCoordinatesStruct{
    VertexPosition positions[6];
} GateCoordinates;

typedef struct scaleBucketStruct{
    float dbz;
    int range[6];
} ScaleBucket;

void moveWithBearing(float originLatitude, float originLongitude, float distanceMeters, float bearingDegrees, float * outLatitude, float * outLongitude);
void projectLatitudeMercator( double latitude, float * projectedLatitude);
void projectLongitudeMercator( double longitude, float * projectedLongitude);
void decompressChunk(char * input, size_t inputLength, size_t * uncompressedSizeOut, char ** resultOut);
void messageHeaderLoad(MessageHeader * header);
float ntohf(float input);
void scanHeaderLoad(ScanHeader * header);
void dataBlockHeaderLoad(DataBlockHeader * header);
void refRecordLoad(RefRecord * record);
void volRecordLoad(VolRecord * record);


char *socket_path = "\0radar-processor";;
int process(int fd){
    char * uncompressedFile = malloc(1);
    size_t sum = 0;

    char fileheader[24];
    int bytesRead0 = 0;
    while(bytesRead0 < 24){
        int bytes = read(fd, fileheader + bytesRead0 , 24 - bytesRead0);
        if(bytes <= 0){
            exit(1);
        }
        bytesRead0 += bytes;
    }

    short last = 0;
    int chunkIndex = 0;

    while(!last){
        ChunkHeader header;

        int bytesRead1 = 0;
        while(bytesRead1 < sizeof(ChunkHeader)){
            int bytes = read(fd, ((char*)&header)+ bytesRead1 , sizeof(ChunkHeader) - bytesRead1);
            if(bytes <= 0){
                exit(1);
            }
            bytesRead1 += bytes;
        }
        header.chunkSize = ntohl(header.chunkSize);
        // If this is the last chunk, the size will be negative.
        if(header.chunkSize < 1){
            header.chunkSize *= -1;
            last = 1;
        }
        if (header.chunkSize == 0){
            continue;
        }
        // Checking if this chunk is compressed.
   
        if(header.compressedSignifier[0] != 'B' && header.compressedSignifier[1] != 'Z'){
          exit(200);
        }

        char * compressedContents = malloc(header.chunkSize);
        compressedContents[0] = 'B';
        compressedContents[1] = 'Z';

        int chunkRealSize = header.chunkSize-2;
        int bytesRead2 = 0;
        while(bytesRead2 < chunkRealSize){
            int bytes = read(fd, 2+ compressedContents + bytesRead2, chunkRealSize-bytesRead2);
            if(bytes <= 0){
                exit(1);
            }
            bytesRead2 += bytes;
        }

        //printf("pointer : %p length: %i isCompressed %i\n", header->compressedSignifier, header->chunkSize, isCompressed);

        char * uncompressedPointer;
        size_t uncompressedSize;
        decompressChunk(compressedContents, header.chunkSize, &uncompressedSize, &uncompressedPointer);

        size_t newSize = sum + uncompressedSize;
        uncompressedFile = realloc(uncompressedFile, newSize);
        memcpy(uncompressedFile + sum, uncompressedPointer, uncompressedSize);
        free(uncompressedPointer);
        sum = newSize;

        chunkIndex++;
    }
    int record = 0;
    int message_offset31 = 0;
    float siteLatitude = 999;
    float siteLongitude = 0;
    float azimuths[721];
    GateInfo * gates = malloc(1);
    int gateCount = 0;
    int gateAllocated = 0;

    int numBuckets = 9;
    ScaleBucket buckets[numBuckets ];

    int scaleValues[9][7] = {
        {80, 128, 128, 128, 0,     0,   0},
        {70, 255, 255, 255, 0,     0,   0},
        {60, 255,   0, 255, 128,   0, 128},
        {50, 255,   0,   0, 160,   0,   0},
        {40, 255, 255,   0, 255, 128,   0},
        {30, 0,   255,   0, 0  , 128,   0},
        {20, 64 , 128, 255, 32 ,  64, 128},
        {10, 164, 164, 255, 100, 100, 192},
        {-10,64,   64,  64, 164, 164, 164}
    };
    for (int i = 0; i < 9; i++) {
        buckets[i].dbz = scaleValues[i][0];
        for (int j = 0; j < 7; j++) {
            buckets[i].range[j] = scaleValues[i][j+1];
        }
    }
    buckets[0].dbz = 80;

    while (1){
        unsigned long offset = record * 2432  + message_offset31;
        record++;
        if (offset >= sum){
            break;
        }
        char * messageHeaderOffset = uncompressedFile + offset;
        MessageHeader * messageHead = ( (MessageHeader *) messageHeaderOffset);
        messageHeaderLoad(messageHead);
        //NSLog(@"header message size: %i type: %i",header->messageSize, header->messageType);

        if (messageHead->messageType == 31){
            message_offset31 += (messageHead->messageSize * 2 + 12 - 2432);

            char * scanHeaderOffset = messageHeaderOffset + sizeof(MessageHeader);

            ScanHeader * scan = ((ScanHeader *) scanHeaderOffset);
            scanHeaderLoad(scan);

            if (scan->ars != 1){
                //not HI res.
                continue;
            }
            if (scan->elevationNum != 1){
                continue;
            }
            azimuths[scan->azimuthNumber] = scan->azimuthAngle;

            //NSLog(@"%c%c%c%c found HI res data with elevation 1 ars 1 azimuth: %f",scan->identifier[0],scan->identifier[1], scan->identifier[2],scan->identifier[3],  scan->azimuthAngle);

            for (int i = 0; i < 9; i++){
                int dataPointer = scan->datapointers[i];
                if(dataPointer > 0){
                    char * dataBlockHeaderOffset = (scanHeaderOffset + dataPointer);
                    DataBlockHeader * dataBlock = ((DataBlockHeader *) dataBlockHeaderOffset);
                    dataBlockHeaderLoad(dataBlock);
                    if(strncmp(dataBlock->tname, "REF", 3) == 0){
                        char * refRecordOffset = dataBlockHeaderOffset + sizeof(DataBlockHeader);
                        RefRecord * ref = (RefRecord *) refRecordOffset;
                        refRecordLoad(ref);
                        uint8_t * gateOffset = (uint8_t *) (refRecordOffset + sizeof(RefRecord));

                        int firstIndex = 0;
                        float firstVal = gateOffset[0];
                        for(int k = 1; k < ref->numGates; k++){  //0 without optimization
                            float curVal = gateOffset[k];
                            int saveSet = 0;
                            if(curVal != firstVal){ //abs(curVal - firstVal) > 3){
                                saveSet = 1;
                            }
                            if(k == ref->numGates -1){
                                // last gate.
                                saveSet = 1;
                            }
                            if(saveSet){
                                float reflectivity = firstVal * 0.5f - 33;   // change to moment scale andoffset
                                if(reflectivity >= 1){
                                    if(gateAllocated <= gateCount){
                                        gateAllocated += 5000;
                                        gates = realloc(gates, sizeof(GateInfo)*gateAllocated);
                                    }
                                    GateInfo * gate = &gates[gateCount];
                                    gate->reflectivity = reflectivity;
                                    gate->azimuthNumber =scan->azimuthNumber;
                                    gate->distBegin =ref->firstGateDistance + ref->gateDistanceInterval * firstIndex;
                                    gate->distEnd =ref->firstGateDistance + ref->gateDistanceInterval * k + 250;
                                    gateCount++;
                                }
                                firstIndex = k;
                                firstVal = curVal;
                            }

                        }
                    } else if (strncmp(dataBlock->tname, "VOL", 3) == 0 && siteLatitude == 999){
                        char * volRecordOffset = dataBlockHeaderOffset + sizeof(DataBlockHeader);
                        VolRecord * vol = (VolRecord *) volRecordOffset;
                        volRecordLoad(vol);
                        siteLatitude = vol->latitude;
                        siteLongitude = vol->longitude;
                    }
                }
            }
        }
    }
    unsigned long outputLength = (sizeof(GateColors) + sizeof(GateCoordinates))*gateCount;
    char * outputPointer = malloc(outputLength);
    GateCoordinates * gateCoordinatesPointer = (GateCoordinates *)outputPointer;
    GateColors * gateColorsPointer = (GateColors *) (outputPointer + sizeof(GateCoordinates) *gateCount);

    //GateData * vertexArray = malloc(sizeof(GateData) * gateCount);

    for (int i = 0; i < gateCount; i++) {
        GateInfo gate = gates[i];

        GateCoordinates * curgateCoordinates = gateCoordinatesPointer + i;
        GateColors * curgateColors = gateColorsPointer + i;

        int azimuthNumberAfter = gate.azimuthNumber+1;
        if(azimuthNumberAfter == 721){
            azimuthNumberAfter = 1;
        }

        float azimuth = azimuths[gate.azimuthNumber];
        float azimuthAfter = azimuths[azimuthNumberAfter];
    //    GateData * gateData = vertexArray + i;
        VertexPosition topLeft;
        moveWithBearing(siteLatitude, siteLongitude, gate.distEnd, azimuthAfter, &topLeft.y, &topLeft.x);
        VertexPosition topRight;
        moveWithBearing(siteLatitude, siteLongitude, gate.distEnd, azimuth, &topRight.y, &topRight.x);
        VertexPosition bottomRight;
        moveWithBearing(siteLatitude, siteLongitude, gate.distBegin, azimuth, &bottomRight.y, &bottomRight.x);
        VertexPosition bottomLeft;
        moveWithBearing(siteLatitude, siteLongitude, gate.distBegin, azimuthAfter, &bottomLeft.y, &bottomLeft.x);
        projectLongitudeMercator(topLeft.x, &curgateCoordinates->positions[0].x);
        projectLatitudeMercator(topLeft.y, &curgateCoordinates->positions[0].y);
        projectLongitudeMercator(topRight.x, &curgateCoordinates->positions[1].x);
        projectLatitudeMercator(topRight.y, &curgateCoordinates->positions[1].y);
        projectLongitudeMercator(bottomRight.x, &curgateCoordinates->positions[2].x);
        projectLatitudeMercator(bottomRight.y, &curgateCoordinates->positions[2].y);
        projectLongitudeMercator(bottomRight.x, &curgateCoordinates->positions[3].x);
        projectLatitudeMercator(bottomRight.y, &curgateCoordinates->positions[3].y);
        projectLongitudeMercator(bottomLeft.x, &curgateCoordinates->positions[4].x);
        projectLatitudeMercator(bottomLeft.y, &curgateCoordinates->positions[4].y);
        projectLongitudeMercator(topLeft.x, &curgateCoordinates->positions[5].x);
        projectLatitudeMercator(topLeft.y, &curgateCoordinates->positions[5].y);

        int bucket = -1;
        for(int j = 0; j < numBuckets; j++){
            if( gate.reflectivity > buckets[j].dbz )
            {
                bucket = j;
                break;
            }
        }
        int gateColor[3];
        gateColor[0] = 0;
        gateColor[1] = 0;
        gateColor[2] = 0;

        switch (bucket)
        {
            case 0:
            case -1:
                //off scale.
                break;
            case 1:
            default:
            {
                int * lowColor;
                int * highColor;
                ScaleBucket * scaleBucket = &buckets[bucket];
                if(bucket == 1){
                    lowColor = &scaleBucket->range[0];
                    highColor = &scaleBucket->range[0];
                } else {
                    lowColor = &scaleBucket->range[3];
                    highColor = &scaleBucket->range[0];
                }
                float range = buckets[bucket-1].dbz -  buckets[bucket].dbz;
                float percentForLowColor = (gate.reflectivity - buckets[bucket].dbz) / range;
                float percentForHighColor = 1 - percentForLowColor;
                gateColor[0] = (percentForHighColor * highColor[0] + percentForLowColor * lowColor[0]);
                gateColor[1] = (percentForHighColor * highColor[1] + percentForLowColor * lowColor[1]);
                gateColor[2] = (percentForHighColor * highColor[2] + percentForLowColor * lowColor[2]);

                break;
            }
        }

        for (int z = 0; z< 6; z++) {
            curgateColors->colors[z].r = gateColor[0];
            curgateColors->colors[z].g = gateColor[1];
            curgateColors->colors[z].b = gateColor[2];
            curgateColors->colors[z].a = 200;
        }
    }

    free(gates);
    free(uncompressedFile);
    write(1, outputPointer, outputLength);
    free(outputPointer);
    return 0;
}


int main(int argc, char *argv[]) {
  struct sockaddr_un addr;
  int fd,cl;

  if (argc > 1) {
    socket_path = argv[1];
  }

  if ( (fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
    perror("socket error");
    exit(-1);
  }

  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path)-1);

  unlink(socket_path);

  if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
    perror("bind error");
    exit(-1);
  }

  if (listen(fd, 5) == -1) {
    perror("listen error");
    exit(-1);
  }

  pid_t  pid;

  for(int i = 0; i < 10; i++) {
    pid = fork();
    if (pid == 0) {
      break;
    }
  }


  while (1) {
    if ( (cl = accept(fd, NULL, NULL)) == -1) {
      perror("accept error");
      continue;
    }
    process(cl);
    close(cl);
  }
  return 0;
}


void moveWithBearing(float originLatitude, float originLongitude, float distanceMeters, float bearingDegrees, float * outLatitude, float * outLongitude){
    float bearing = bearingDegrees * M_PI/180.0;

    const double distRadians = distanceMeters / (RADIUS_OF_EARTH);
    float lat1 = originLatitude * M_PI / 180;
    float lon1 = originLongitude * M_PI / 180;
    float lat2 = asin( sin(lat1) * cos(distRadians) + cos(lat1) * sin(distRadians) * cos(bearing));
    float lon2 = lon1 + atan2( sin(bearing) * sin(distRadians) * cos(lat1),
                              cos(distRadians) - sin(lat1) * sin(lat2) );
    *outLatitude = lat2 * 180 / M_PI;
    *outLongitude = lon2 * 180 / M_PI;
}

void projectLatitudeMercator( double latitude, float * projectedLatitude){
    const int mapWidth = 360;
    const int mapHeight = 180;

    // convert from degrees to radians
    float latRad = latitude * M_PI / 180;

    // get y value
    float mercN = log(tan((M_PI / 4) + (latRad / 2)));
    *projectedLatitude = (mapHeight / 2) - (mapWidth * mercN / (2 * M_PI));
}

void projectLongitudeMercator( double longitude, float * projectedLongitude){
    const int mapWidth = 360;
    //const int mapHeight = 180;

    // get x value
    *projectedLongitude = (longitude + 180) * (mapWidth / 360.0);
}

void decompressChunk(char * input, size_t inputLength, size_t * uncompressedSizeOut, char ** resultOut) {
    int BLOCK_SIZE = 1024*100;
    char *base = malloc(BLOCK_SIZE);
    bz_stream stream = { 0 };
    stream.next_in = input;
    stream.avail_in = (int)inputLength;
    stream.next_out = base;
    stream.avail_out = BLOCK_SIZE;
    BZ2_bzDecompressInit(&stream, 0, 0); // == BZ_OK;

    size_t uncompressed_size = 0;
    int status = BZ_STREAM_END;
    while((status = BZ2_bzDecompress(&stream)) == BZ_OK) {
        uncompressed_size += BLOCK_SIZE - stream.avail_out;
        base = realloc(base, uncompressed_size + BLOCK_SIZE);
        stream.next_out = base + uncompressed_size;
        stream.avail_out = BLOCK_SIZE;
    }
    uncompressed_size += BLOCK_SIZE - stream.avail_out;

    base = realloc(base, uncompressed_size);
    BZ2_bzDecompressEnd(&stream); // == BZ_OK;

    *uncompressedSizeOut = uncompressed_size;
    *resultOut = base;
}

void messageHeaderLoad(MessageHeader * header){
    header->messageSize = ntohs(header->messageSize);
    header->numSegments = ntohs(header->numSegments);
    header->curSegment = ntohs(header->curSegment);
}

float ntohf(float input){
    int32_t converted = ntohl( *((int32_t *) &input));
    return *(float *)&converted;
}

void scanHeaderLoad(ScanHeader * header){
    header->azimuthAngle = ntohf(header->azimuthAngle);
    header->azimuthNumber = ntohs(header->azimuthNumber);
    header->elevation = ntohf(header->elevation);
    header->rlength = ntohs(header->rlength);
    header->dcount = ntohs(header->dcount);
    for (int i = 0; i < 9; i++) {
        header->datapointers[i] = ntohl(header->datapointers[i]);
    }
}

void dataBlockHeaderLoad(DataBlockHeader * header){

}

void refRecordLoad(RefRecord * record){
    record->numGates = ntohs(record->numGates);
    record->firstGateDistance = ntohs(record->firstGateDistance);
    record->gateDistanceInterval = ntohs(record->gateDistanceInterval);
    record->dataMomentScale = ntohf(record->dataMomentScale);
    record->dataMomentAddOffset = ntohf(record->dataMomentAddOffset);
}

void volRecordLoad(VolRecord * record){
    record->latitude = ntohf(record->latitude);
    record->longitude = ntohf(record->longitude);
}
