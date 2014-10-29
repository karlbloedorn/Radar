//
//  RadarLayer.m
//  WeatherRadar
//
//  Created by Karl Bloedorn on 10/3/14.
//  Copyright (c) 2014 Karl Bloedorn. All rights reserved.
//

#import "RadarLayer.h"
#import "radarparser.h"

#define BUFFER_OFFSET(i) ((char *)NULL + (i))

@implementation RadarLayerData


-(instancetype) initWithRadialCount:(int32_t) radial_count withGateCounts: (int32_t * )gate_counts withGateData: (GateData ** )gate_data {
    
    self = [super init];
    if(self){
        self.radial_count = radial_count;
        self.gate_counts = gate_counts;
        self.gate_data = gate_data;
    }
    return self;
}

-(void) setBuffers: (GLuint *) buffers{
    self.vbos = buffers;
}

@end

@implementation RadarLayer{
    GLuint vbo;
    int triangleCount;
    NSData * vertexData;
    RadarLayerData * data;
}
-(instancetype) initWithData:(RadarLayerData *) datas andLabel:(NSString *) label{
    self = [super init];
    if(self){
        data = datas;
    }
    return self;
}

-(void) setup{
    /*
    triangleCount = (int)(vertexData.length / 12 /3);
    self.isSetup = YES;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertexData.length, [vertexData bytes], GL_STATIC_DRAW);
    vertexData = nil;*/
}
-(void) draw{
   // if(self.isVisible){
    
        for (int i = 0; i < data.radial_count; i++) { //
            glBindBuffer(GL_ARRAY_BUFFER, data.vbos[i]);
            glEnableVertexAttribArray(GLKVertexAttribPosition);
            glEnableVertexAttribArray(GLKVertexAttribColor);
            glVertexAttribPointer(GLKVertexAttribPosition, 2, GL_FLOAT, GL_FALSE, sizeof(VertexColor) + sizeof(VertexPosition), BUFFER_OFFSET(0));
            glVertexAttribPointer(GLKVertexAttribColor, 4, GL_UNSIGNED_BYTE, GL_TRUE,sizeof(VertexColor) + sizeof(VertexPosition), BUFFER_OFFSET(sizeof(VertexPosition)));            
            glDrawArrays(GL_TRIANGLES, 0,data.gate_counts[i]*6);
        }
        
        /*
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glEnableVertexAttribArray(GLKVertexAttribPosition);
        glVertexAttribPointer(GLKVertexAttribPosition, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
        
        glEnableVertexAttribArray(GLKVertexAttribColor);
        glVertexAttribPointer(GLKVertexAttribColor, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, BUFFER_OFFSET(2*sizeof(float)*triangleCount*3));
        glDrawArrays(GL_TRIANGLES, 0, triangleCount * 3);
         */
    //}
}
@end
