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


@implementation RadarLayer{
    GLuint vbo;
    int triangleCount;
    NSData * vertexData;
}


-(instancetype) initWithData:(NSData *) data andLabel:(NSString *) label{
    self = [super init];
    if(self){

        
        self.label = label;
        
        dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
            vertexData = data;

            char * bytes = malloc(data.length);
            [vertexData getBytes:bytes length:data.length];
            
            NSDate *date = [NSDate date];
            float a = parse(bytes, [[NSProcessInfo processInfo] activeProcessorCount]*8);
            double timePassed_ms = [date timeIntervalSinceNow] * -1000.0;
            
            dispatch_async(dispatch_get_main_queue(), ^{
                UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"Time" message:[NSString stringWithFormat: @"time: %f for length data: %lu val:%f", timePassed_ms, (unsigned long)[vertexData length], a] delegate:nil cancelButtonTitle:@"OK" otherButtonTitles:nil];
                [alert show];
            });
            
        });
        
        
        
    }
    return self;
}
-(void) setup{
    triangleCount = (int)(vertexData.length / 12 /3);
    self.isSetup = YES;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertexData.length, [vertexData bytes], GL_STATIC_DRAW);
    vertexData = nil;
}
-(void) draw{
    if(self.isVisible){
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glEnableVertexAttribArray(GLKVertexAttribPosition);
        glVertexAttribPointer(GLKVertexAttribPosition, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
        
        glEnableVertexAttribArray(GLKVertexAttribColor);
        glVertexAttribPointer(GLKVertexAttribColor, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, BUFFER_OFFSET(2*sizeof(float)*triangleCount*3));
        glDrawArrays(GL_TRIANGLES, 0, triangleCount * 3);
    }
}
@end
