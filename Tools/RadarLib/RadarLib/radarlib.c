#include "radarlib.h"
#include <stdlib.h>
#include <string.h>
#include <bzlib.h>

#define VERSION_PREFIX "AR2V00"
#define SetLastError(x) strncpy(context->last_error, x, sizeof(context->last_error));
#define CheckBounds(x) if(offset + x > context->input.length){ \
        SetLastError("Bounds check failed for offset"); \
        return RADAR_INVALID_DATA; \
        }


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

typedef struct __attribute__ ((packed)) volumeHeaderFileStruct {
    char tape[6];
    char version[3];
    char extension[3];
    int date;
    int time;
    char icao[4];
} VolumeHeaderFile;

typedef struct volumeHeaderStruct {
    uint8_t version;
    uint16_t extension;
    time_t datetime;
    char icao[5];
} VolumeHeader;

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

radar_status_t loadVolumeHeader(VolumeHeaderFile *in, VolumeHeader *out) {
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
    //fprintf(stderr, "%i - %i\n", ntohl(in->date), ntohl(in->time));
    return RADAR_OK;
}

radar_status_t messageHeaderLoad(MessageHeader * header)
{
    header->messageSize = ntohs(header->messageSize);
    header->numSegments = ntohs(header->numSegments);
    header->curSegment = ntohs(header->curSegment);
    return RADAR_OK;
}

radar_status_t scanHeaderLoad(ScanHeader * header)
{
    header->azimuthAngle = ntohf(header->azimuthAngle);
    header->azimuthNumber = ntohs(header->azimuthNumber)-1;
    header->elevation = ntohf(header->elevation);
    header->rlength = ntohs(header->rlength);
    header->dcount = ntohs(header->dcount);
    for (int i = 0; i < 9; i++) {
        header->datapointers[i] = ntohl(header->datapointers[i]);
    }
    return RADAR_OK;
}

radar_status_t dataBlockHeaderLoad(DataBlockHeader * header)
{
    return RADAR_OK;
}

radar_status_t refRecordLoad(RefRecord * record)
{
    record->numGates = ntohs(record->numGates);
    record->firstGateDistance = ntohs(record->firstGateDistance);
    record->gateDistanceInterval = ntohs(record->gateDistanceInterval);
    record->dataMomentScale = ntohf(record->dataMomentScale);
    record->dataMomentAddOffset = ntohf(record->dataMomentAddOffset);
    return RADAR_OK;
}

radar_status_t volRecordLoad(VolRecord * record)
{
    record->latitude = ntohf(record->latitude);
    record->longitude = ntohf(record->longitude);
    return RADAR_OK;
}

radar_status_t decompressChunk(char *input, size_t inputLength,
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

radar_status_t create_context(RadarContext ** context){
    RadarContext * new_context = malloc(sizeof(RadarContext));
    memset(new_context, 0, sizeof(RadarContext));
    if(new_context == NULL){
        return RADAR_NOMEM;
    }
    *context = new_context;
    return RADAR_OK;
}

radar_status_t process_level2(RadarContext * context){
    int offset = 0;
    float siteLatitude = 999;
    float siteLongitude = 0;
    int numberOfRangeBins = 1832;
    int numberOfRadials = 720;
    int outputSize =  sizeof(int8_t) * numberOfRangeBins * numberOfRadials;
    int azimuthSize = numberOfRadials * sizeof(float);
    context->output.length = sizeof(RadarHeader) + azimuthSize + outputSize;
    context->output.data = malloc(context->output.length);
    
    RadarHeader * outputHeader = (RadarHeader *)context->output.data;
    outputHeader->number_of_bins = numberOfRangeBins;
    outputHeader->number_of_radials = numberOfRadials;
    outputHeader->each_bin_distance = 250;

    int8_t * data_array = (int8_t *) (context->output.data + sizeof(RadarHeader) + azimuthSize);
    float * azimuths = (float *)(context->output.data + sizeof(RadarHeader));
    
    if(data_array == NULL){
        SetLastError("Unable to allocate data array");
        return RADAR_NOMEM;
    }
    CheckBounds(sizeof(VolumeHeaderFile));
    
    VolumeHeaderFile * fileHeader = (VolumeHeaderFile *) context->input.data + offset;
    VolumeHeader header;
    loadVolumeHeader(fileHeader, &header);
    offset += sizeof(VolumeHeaderFile);
    
    fprintf(stderr, "ICAO: %s - %i %i - %lu\n", header.icao, header.extension, header.version, header.datetime);
    
    strncpy(outputHeader->callsign, header.icao, 4);
    outputHeader->scan_date = (uint32_t) header.datetime;
    
    // Go through the chunks
    short lastChunk = 0;
    while (!lastChunk) {
        ChunkHeader *header;
        CheckBounds(sizeof(ChunkHeader));
        header = (ChunkHeader *)(context->input.data + offset);
        header->chunkSize = ntohl(header->chunkSize);
        offset += sizeof(header->chunkSize);
        
        // If negative, this is the last chunk. Reverse sign to get size
        if (header->chunkSize < 0) {
            header->chunkSize *= -1;
            lastChunk = 1;
        }
        
        // All chuncks should be BZ compressed currently, we do not support uncompressed chunks.
        if (header->compressedSignifier[0] != 'B'
            || header->compressedSignifier[1] != 'Z') {
            SetLastError("Chunk was not compressed");
            return RADAR_INVALID_DATA;
        }
        CheckBounds(header->chunkSize);
        
        char *uncompressedPointer;
        size_t uncompressedSize;
        radar_status_t status = decompressChunk( context->input.data + offset , header->chunkSize, &uncompressedSize, &uncompressedPointer);
        if (status == RADAR_NOMEM) {
            SetLastError("Failed to allocate memory for decompression")
            goto memory_error;
        }
        if (status == RADAR_INVALID_DATA) {
            SetLastError("Invalid compressed data")
            goto invalid_error;
        }
        int record = 0;
        int message_offset31 = 0;
        
        while (1) {
            unsigned long offset = record * 2432 + message_offset31;
            record++;
            if (offset >= uncompressedSize) {
                break;
            }
            char *messageHeaderOffset = uncompressedPointer + offset;
            MessageHeader *messageHead = ((MessageHeader *) messageHeaderOffset);
            messageHeaderLoad(messageHead);
            //fprintf(stderr, " %i" , messageHead->messageType);
            
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
                if(scan->azimuthNumber > numberOfRadials-1) {
                    SetLastError("Azimuth number greater than number of radials-1")
                    goto invalid_error;
                }
                azimuths[scan->azimuthNumber] = scan->azimuthAngle;
                
                for (int i = 0; i < 9; i++) {
                    int dataPointer = scan->datapointers[i];
                    if (dataPointer > 0) {
                        char *dataBlockHeaderOffset = (scanHeaderOffset + dataPointer);
                        DataBlockHeader *dataBlock = ((DataBlockHeader *) dataBlockHeaderOffset);
                        dataBlockHeaderLoad(dataBlock);
                        if (strncmp(dataBlock->tname, "REF", 3) == 0) {
                            char *refRecordOffset = dataBlockHeaderOffset + sizeof(DataBlockHeader);
                            RefRecord *ref = (RefRecord *) refRecordOffset;
                            refRecordLoad(ref);
                            
                            outputHeader->first_bin_distance =ref->firstGateDistance;
                            uint8_t *gateOffset = (uint8_t *) (refRecordOffset + sizeof(RefRecord));
                            
                            if(ref->numGates != numberOfRangeBins) {
                                SetLastError("Invalid number of range bins")
                                goto invalid_error;
                            }
                            for (int k = 0; k < ref->numGates; k++) {
                                float curVal = gateOffset[k];
                                int8_t reflectivity = 0xFF;
                                float rawReflectivity = curVal * 0.5f - 33;  // change to moment scale andoffset
                                if(rawReflectivity > 0){ // change this to minimum value on scale?
                                    reflectivity = (int) rawReflectivity;
                                }
                                int data_offset = scan->azimuthNumber * numberOfRangeBins + k;
                                data_array[data_offset] = reflectivity;
                               
                            }
                        } else if (strncmp(dataBlock->tname, "VOL", 3) == 0 && siteLatitude == 999) {
                            char *volRecordOffset = dataBlockHeaderOffset + sizeof(DataBlockHeader);
                            VolRecord *vol = (VolRecord *) volRecordOffset;
                            volRecordLoad(vol);
                            siteLatitude = vol->latitude;
                            siteLongitude = vol->longitude;
                        }
                    }
                }
            }
        }
        offset += header->chunkSize;
        free(uncompressedPointer);
        //printf("size %i %c %c \n", header->chunkSize, header->compressedSignifier[0], header->compressedSignifier[1] );
    }
    SetLastError("No error");
    return RADAR_OK;
memory_error:
    free(data_array);
    return RADAR_NOMEM;
invalid_error:
    free(data_array);
    return RADAR_INVALID_DATA;
}
radar_status_t process_level3(RadarContext * context){
    SetLastError("Level 3 processing not implemented yet");
    return RADAR_NOT_IMPLEMENTED;
}

radar_status_t process(RadarContext * context, radar_format_t format){
    context->format = format;
    if(format == RADAR_LEVEL2){
        return process_level2(context);
    } else if(format == RADAR_LEVEL3){
        return process_level3(context);
    }
    return RADAR_INVALID_DATA;
}

radar_status_t create_output_data(RadarContext * context){
    return RADAR_NOT_IMPLEMENTED;
}
radar_status_t compress_output_data(RadarContext * context){
    return RADAR_NOT_IMPLEMENTED;
}
radar_status_t destroy_context(RadarContext * context){
    if(context->output.data != NULL){
        free(context->output.data);
    }
    if(context->compressed_output.data != NULL){
        free(context->compressed_output.data);
    }
    free(context);
    return RADAR_OK;
}






