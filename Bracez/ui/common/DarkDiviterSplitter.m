//
//  DarkDiviterSplitter.m
//  JsonMockup
//
//  Created by Eldan on 2/9/11.
//  Copyright 2011 Eldan Ben-Haim. All rights reserved.
//

#import "DarkDiviterSplitter.h"


@implementation DarkDiviterSplitter

- (CGFloat)dividerThickness
{
   return 7;
}

- (void)drawDividerInRect:(NSRect)aRect
{
   [super drawDividerInRect:aRect];

   [[NSColor darkGrayColor] set];

   NSBezierPath *lPath = [NSBezierPath bezierPath];
   if([self isVertical])
   {
      [lPath moveToPoint:NSMakePoint(aRect.origin.x, aRect.origin.y)];
      [lPath lineToPoint:NSMakePoint(aRect.origin.x, aRect.origin.y+aRect.size.height)];

      [lPath moveToPoint:NSMakePoint(aRect.origin.x+aRect.size.width, aRect.origin.y)];
      [lPath lineToPoint:NSMakePoint(aRect.origin.x+aRect.size.width, aRect.origin.y+aRect.size.height)];

   } else {
      [lPath moveToPoint:NSMakePoint(aRect.origin.x, aRect.origin.y)];
      [lPath lineToPoint:NSMakePoint(aRect.origin.x+aRect.size.width, aRect.origin.y)];
      
      [lPath moveToPoint:NSMakePoint(aRect.origin.x, aRect.origin.y+aRect.size.height)];
      [lPath lineToPoint:NSMakePoint(aRect.origin.x+aRect.size.width, aRect.origin.y+aRect.size.height)];
   }

   [lPath stroke];
}
@end
