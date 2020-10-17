//
//  JsonMockupAppDelegate.m
//  JsonMockup
//
//  Created by Eldan on 6/5/10.
//  Copyright 2010 Eldan Ben-Haim. All rights reserved.
//

#import "JsonMockupAppDelegate.h"
#import "JsonPreferencesDialogController.h"

#import "JsonCocoaNode.h"
#include "json_file.h"

@implementation JsonMockupAppDelegate


- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
}

- (IBAction)showPreferences:(id)aSender
{
   if(!prefCtler)
   {
      prefCtler = [[JsonPreferencesDialogController alloc] initWithWindowNibName:@"PreferencesWindow"];
   }
   
   [prefCtler showDialog];
}

@end
