//
//  PreferencesDialogController.h
//  JsonMockup
//
//  Created by Eldan on 13/6/10.
//  Copyright 2010 Eldan Ben-Haim. All rights reserved.
//

#import <Cocoa/Cocoa.h>


@interface PreferencesDialogController : NSObject {
   IBOutlet NSWindow *preferencesDialog;
   NSNib *nibFile;
}

- (id)initWithWindowNibName:(NSString *)aNibName;
- (void)showDialog;
- (void)windowWillClose:(NSNotification *)notification;
- (void)loadFromNib;

@end
