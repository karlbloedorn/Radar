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
#import <GLKit/GLKit.h>
#import <OpenGLES/ES2/gl.h>
#import <OpenGLES/ES2/glext.h>

@interface RadarViewController ()

@property (strong, nonatomic) IBOutlet GADBannerView *bannerView;
@property (strong, nonatomic) IBOutlet GLKView *radarSurface;
@property (strong, nonatomic) EAGLContext *context;

@end

@implementation RadarViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    
   
    
    
    self.bannerView.adUnitID = @"ca-app-pub-5636726170867832/9790740103";
    self.bannerView.rootViewController = self;
    GADRequest *request = [GADRequest request];
    request.testDevices = @[ GAD_SIMULATOR_ID ];
    [self.bannerView loadRequest:request];
    self.context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];
    if (!self.context) {
        NSLog(@"Failed to create ES context");
    }
    self.radarSurface.context = self.context;
    self.radarSurface.contentScaleFactor = 1.0;
  
    UIBarButtonItem * share = [[UIBarButtonItem alloc] initWithImage:[UIImage imageNamed:@"702-share-toolbar.png"] style:UIBarButtonItemStyleDone target:self action: @selector(sharePressed:)];
    UIBarButtonItem * location = [[UIBarButtonItem alloc] initWithImage:[UIImage imageNamed:@"723-location-arrow-toolbar.png"] style:UIBarButtonItemStyleDone target:self action:nil];
    UIBarButtonItem * layers = [[UIBarButtonItem alloc] initWithImage:[UIImage imageNamed:@"832-stack-1-toolbar.png"] style:UIBarButtonItemStyleDone target:self action:nil];
    UIBarButtonItem * settings = [[UIBarButtonItem alloc] initWithImage:[UIImage imageNamed:@"740-gear-toolbar.png"] style:UIBarButtonItemStyleDone target:self action:nil];
    self.navigationItem.rightBarButtonItems = @[  settings,layers ];
    self.navigationItem.leftBarButtonItems =@[ location,share];
    
    CADisplayLink* displayLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(render:)];
    displayLink.frameInterval = 1;
    [displayLink addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSRunLoopCommonModes];
}

-(void) sharePressed: (id) sender{
    NSMutableArray *items = [NSMutableArray new];

    [items addObject:[self.radarSurface snapshot]];
    NSArray *activityItems = [NSArray arrayWithArray:items];
    UIActivityViewController *activityViewController = [[UIActivityViewController alloc] initWithActivityItems:activityItems applicationActivities:nil];
    
    if (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPhone) {
        [self presentViewController: activityViewController animated:YES completion:nil];
    } else if (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad) {
        UIPopoverController *aPopoverController = [[UIPopoverController alloc] initWithContentViewController:activityViewController];
        [aPopoverController presentPopoverFromBarButtonItem:sender permittedArrowDirections:UIPopoverArrowDirectionAny animated:YES];
    }
}
-(void) layersPressed{
    
}
-(void) settingsPressed{
    
}
-(void) locationPressed{
    
}

- (void)render:(CADisplayLink*)displayLink {
    [self.radarSurface setNeedsDisplay];
}

- (void)glkView:(GLKView *)view drawInRect:(CGRect)rect {
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);
}
@end
