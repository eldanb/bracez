//
//  NodeTypeToColorTransformer.h
//  JsonMockup
//
//  Created by Eldan on 9/1/10.
//  Copyright 2010 Eldan Ben-Haim. All rights reserved.
//

#import <Cocoa/Cocoa.h>


@interface NodeTypeToColorTransformer: NSValueTransformer 
{
}


-(BOOL)enabled;
-(NSColor*)colorForNodeType:(int)aNodeType;
-(NSColor*)keyColor;

-(void) _loadColors;

+(NodeTypeToColorTransformer*)sharedInstance;

@end

