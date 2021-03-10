//
//  SplitterAnimation.m
//  JsonMockup
//
//  Created by Eldan on 8/21/10.
//  Copyright 2010 Eldan Ben-Haim. All rights reserved.
//

#import "SplitterAnimation.h"


@implementation SplitterAnimation



-(id)initWithTarget:(NSSplitView*)aTarget divIndex:(int)aIdx startValue:(double)aStart endValue:(double)aEnd
{
   self = [super init];
   
   if(self)
   {
      animationTarget = aTarget;
      splitterIndex = aIdx;
      startVal = aStart;
      endVal = aEnd;
   }
   
   return self;
}

-(id)initWithTarget:(NSSplitView*)aTarget divIndex:(int)aIdx endValue:(double)aEnd
{
   NSRect lInterestingFrame = [[[aTarget subviews] objectAtIndex:aIdx] frame];
   return [self initWithTarget:aTarget divIndex:aIdx startValue:[aTarget isVertical]?lInterestingFrame.size.width:lInterestingFrame.size.height endValue:aEnd];
}

-(void)setCurrentProgress:(NSAnimationProgress)progress
{
   [animationTarget setPosition:(startVal+(endVal-startVal)*progress) ofDividerAtIndex:splitterIndex];
   [animationTarget display];
}

@end
