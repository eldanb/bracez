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
   NSUInteger lNextSiblingIdx = [self indexAtPosition:[self length]-1]+1;
   NSIndexPath *lNewPath = [[self indexPathByRemovingLastIndex] indexPathByAddingIndex:lNextSiblingIdx];
   return lNewPath;
}


-(NSIndexPath*)indexPathOfPrevSibling
{
   NSUInteger selfIndex = [self indexAtPosition:[self length]-1];
   
   if(selfIndex == 0)
   {
      return nil;
   } else {
      NSIndexPath *lNewPath = [[self indexPathByRemovingLastIndex] indexPathByAddingIndex:selfIndex-1];
      return lNewPath;
   }
   
}

@end
