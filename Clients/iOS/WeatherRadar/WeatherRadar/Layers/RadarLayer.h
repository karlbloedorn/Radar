//
//  RadarLayer.h
//  WeatherRadar
//
//  Created by Karl Bloedorn on 10/3/14.
//  Copyright (c) 2014 Karl Bloedorn. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <GLKit/GLKit.h>

@interface RadarLayer : NSObject

@property (strong, nonatomic) NSString * label;
@property (strong, nonatomic) NSData * colorBuffer;
@property (strong, nonatomic) NSData * positionBuffer;
@property BOOL isVisible;
@property BOOL isSetup;

-(instancetype) initWithData:(NSData *) data andLabel:(NSString *) label;
-(void) setup;
-(void) draw;

@end
