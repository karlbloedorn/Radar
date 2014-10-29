//
//  RadarLayer.h
//  WeatherRadar
//
//  Created by Karl Bloedorn on 10/3/14.
//  Copyright (c) 2014 Karl Bloedorn. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <GLKit/GLKit.h>
#import "radarparser.h"

@interface RadarLayerData : NSObject

@property int32_t radial_count;
@property int32_t * gate_counts;
@property GateData ** gate_data;
@property GLuint * vbos;

-(instancetype) initWithRadialCount:(int32_t) radial_count withGateCounts: (int32_t * )gate_counts withGateData: (GateData ** )gate_data ;

@end


@interface RadarLayer : NSObject

@property (strong, nonatomic) NSString * label;
@property (strong, nonatomic) NSData * colorBuffer;
@property (strong, nonatomic) NSData * positionBuffer;
@property BOOL isVisible;
@property BOOL isSetup;
-(instancetype) initWithData:(RadarLayerData *) datas andLabel:(NSString *) label;
//-(instancetype) initWithData:(NSData *) data andLabel:(NSString *) label;
-(void) setup;
-(void) draw;

@end
