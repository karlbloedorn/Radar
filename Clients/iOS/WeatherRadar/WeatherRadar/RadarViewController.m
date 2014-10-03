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

@implementation RadarViewController{
    float mapScale;
    float centerMapY;
    float centerMapX;
    GLKMatrix4 modelViewProjectionMatrix;
    GLuint lineProgram;
    GLuint radarProgram;
    GLint radarModelViewUniform;
    GLint lineModelViewUniform;
    
}

- (void)viewDidLoad {
    [super viewDidLoad];

    mapScale = 60;
    centerMapY = 54;
    centerMapX = 78;
    
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
    
    [EAGLContext setCurrentContext:self.context];

    
    lineProgram = [self loadShadersWithVertPath:@"lines_vertex" andFragPath: @"lines_fragment" andColorEnabled:NO];
    radarProgram = [self loadShadersWithVertPath:@"radar_vertex" andFragPath: @"radar_fragment" andColorEnabled:YES];
    
    lineModelViewUniform = glGetUniformLocation(lineProgram, "modelViewProjectionMatrix");
   radarModelViewUniform = glGetUniformLocation(radarProgram, "modelViewProjectionMatrix");

    
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

    
    GLKMatrix4 translate = GLKMatrix4MakeTranslation(-centerMapX, -centerMapY, 0);
    GLKMatrix4 scale = GLKMatrix4MakeScale(mapScale, mapScale, 1.0);
    GLKMatrix4 modelviewMatrix = GLKMatrix4Multiply(scale, translate);
    float top = -(self.view.frame.size.height/2.0);
    float bottom = self.view.frame.size.height/2.0;
    float left = -(self.view.frame.size.width/2.0f);
    float right = self.view.frame.size.width/2.0;
    float far = 10.0f;
    float near = -10.0f;
    
    
    GLKMatrix4 projectionMatrix = GLKMatrix4MakeOrtho(left, right, bottom,top,near,far);
    modelViewProjectionMatrix = GLKMatrix4Multiply(projectionMatrix, modelviewMatrix);

    glUseProgram(radarProgram);
    glUniformMatrix4fv(radarModelViewUniform, 1, 0, modelViewProjectionMatrix.m);
    
    // render radar layers here.
    
    glUseProgram(lineProgram);
    glUniformMatrix4fv(lineModelViewUniform, 1, 0, modelViewProjectionMatrix.m);
    
    // render line layers here
}


#pragma mark Shader setup

- (GLuint)loadShadersWithVertPath: (NSString *) vertPath andFragPath: (NSString *) fragPath andColorEnabled: (BOOL) colorEnabled
{
    GLuint vertShader, fragShader;
    NSString *vertShaderPathname, *fragShaderPathname;
    GLuint program = glCreateProgram();
    vertShaderPathname = [[NSBundle mainBundle] pathForResource:vertPath ofType:@"glsl"];
    if (![self compileShader:&vertShader type:GL_VERTEX_SHADER file:vertShaderPathname]) {
        NSLog(@"Failed to compile vertex shader");
        return NO;
    }
    fragShaderPathname = [[NSBundle mainBundle] pathForResource:fragPath ofType:@"glsl"];
    if (![self compileShader:&fragShader type:GL_FRAGMENT_SHADER file:fragShaderPathname]) {
        NSLog(@"Failed to compile fragment shader");
        return NO;
    }
    glAttachShader(program, vertShader);
    glAttachShader(program, fragShader);
    glBindAttribLocation(program, GLKVertexAttribPosition, "position");
    
    if(colorEnabled){
        glBindAttribLocation(program, GLKVertexAttribColor, "color");
    }
    
    if (![self linkProgram:program]) {
        NSLog(@"Failed to link program: %d", program);
        if (vertShader) {
            glDeleteShader(vertShader);
            vertShader = 0;
        }
        if (fragShader) {
            glDeleteShader(fragShader);
            fragShader = 0;
        }
        if (program) {
            glDeleteProgram(program);
            program = 0;
        }
        return NO;
    }
    if (vertShader) {
        glDetachShader(program, vertShader);
        glDeleteShader(vertShader);
    }
    if (fragShader) {
        glDetachShader(program, fragShader);
        glDeleteShader(fragShader);
    }
    return program;
}

- (BOOL)compileShader:(GLuint *)shader type:(GLenum)type file:(NSString *)file
{
    GLint status;
    const GLchar *source;
    source = (GLchar *)[[NSString stringWithContentsOfFile:file encoding:NSUTF8StringEncoding error:nil] UTF8String];
    if (!source) {
        NSLog(@"Failed to load vertex shader");
        return NO;
    }
    *shader = glCreateShader(type);
    glShaderSource(*shader, 1, &source, NULL);
    glCompileShader(*shader);
#if defined(DEBUG)
    GLint logLength;
    glGetShaderiv(*shader, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 0) {
        GLchar *log = (GLchar *)malloc(logLength);
        glGetShaderInfoLog(*shader, logLength, &logLength, log);
        NSLog(@"Shader compile log:\n%s", log);
        free(log);
    }
#endif
    glGetShaderiv(*shader, GL_COMPILE_STATUS, &status);
    if (status == 0) {
        glDeleteShader(*shader);
        return NO;
    }
    return YES;
}

- (BOOL)linkProgram:(GLuint)prog
{
    GLint status;
    glLinkProgram(prog);
#if defined(DEBUG)
    GLint logLength;
    glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 0) {
        GLchar *log = (GLchar *)malloc(logLength);
        glGetProgramInfoLog(prog, logLength, &logLength, log);
        NSLog(@"Program link log:\n%s", log);
        free(log);
    }
#endif
    glGetProgramiv(prog, GL_LINK_STATUS, &status);
    if (status == 0) {
        return NO;
    }
    return YES;
}
#pragma mark Cleanup

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
}
- (void)viewDidUnload
{
    [super viewDidUnload];
    if ([EAGLContext currentContext] == self.context) {
        [EAGLContext setCurrentContext:nil];
    }
    self.context = nil;
}

@end
