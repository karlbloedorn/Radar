//
//  LineLayer.m
//  WeatherRadar
//
//  Created by Karl Bloedorn on 10/3/14.
//  Copyright (c) 2014 Karl Bloedorn. All rights reserved.
//

#import "LineLayer.h"
#import <GLKit/GLKit.h>


@implementation LineLayer{
    GLuint vbo;
    int lineCount;
}

-(instancetype) initWithData:(NSData *) data andLabel:(NSString *) label{
    self = [super init];
    if(self){
        self.label = label;
        // load file here into buffer.
    }
    return self;
}

-(void) setup{
    
}

-(void) draw:(GLuint) program{
    
}
@end

