//
//  NSIndexPathAdditions.m
//  JsonMockup
//
//  Created by Eldan on 8/18/10.
//  Copyright 2010 Eldan Ben-Haim. All rights reserved.
//

#import "NSIndexPathAdditions.h"


@implementation NSIndexPath (NavigationAdditions)

-(NSIndexPath*)indexPathOfNextSibling
{
   int lNextSiblingIdx = [self indexAtPosition:[self length]-1]+1;
   NSIndexPath *lNewPath = [[self indexPathByRemovingLastIndex] indexPathByAddingIndex:lNextSiblingIdx];
   return lNewPath;
}


-(NSIndexPath*)indexPathOfPrevSibling
{
   int lPrevSiblingIdx = [self indexAtPosition:[self length]-1]-1;
   
   if(lPrevSiblingIdx<0)
   {
      return nil;
   } else {
      NSIndexPath *lNewPath = [[self indexPathByRemovingLastIndex] indexPathByAddingIndex:lPrevSiblingIdx];
      return lNewPath;
   }
   
}

@end
