//
//  KvoAnimation.h
//  JsonMockup
//
//  Created by Eldan on 8/21/10.
//  Copyright 2010 Eldan Ben-Haim. All rights reserved.
//

#import <Cocoa/Cocoa.h>


@protocol KvoAnimatable

-(void)setValue:(NSObject*)value;

@end

@interface KvoAnimation : NSAnimation {

   NSDictionary *animatedPropsStart;
   NSDictionary *animatedPropsEnd;
   NSObject<KvoAnimatable>     *animationTarget;
   
}

-(id)initWithTarget:(NSObject<KvoAnimatable>*)aTarget startValues:(NSDictionary*)aStart endValues:(NSDictionary*)aEnd;
-(id)initWithTarget:(NSObject<KvoAnimatable>*)aTarget endValues:(NSDictionary*)aEnd;

-(void)setCurrentProgress:(NSAnimationProgress)progress;


@end
