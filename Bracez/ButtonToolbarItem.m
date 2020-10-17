//
//  ButtonToolbarItem.m
//  Bracez
//
//  Created by Eldan Ben Haim on 17/10/2020.
//

#import "ButtonToolbarItem.h"

@implementation ButtonToolbarItem

-(void)validate {
    NSControl *control = (NSControl*)self.view;
    SEL action = self.action;
    
    NSObject *target = [NSApp targetForAction:action to:self.target from:self];
    BOOL validationResult;
    if([target conformsToProtocol:@protocol(NSUserInterfaceValidations)]) {
        validationResult = [(NSObject<NSUserInterfaceValidations>*)target validateUserInterfaceItem:self];
    } else {
        validationResult = [target validateToolbarItem:self];
    }
    
    control.enabled = validationResult;
}

@end
