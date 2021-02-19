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
   [[NSUserDefaults standardUserDefaults] setValue:[NSKeyedArchiver archivedDataWithRootObject:aFont
                                                                         requiringSecureCoding:NO
                                                                                         error:nil]
                                            forKey:@"TextEditorFont"];
}

- (void)_displayChosenFont
{
   NSData *lFontData = [[NSUserDefaults standardUserDefaults] valueForKey:@"TextEditorFont"];
   NSFont *lFont = [NSKeyedUnarchiver unarchivedObjectOfClass:NSFont.class
                                                     fromData:lFontData
                                                        error:nil];    
   [fontLabel setFont:[NSFont fontWithDescriptor:[lFont fontDescriptor] size:11.0]];
   [fontLabel setStringValue:[NSString stringWithFormat:@"%@ %.0f", [lFont displayName], [lFont pointSize]]];
}


@end
