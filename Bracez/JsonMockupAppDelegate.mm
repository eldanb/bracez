//
//  JsonMockupAppDelegate.m
//  JsonMockup
//
//  Created by Eldan on 6/5/10.
//  Copyright 2010 Eldan Ben-Haim. All rights reserved.
//

#import "JsonMockupAppDelegate.h"
#import "JsonPreferencesDialogController.h"
#import "BracezPreferences.h"

#import "JsonCocoaNode.h"
#include "json_file.h"

@implementation JsonMockupAppDelegate


- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(preferencesChanged:) name:BracezPreferencesChangedNotification object:nil];
    
    [self loadAppearanceFromPreferences];
}

-(void)preferencesChanged:(id)notification {
    [self loadAppearanceFromPreferences];
}

-(void)loadAppearanceFromPreferences {
    AppearanceSelection selAppearance = [BracezPreferences sharedPreferences].selectedAppearance;
    switch(selAppearance) {
        case System:
            [NSApplication sharedApplication].appearance = nil;
            break;
            
        case Dark:
            [NSApplication sharedApplication].appearance = [NSAppearance appearanceNamed:NSAppearanceNameVibrantDark];
            break;
            
        case Light:
            [NSApplication sharedApplication].appearance = [NSAppearance appearanceNamed:NSAppearanceNameVibrantLight];
            break;
    }
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
