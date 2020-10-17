//
//  NSIndexPathAdditions.h
//  JsonMockup
//
//  Created by Eldan on 8/18/10.
//  Copyright 2010 Eldan Ben-Haim. All rights reserved.
//

#import <Cocoa/Cocoa.h>


@interface NSIndexPath (NavigationAdditions)

-(NSIndexPath*)indexPathOfNextSibling;
-(NSIndexPath*)indexPathOfPrevSibling;

@end
