//
//  PreferencesWindow.h
//
//  Created by Eldan on 19/6/10.
//  Copyright 2010 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@interface PreferencesWindow : NSWindow {

   IBOutlet NSTextField *fontLabel;
}

- (IBAction)setFontClicked:(id)aSender;
- (IBAction)changeFont:(id)aSender;

- (void) userDefaultsChanged:(NSNotification*)aNotification;

- (void)_updateDefaultFont:(NSFont*)aFont;
- (void)_displayChosenFont;

@end
