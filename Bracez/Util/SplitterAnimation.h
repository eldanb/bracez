//
//  SplitterAnimation.h
//  JsonMockup
//
//  Created by Eldan on 8/21/10.
//  Copyright 2010 Eldan Ben-Haim. All rights reserved.
//

#import <Cocoa/Cocoa.h>


@interface SplitterAnimation : NSAnimation {

   NSSplitView  *animationTarget;
   int splitterIndex;

   double startVal;
   double endVal;
}

-(id)initWithTarget:(NSSplitView*)aTarget divIndex:(int)aIdx startValue:(double)aStart endValue:(double)aEnd;
-(id)initWithTarget:(NSSplitView*)aTarget divIndex:(int)aIdx endValue:(double)aEnd;


-(void)setCurrentProgress:(NSAnimationProgress)progress;


@end
