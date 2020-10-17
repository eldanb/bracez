//
//  ViewModelEditor.m
//  Bracez
//
//  Created by Eldan Ben Haim on 17/10/2020.
//

#import "ViewModelEditor.h"

@implementation ViewModelEditor

-(instancetype)initWithItemIdentifier:(NSToolbarItemIdentifier)itemIdentifier {
    self = [super initWithItemIdentifier:itemIdentifier];
    
    [self loadFromNib];
    
    return self;
}

-(void)loadFromNib {
    NSNib *nib = [[NSNib alloc] initWithNibNamed:@"ViewModelEditor"
                                           bundle:nil];
    
    NSArray* topLevelObjects = nil;
    [nib instantiateWithOwner:self
              topLevelObjects:&topLevelObjects];
    
    self.view = topLevelObjects[1];
}


@end
