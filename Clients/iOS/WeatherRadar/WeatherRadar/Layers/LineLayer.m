//
//  LineLayer.m
//  WeatherRadar
//
//  Created by Karl Bloedorn on 10/3/14.
//  Copyright (c) 2014 Karl Bloedorn. All rights reserved.
//

#import "LineLayer.h"
#import <GLKit/GLKit.h>
#define BUFFER_OFFSET(i) ((char *)NULL + (i))

@implementation LineLayer{
    GLuint vbo;
    int lineCount;
    NSData * positionData;
}

-(instancetype) initWithData:(NSData *) data andLabel:(NSString *) label{
    self = [super init];
    if(self){
        self.label = label;
        positionData = data;
    }
    return self;
}

-(void) setup{
    lineCount = *((int *)[positionData bytes]);
    lineCount = ntohl(lineCount);
    self.isSetup = YES;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, positionData.length-4, [positionData bytes] + 4, GL_STATIC_DRAW);
    positionData = nil;
}

-(void) draw{
    if(self.isVisible){
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glEnableVertexAttribArray(GLKVertexAttribPosition);
        glVertexAttribPointer(GLKVertexAttribPosition, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
        glDrawArrays(GL_LINES, 0, lineCount * 2);

    }
}
@end

