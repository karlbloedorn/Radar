//
//  LayerSelectorViewController.h
//  WeatherRadar
//
//  Created by Karl Bloedorn on 10/4/14.
//  Copyright (c) 2014 Karl Bloedorn. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface LayerSelectorViewController : UIViewController <UITableViewDataSource, UITableViewDelegate>

@property (strong, nonatomic) NSArray * lineLayers;

@end
