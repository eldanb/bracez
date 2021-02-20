//
//  PreferencesWindow.m
//
//  Created by Eldan on 19/6/10.
//  Copyright 2010 __MyCompanyName__. All rights reserved.
//

#import "PreferencesWindow.h"

@implementation PreferencesWindow

-(void)awakeFromNib
{
   [self _displayChosenFont];
   [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(userDefaultsChanged:) name:NSUserDefaultsDidChangeNotification object:nil];
}

- (IBAction)changeFont:(id)aSender
{
   [self _updateDefaultFont:[aSender convertFont:[fontLabel font]]];
}

- (IBAction)setFontClicked:(id)aSender
{
   [[NSFontManager sharedFontManager] orderFrontFontPanel:self];
}

- (void) userDefaultsChanged:(NSNotification*)aNotification
{
   [self _displayChosenFont];
}

- (void)_updateDefaultFont:(NSFont*)aFont
{
    preferences.editorFont = aFont;
}

- (void)_displayChosenFont
{
   NSFont *lFont = preferences.editorFont;
   [fontLabel setFont:[NSFont fontWithDescriptor:[ preferences.editorFont fontDescriptor]
                                            size:11.0]];
   [fontLabel setStringValue:[NSString stringWithFormat:@"%@ %.0f", [lFont displayName], [lFont pointSize]]];
}


@end
