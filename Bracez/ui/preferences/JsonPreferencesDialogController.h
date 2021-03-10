//
//  JsonPreferencesDialogController.h
//  JsonMockup
//
//  Created by Eldan on 13/6/10.
//  Copyright 2010 Eldan Ben-Haim. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "PreferencesDialogController.h"
#import "AnimatingTabView.h"

@interface JsonPreferencesDialogController : PreferencesDialogController {
   IBOutlet AnimatingTabView *tabView;
   IBOutlet NSToolbar *toolbar;

}

-(IBAction)changePage:(id)aSender;

@end
