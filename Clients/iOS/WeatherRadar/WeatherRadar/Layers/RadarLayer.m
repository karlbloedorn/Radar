//
//  RadarLayer.m
//  WeatherRadar
//
//  Created by Karl Bloedorn on 10/3/14.
//  Copyright (c) 2014 Karl Bloedorn. All rights reserved.
//

#import "RadarLayer.h"

@implementation RadarLayer

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
-(void) draw{
    
}
@end
