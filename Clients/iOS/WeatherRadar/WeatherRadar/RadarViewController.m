//
//  ViewController.m
//  WeatherRadar
//
//  Created by Karl Bloedorn on 10/2/14.
//  Copyright (c) 2014 Karl Bloedorn. All rights reserved.
//

#import "RadarViewController.h"
#import "GADBannerView.h"
#import "GADRequest.h"

@interface RadarViewController ()

@property (strong, nonatomic) IBOutlet GADBannerView *bannerView;

@end

@implementation RadarViewController

- (void)viewDidLoad {
    [super viewDidLoad];

    self.bannerView.adUnitID = @"ca-app-pub-5636726170867832/9790740103";
    self.bannerView.rootViewController = self;
    
    GADRequest *request = [GADRequest request];
    request.testDevices = @[ GAD_SIMULATOR_ID ];
    [self.bannerView loadRequest:request];
}

@end
