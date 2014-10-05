//
//  LayerSelectorViewController.m
//  WeatherRadar
//
//  Created by Karl Bloedorn on 10/4/14.
//  Copyright (c) 2014 Karl Bloedorn. All rights reserved.
//

#import "LayerSelectorViewController.h"
#import "LineLayer.h"

@interface LayerSelectorViewController ()

@end

@implementation LayerSelectorViewController

- (void)viewDidLoad {
    [super viewDidLoad];
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
}

-(UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath{
    LineLayer * cur = [self.lineLayers objectAtIndex:indexPath.row];

    UITableViewCell * cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:@"layerSelectorCell"];
    cell.textLabel.text = cur.label;
    if(cur.isVisible){
        cell.accessoryType = UITableViewCellAccessoryCheckmark;
    } else {
        cell.accessoryType = UITableViewCellAccessoryNone;
    }
    return cell;
}
-(NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section{
    return self.lineLayers.count;
}
- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath{
    LineLayer * cur = [self.lineLayers objectAtIndex:indexPath.row];
    cur.isVisible = !cur.isVisible;
    [tableView deselectRowAtIndexPath:indexPath animated:YES];
    [tableView reloadRowsAtIndexPaths:@[indexPath] withRowAnimation:UITableViewRowAnimationNone];
}



@end
