//
//  JsonPreferencesDialogController.mm
//  JsonMockup
//
//  Created by Eldan on 13/6/10.
//  Copyright 2010 Eldan Ben-Haim. All rights reserved.
//

#import "JsonPreferencesDialogController.h"


@implementation JsonPreferencesDialogController


- (void)loadFromNib
{
   [super loadFromNib];
   [toolbar setSelectedItemIdentifier:@"general"];

   [tabView setTransitionStyle:AnimatingTabViewDissolveTransitionStyle];
   
   [self changePage:self];
}


-(IBAction)changePage:(id)aSender
{
   [tabView selectTabViewItemWithIdentifier:[toolbar selectedItemIdentifier]];
}

@end
