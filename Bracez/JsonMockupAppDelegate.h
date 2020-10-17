//
//  JsonMockupAppDelegate.h
//  JsonMockup
//
//  Created by Eldan on 6/5/10.
//  Copyright 2010 Eldan Ben-Haim. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "JsonDocument.h"
#import "NodeSelectionController.h"
#import "PreferencesDialogController.h"

@interface JsonMockupAppDelegate : NSObject {
   PreferencesDialogController *prefCtler;
}

- (IBAction)showPreferences:(id)aSender;

@end
