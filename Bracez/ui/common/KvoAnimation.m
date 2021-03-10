//
//  KvoAnimation.m
//  JsonMockup
//
//  Created by Eldan on 8/21/10.
//  Copyright 2010 Eldan Ben-Haim. All rights reserved.
//


#import "KvoAnimation.h"

@implementation KvoAnimation



-(id)initWithTarget:(NSObject<KvoAnimatable>*)aTarget startValues:(NSDictionary*)aStart endValues:(NSDictionary*)aEnd
{
   self = [super init];
   
   if(self)
   {
      animatedPropsStart = aStart;
      animatedPropsEnd = aEnd;
      animationTarget = aTarget;
   }
   
   return self;
}

-(id)initWithTarget:(NSObject<KvoAnimatable>*)aTarget endValues:(NSDictionary*)aEnd
{
   NSMutableDictionary *lStart = [NSMutableDictionary dictionaryWithCapacity:[aEnd count]];
   
   for(NSString *lKey in [aEnd keyEnumerator])
   {
      [lStart setValue:[aTarget valueForKey:lKey] forKey:lKey];
   }
   
   return [self initWithTarget:aTarget startValues:lStart endValues:aEnd];
}


-(void)setCurrentProgress:(NSAnimationProgress)progress
{
   for(NSString *lPropName in [animatedPropsStart keyEnumerator])
   {
      double lStart = [[animatedPropsStart valueForKey:lPropName] doubleValue];
      double lEnd = [[animatedPropsEnd valueForKey:lPropName] doubleValue];
      
      double lCur = lStart+(lEnd-lStart)*progress;
      [animationTarget setValue:[NSNumber numberWithDouble:lCur]];
   }      
}
@end

