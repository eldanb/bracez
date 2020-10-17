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
   NSString *enableKey;
   
   NSColor *defaultColor;
   NSColor *stringColor;
   NSColor *keyColor;
   NSColor *keywordColor;
   NSColor *numberColor;
   
}


-(id)initWithEnableKey:(NSString*)aEnableKey;

-(NSColor*)colorForNodeType:(int)aNodeType;
-(NSColor*)keyColor;

-(void) _loadColors;


@end

