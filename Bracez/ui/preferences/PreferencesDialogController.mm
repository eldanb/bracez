//
//  PreferencesDialogController.mm
//  JsonMockup
//
//  Created by Eldan on 13/6/10.
//  Copyright 2010 Eldan Ben-Haim. All rights reserved.
//

#import "PreferencesDialogController.h"


@implementation PreferencesDialogController


- (id)initWithWindowNibName:(NSString *)aNibName
{
   self = [super init];
   
   if(self)
   {
      nibFile = [[NSNib alloc] initWithNibNamed:aNibName bundle:nil];
   }
   
   return self;
}

- (void)loadFromNib
{
   [nibFile instantiateWithOwner:self topLevelObjects:nil];
}

- (void)showDialog
{
   if(!preferencesDialog)
   {
      [self loadFromNib];
   }
   
   [preferencesDialog makeKeyAndOrderFront:self];
}

- (void)windowWillClose:(NSNotification *)notification
{
   preferencesDialog = nil;
}

@end
