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
#include <errno.h>
#include <err.h>
#include <unistd.h>

#include <bzlib.h>
#include <zlib.h>

#define RADIUS_OF_EARTH 6371000.0
#define NUMBUCKETS 9
#define VERSION_PREFIX "AR2V00"

typedef enum { RADAR_NOMEM, RADAR_INVALID_DATA, RADAR_OK } radar_errors_t;

typedef struct __attribute__ ((packed)) chunkHeaderStruct {
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

typedef struct dataBlockHeaderStruct {
     char pad1[1];
     char tname[3];
     char pad2[4];
} DataBlockHeader;

typedef struct refRecordStruct {
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

typedef struct volRecordStruct {
     float latitude;
     float longitude;
} VolRecord;

typedef struct gateInfoStruct {
     float distBegin;
     float distEnd;
     int azimuthNumber;
     float reflectivity;
} GateInfo;

typedef struct vertexPositionStruct {
     float x;
     float y;
} VertexPosition;

typedef struct vertexColorStruct {
     uint8_t r;
     uint8_t g;
     uint8_t b;
     uint8_t a;
} VertexColor;

typedef struct gateColorStruct {
     VertexColor colors[6];
} GateColors;

typedef struct gateCoordinatesStruct {
     VertexPosition positions[6];
} GateCoordinates;

typedef struct scaleBucketStruct {
     float dbz;
     int range[6];
} ScaleBucket;

typedef struct __attribute__ ((packed)) volumeHeaderFileStruct {
     char tape[6];
     char version[3];
     char extension[3];
     int date;
     int time;
     char icao[4];
} volumeHeaderFile;

typedef struct volumeHeaderStruct {
     uint8_t version;
     uint16_t extension;
     time_t datetime;
     char icao[5];
} volumeHeader;

radar_errors_t loadVolumeHeader(volumeHeaderFile *in, volumeHeader *out) {
     char version[3];
     char extension[4];
     if(strncmp(VERSION_PREFIX, in->tape, sizeof(VERSION_PREFIX) - 1)) {
          fprintf(stderr, "Failed to match prefix -%s- != -%s-!\n", in->tape, VERSION_PREFIX);
     }
     strncpy(version, in->version, 2);
     strncpy(extension, in->extension, 3);
     strncpy(out->icao, in->icao, 4);
     version[2] = '\0';
     extension[3] = '\0';
     out->icao[4] = '\0';
     out->version = (uint8_t)atoi(version);
     out->extension = (uint16_t)atoi(extension);
     out->datetime = (ntohl(in->date) - 1) * 86400 + (ntohl(in->time)/1000);
     fprintf(stderr, "%i - %i\n", ntohl(in->date), ntohl(in->time));
     return RADAR_OK;
}

float ntohf(float input);
float htonf(float input);
void moveWithBearing(float originLatitude, float originLongitude,
                     float distanceMeters, float bearingDegrees,
                     float *outLatitude, float *outLongitude);
void projectLatitudeMercator(double latitude, float *projectedLatitude);
void projectLongitudeMercator(double longitude, float *projectedLongitude);
radar_errors_t gzip_data(unsigned char *data, size_t data_len, size_t *compressedSize, unsigned char **compressedData);
radar_errors_t decompressChunk(char *input, size_t inputLength,
                               size_t * uncompressedSizeOut,
                               char **resultOut);
radar_errors_t messageHeaderLoad(MessageHeader * header);
radar_errors_t scanHeaderLoad(ScanHeader * header);
radar_errors_t dataBlockHeaderLoad(DataBlockHeader * header);
radar_errors_t refRecordLoad(RefRecord * record);
radar_errors_t volRecordLoad(VolRecord * record);

void nomem_error(int fd)
{
}

void invalid_error(int fd)
{
}

radar_errors_t process(int fd, char **output, size_t *output_length)
{
     *output = NULL;
     *output_length = 0;
     char *uncompressedFile = malloc(1);
     radar_errors_t status = RADAR_OK;
     if (uncompressedFile == NULL) {
          goto memory_error;
     }
     size_t sum = 0;

     volumeHeaderFile fileHeader;
     int bytesRead0 = 0;
     while (bytesRead0 < sizeof(volumeHeaderFile)) {
          int bytes = read(fd, ((char *)&fileHeader) + bytesRead0, sizeof(volumeHeaderFile) - bytesRead0);
          if (bytes <= 0) {
               exit(1);
          }
          bytesRead0 += bytes;
     }
     volumeHeader header;
     loadVolumeHeader(&fileHeader, &header);
     fprintf(stderr, "ICAO: %s - %i %i - %lu\n", header.icao, header.extension, header.version, header.datetime);

     short last = 0;
     int chunkIndex = 0;

     while (!last) {
          ChunkHeader header;
          int bytesRead1 = 0;
          while (bytesRead1 < (int) sizeof(ChunkHeader)) {
               int bytes = read(fd, ((char *) &header) + bytesRead1,
                                sizeof(ChunkHeader) - bytesRead1);
               if (bytes <= 0) {
                    exit(1);
               }
               bytesRead1 += bytes;
          }
          header.chunkSize = ntohl(header.chunkSize);

          // If this is the last chunk, the size will be negative.
          if (header.chunkSize < 1) {
               header.chunkSize *= -1;
               last = 1;
          }
          if (header.chunkSize == 0) {
               continue;
          }

          // Checking if this chunk is compressed.
          if (header.compressedSignifier[0] != 'B'
              && header.compressedSignifier[1] != 'Z') {
               invalid_error(fd);
               goto invalid_error;
          }

          char *compressedContents = malloc(header.chunkSize);
          if (compressedContents == NULL) {
               goto memory_error;
          }
          compressedContents[0] = 'B';
          compressedContents[1] = 'Z';

          int chunkRealSize = header.chunkSize - 2;
          int bytesRead2 = 0;
          while (bytesRead2 < chunkRealSize) {
               int bytes = read(fd, 2 + compressedContents + bytesRead2,
                                chunkRealSize - bytesRead2);
               if (bytes <= 0) {
                    exit(1);
               }
               bytesRead2 += bytes;
          }

          char *uncompressedPointer;
          size_t uncompressedSize;
          status =
               decompressChunk(compressedContents, header.chunkSize,
                               &uncompressedSize, &uncompressedPointer);
          if (status == RADAR_NOMEM) {
               goto memory_error;
          }
          if (status == RADAR_INVALID_DATA) {
               goto invalid_error;
          }

          size_t newSize = sum + uncompressedSize;
          uncompressedFile = realloc(uncompressedFile, newSize);
          if (uncompressedFile == NULL) {
               goto memory_error;
          }
          memcpy(uncompressedFile + sum, uncompressedPointer,
                 uncompressedSize);
          free(uncompressedPointer);
          sum = newSize;

          chunkIndex++;
     }
     int record = 0;
     int message_offset31 = 0;
     float siteLatitude = 999;
     float siteLongitude = 0;
     float azimuths[721];
     GateInfo *gates = NULL;
     int gateCount = 0;
     int gateAllocated = 0;

     ScaleBucket buckets[NUMBUCKETS];

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

     gates = malloc(1);
     if (gates == NULL) {
          goto memory_error;
     }

     for (int i = 0; i < 9; i++) {
          buckets[i].dbz = scaleValues[i][0];
          for (int j = 0; j < 6; j++) {
               buckets[i].range[j] = scaleValues[i][j + 1];
          }
     }
     buckets[0].dbz = 80;

     while (1) {
          unsigned long offset = record * 2432 + message_offset31;
          record++;
          if (offset >= sum) {
               break;
          }
          char *messageHeaderOffset = uncompressedFile + offset;
          MessageHeader *messageHead =
               ((MessageHeader *) messageHeaderOffset);
          messageHeaderLoad(messageHead);
          fprintf(stderr, " %i" , messageHead->messageType);

          if (messageHead->messageType == 31) {
               message_offset31 += (messageHead->messageSize * 2 + 12 - 2432);

               char *scanHeaderOffset =
                    messageHeaderOffset + sizeof(MessageHeader);

               ScanHeader *scan = ((ScanHeader *) scanHeaderOffset);
               scanHeaderLoad(scan);

               if (scan->ars != 1) {
                    //not HI res.
                    continue;
               }
               if (scan->elevationNum != 1) {
                    continue;
               }
               azimuths[scan->azimuthNumber] = scan->azimuthAngle;

               for (int i = 0; i < 9; i++) {
                    int dataPointer = scan->datapointers[i];
                    if (dataPointer > 0) {
                         char *dataBlockHeaderOffset =
                              (scanHeaderOffset + dataPointer);
                         DataBlockHeader *dataBlock = ((DataBlockHeader *)
                                                       dataBlockHeaderOffset);
                         dataBlockHeaderLoad(dataBlock);
                         if (strncmp(dataBlock->tname, "REF", 3)
                             == 0) {
                              char *refRecordOffset =
                                   dataBlockHeaderOffset +
                                   sizeof(DataBlockHeader);
                              RefRecord *ref = (RefRecord *)
                                   refRecordOffset;
                              refRecordLoad(ref);
                              uint8_t *gateOffset =
                                   (uint8_t *) (refRecordOffset +
                                                sizeof(RefRecord));

                              int firstIndex = 0;
                              float firstVal = gateOffset[0];
                              for (int k = 1; k < ref->numGates; k++) {   //0 without optimization
                                   float curVal = gateOffset[k];
                                   int saveSet = 0;
                                   if (curVal != firstVal) {   //abs(curVal - firstVal) > 3){
                                        saveSet = 1;
                                   }
                                   if (k == ref->numGates - 1) {
                                        // last gate.
                                        saveSet = 1;
                                   }
                                   if (saveSet) {
                                        float reflectivity = firstVal * 0.5f - 33;  // change to moment scale andoffset
                                        if (reflectivity >= 1) {
                                             if (gateAllocated <= gateCount) {
                                                  gateAllocated += 5000;
                                                  gates =
                                                       realloc(gates, sizeof(GateInfo)
                                                               * gateAllocated);
                                                  if (gates == NULL) {
                                                       goto memory_error;
                                                  }
                                             }
                                             GateInfo *gate = &gates[gateCount];
                                             gate->reflectivity = reflectivity;
                                             gate->azimuthNumber =
                                                  scan->azimuthNumber;
                                             gate->distBegin =
                                                  ref->firstGateDistance +
                                                  ref->gateDistanceInterval *
                                                  firstIndex;
                                             gate->distEnd =
                                                  ref->firstGateDistance +
                                                  ref->gateDistanceInterval * k +
                                                  250;
                                             gateCount++;
                                        }
                                        firstIndex = k;
                                        firstVal = curVal;
                                   }

                              }
                         } else if (strncmp(dataBlock->tname, "VOL", 3) == 0
                                    && siteLatitude == 999) {
                              char *volRecordOffset =
                                   dataBlockHeaderOffset +
                                   sizeof(DataBlockHeader);
                              VolRecord *vol = (VolRecord *)
                                   volRecordOffset;
                              volRecordLoad(vol);
                              siteLatitude = vol->latitude;
                              siteLongitude = vol->longitude;
                         }
                    }
               }
          }
     }
     unsigned long outputLength =
          (sizeof(GateColors) + sizeof(GateCoordinates)) * gateCount;
     char *outputPointer = malloc(outputLength);
     if (outputPointer == NULL) {
          goto memory_error;
     }
     GateCoordinates *gateCoordinatesPointer =
          (GateCoordinates *) outputPointer;
     GateColors *gateColorsPointer =
          (GateColors *) (outputPointer +
                          sizeof(GateCoordinates) * gateCount);

     for (int i = 0; i < gateCount; i++) {
          GateInfo gate = gates[i];

          GateCoordinates *curgateCoordinates = gateCoordinatesPointer + i;
          GateColors *curgateColors = gateColorsPointer + i;

          int azimuthNumberAfter = gate.azimuthNumber + 1;
          if (azimuthNumberAfter == 721) {
               azimuthNumberAfter = 1;
          }

          float azimuth = azimuths[gate.azimuthNumber];
          float azimuthAfter = azimuths[azimuthNumberAfter];

          VertexPosition topLeft;
          moveWithBearing(siteLatitude, siteLongitude, gate.distEnd,
                          azimuthAfter, &topLeft.y, &topLeft.x);
          VertexPosition topRight;
          moveWithBearing(siteLatitude, siteLongitude, gate.distEnd, azimuth,
                          &topRight.y, &topRight.x);
          VertexPosition bottomRight;
          moveWithBearing(siteLatitude, siteLongitude, gate.distBegin,
                          azimuth, &bottomRight.y, &bottomRight.x);
          VertexPosition bottomLeft;
          moveWithBearing(siteLatitude, siteLongitude, gate.distBegin,
                          azimuthAfter, &bottomLeft.y, &bottomLeft.x);
          projectLongitudeMercator(topLeft.x,
                                   &curgateCoordinates->positions[0].x);
          projectLatitudeMercator(topLeft.y,
                                  &curgateCoordinates->positions[0].y);
          projectLongitudeMercator(topRight.x,
                                   &curgateCoordinates->positions[1].x);
          projectLatitudeMercator(topRight.y,
                                  &curgateCoordinates->positions[1].y);
          projectLongitudeMercator(bottomRight.x,
                                   &curgateCoordinates->positions[2].x);
          projectLatitudeMercator(bottomRight.y,
                                  &curgateCoordinates->positions[2].y);
          projectLongitudeMercator(bottomRight.x,
                                   &curgateCoordinates->positions[3].x);
          projectLatitudeMercator(bottomRight.y,
                                  &curgateCoordinates->positions[3].y);
          projectLongitudeMercator(bottomLeft.x,
                                   &curgateCoordinates->positions[4].x);
          projectLatitudeMercator(bottomLeft.y,
                                  &curgateCoordinates->positions[4].y);
          projectLongitudeMercator(topLeft.x,
                                   &curgateCoordinates->positions[5].x);
          projectLatitudeMercator(topLeft.y,
                                  &curgateCoordinates->positions[5].y);
          int bucket = -1;
          for (int j = 0; j < NUMBUCKETS; j++) {
               if (gate.reflectivity > buckets[j].dbz) {
                    bucket = j;
                    break;
               }
          }
          int gateColor[3];
          gateColor[0] = 0;
          gateColor[1] = 0;
          gateColor[2] = 0;

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
               float range =
                    buckets[bucket - 1].dbz - buckets[bucket].dbz;
               float percentForLowColor =
                    (gate.reflectivity - buckets[bucket].dbz) / range;
               float percentForHighColor = 1 - percentForLowColor;
               gateColor[0] =
                    (percentForHighColor * highColor[0] +
                     percentForLowColor * lowColor[0]);
               gateColor[1] =
                    (percentForHighColor * highColor[1] +
                     percentForLowColor * lowColor[1]);
               gateColor[2] =
                    (percentForHighColor * highColor[2] +
                     percentForLowColor * lowColor[2]);

               break;
          }
          }

          for (int z = 0; z < 6; z++) {
               curgateColors->colors[z].r = gateColor[0];
               curgateColors->colors[z].g = gateColor[1];
               curgateColors->colors[z].b = gateColor[2];
               curgateColors->colors[z].a = 200;
          }
     }

     free(gates);
     free(uncompressedFile);

     *output_length = outputLength;
     *output = outputPointer;
     return RADAR_OK;

memory_error:
     return RADAR_NOMEM;
invalid_error:
     return RADAR_INVALID_DATA;
}

static void setnonblocking(int sock)
{
     int opts;

     opts = fcntl(sock,F_GETFL);
     if (opts < 0) {
          perror("fcntl(F_GETFL)");
          exit(EXIT_FAILURE);
     }
     opts = opts & (~O_NONBLOCK);
     if (fcntl(sock,F_SETFL,opts) < 0) {
          perror("fcntl(F_SETFL)");
          exit(EXIT_FAILURE);
     }
     return;
}

int main(int argc, char *argv[])
{
     int fd, cl;
     int stdin_mode = 0;

     if (argc <= 1) {
          exit(1);
     } else {
          if(argv[1][0] == '-') {
               stdin_mode = 1;
          } else {
               fd = atoi(argv[1]);
               if(fd < 0) {
                    exit(1);
               }
          }
     }

     while (1) {
          if(!stdin_mode) {
               if ((cl = accept(fd, NULL, NULL)) == -1) {
                    if(errno == EAGAIN || errno == EWOULDBLOCK) {
                         setnonblocking(fd);
                    } else {
                         perror("Accept error");
                    }
                    continue;
               }
          } else {
               cl = STDIN_FILENO;
          }
          char *output = NULL;
          radar_errors_t process_status = RADAR_OK;
          size_t output_len = 0;
          process_status = process(cl, &output, &output_len);
          if(process_status == RADAR_OK) {
               unsigned char *compressedData = NULL;
               size_t compressSize = 0;
               if(stdin_mode) {
                    gzip_data((unsigned char *)output, output_len, &compressSize, &compressedData);
                    free(output);
                    write(STDOUT_FILENO, compressedData, compressSize);
               } else {
                    gzip_data((unsigned char *)output, output_len, &compressSize, &compressedData);
                    free(output);
                    write(cl, compressedData, compressSize);
               }
               free(compressedData);
          } else {
          }
          close(cl);
          if(stdin_mode) {
               break;
          }
     }
     return 0;
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

radar_errors_t decompressChunk(char *input, size_t inputLength,
                               size_t * uncompressedSizeOut,
                               char **resultOut)
{
     int status = 0;
     int BLOCK_SIZE = 1024 * 100;
     char *base = malloc(BLOCK_SIZE);
     if (base == NULL) {
          return RADAR_NOMEM;
     }
     bz_stream stream = { 0 };
     stream.next_in = input;
     stream.avail_in = (int) inputLength;
     stream.next_out = base;
     stream.avail_out = BLOCK_SIZE;
     if ((status = BZ2_bzDecompressInit(&stream, 0, 0)) != BZ_OK) {
          if (status == BZ_MEM_ERROR) {
               return RADAR_NOMEM;
          } else {
               return RADAR_INVALID_DATA;
          }
     }

     size_t uncompressed_size = 0;
     status = BZ_STREAM_END;
     while ((status = BZ2_bzDecompress(&stream)) == BZ_OK) {
          uncompressed_size += BLOCK_SIZE - stream.avail_out;
          base = realloc(base, uncompressed_size + BLOCK_SIZE);
          if (base == NULL) {
               return RADAR_NOMEM;
          }
          stream.next_out = base + uncompressed_size;
          stream.avail_out = BLOCK_SIZE;
     }
     uncompressed_size += BLOCK_SIZE - stream.avail_out;
     if (status != BZ_STREAM_END) {
          if (status == BZ_MEM_ERROR) {
               return RADAR_NOMEM;
          } else {
               return RADAR_INVALID_DATA;
          }
     }

     base = realloc(base, uncompressed_size);
     if (base == NULL) {
          return RADAR_NOMEM;
     }

     status = BZ2_bzDecompressEnd(&stream);
     if (status != BZ_OK) {
          return RADAR_INVALID_DATA;
     }

     *uncompressedSizeOut = uncompressed_size;
     *resultOut = base;
     return RADAR_OK;
}

#define GZIP_CHUNK 4096

#define CALL_ZLIB(x) {                                                  \
        int status;                                                     \
        status = x;                                                     \
        if (status < 0) {                                               \
             return RADAR_INVALID_DATA;                                 \
        }                                                               \
    }

/* These are parameters to deflateInit2. See
   http://zlib.net/manual.html. */

#define windowBits 15
#define GZIP_ENCODING 16

radar_errors_t strm_init(z_stream * strm, int level)
{
    strm->zalloc = Z_NULL;
    strm->zfree  = Z_NULL;
    strm->opaque = Z_NULL;
    CALL_ZLIB(deflateInit2(strm, level, Z_DEFLATED,
                           windowBits | GZIP_ENCODING, 8,
                           Z_DEFAULT_STRATEGY));
}

radar_errors_t gzip_data(unsigned char *data, size_t data_len, size_t *compressedSize, unsigned char **compressedData)
{
    unsigned char *out = malloc(GZIP_CHUNK);
    size_t offset = 0;
    size_t memory_size = GZIP_CHUNK;
    z_stream strm;
    strm_init(&strm, 8);
    strm.next_in = (unsigned char *)data;
    strm.avail_in = data_len;
    do {
        int have;
        strm.avail_out = GZIP_CHUNK;
        strm.next_out = out + offset;
        CALL_ZLIB(deflate(&strm, Z_FINISH));
        have = GZIP_CHUNK - strm.avail_out;
        offset += have;
        memory_size += GZIP_CHUNK;
        out = realloc(out, memory_size);
    }
    while (strm.avail_out == 0);
    deflateEnd (&strm);
    out = realloc(out, offset);
    *compressedData = out;
    *compressedSize = offset;
    return RADAR_OK;
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


radar_errors_t messageHeaderLoad(MessageHeader * header)
{
     header->messageSize = ntohs(header->messageSize);
     header->numSegments = ntohs(header->numSegments);
     header->curSegment = ntohs(header->curSegment);
     return RADAR_OK;
}

radar_errors_t scanHeaderLoad(ScanHeader * header)
{
     header->azimuthAngle = ntohf(header->azimuthAngle);
     header->azimuthNumber = ntohs(header->azimuthNumber);
     header->elevation = ntohf(header->elevation);
     header->rlength = ntohs(header->rlength);
     header->dcount = ntohs(header->dcount);
     for (int i = 0; i < 9; i++) {
          header->datapointers[i] = ntohl(header->datapointers[i]);
     }
     return RADAR_OK;
}

radar_errors_t dataBlockHeaderLoad(DataBlockHeader * header)
{
     return RADAR_OK;
}

radar_errors_t refRecordLoad(RefRecord * record)
{
     record->numGates = ntohs(record->numGates);
     record->firstGateDistance = ntohs(record->firstGateDistance);
     record->gateDistanceInterval = ntohs(record->gateDistanceInterval);
     record->dataMomentScale = ntohf(record->dataMomentScale);
     record->dataMomentAddOffset = ntohf(record->dataMomentAddOffset);
     return RADAR_OK;
}

radar_errors_t volRecordLoad(VolRecord * record)
{
     record->latitude = ntohf(record->latitude);
     record->longitude = ntohf(record->longitude);
     return RADAR_OK;
}
