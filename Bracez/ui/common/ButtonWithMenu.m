//
//  ButtonWithMenu.m
//  Bracez
//
//  Created by Eldan Ben Haim on 28/08/2021.
//

#import "ButtonWithMenu.h"

@interface ButtonWithMenu () {
    NSSegmentedControl *_segmentedControl;
    NSString *_label;
}

@end


@implementation ButtonWithMenu

-(instancetype)initWithCoder:(NSCoder *)coder {
    self = [super initWithCoder:coder];
    [self realizeControl];
    return self;
}

-(instancetype)initWithFrame:(NSRect)frameRect {
    self = [super initWithFrame:frameRect];
    [self realizeControl];
    return self;
}

-(void)realizeControl {
    _segmentedControl = [[NSSegmentedControl alloc] initWithFrame:self.bounds];
    _segmentedControl.autoresizingMask = NSViewHeightSizable | NSViewWidthSizable;
    [_segmentedControl setAction:@selector(segmentClicked:)];
    [_segmentedControl setTarget:self];
    
    _segmentedControl.segmentCount = 2;
    [_segmentedControl setLabel:@"BLA" forSegment:0];
    _segmentedControl.segmentStyle = NSSegmentStyleRounded;
    _segmentedControl.trackingMode = NSSegmentSwitchTrackingMomentary;
    
    [_segmentedControl setShowsMenuIndicator:YES forSegment:1];
    _segmentedControl.controlSize = NSControlSizeSmall;
    _segmentedControl.font = [NSFont systemFontOfSize:[NSFont smallSystemFontSize]];
    
    [self addSubview:_segmentedControl];
}

-(void)segmentClicked:(id)sender {
    switch(_segmentedControl.selectedSegment) {
        case 0:
            [self sendAction:self.action to:self.target];
            break;
            
        case 1:
            [self.menu popUpMenuPositioningItem:self.menu.itemArray.firstObject
                                     atLocation:self.bounds.origin
                                         inView:self];
            break;
            
        default:
            break;
    }
}

-(void)setLabel:(NSString*)label {
    _label = label;
    [_segmentedControl setLabel:label forSegment:0];
}

-(NSString*)label {
    return _label;
}

-(NSSize)intrinsicContentSize {
    NSSize ret = _segmentedControl.intrinsicContentSize;
    ret.width += 6;
    return ret;
}
@end
