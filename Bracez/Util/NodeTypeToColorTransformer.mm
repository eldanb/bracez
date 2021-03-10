//
//  NodeTypeToColorTransformer.m
//  JsonMockup
//
//  Created by Eldan on 9/1/10.
//  Copyright 2010 Eldan Ben-Haim. All rights reserved.
//

#import "NodeTypeToColorTransformer.h"
#import "BracezPreferences.h"

#include "json_file.h"

using namespace json;

@interface NodeTypeToColorTransformer () {
    NSColor *defaultColor;
    NSColor *stringColor;
    NSColor *keyColor;
    NSColor *keywordColor;
    NSColor *numberColor;
}

@end

@implementation NodeTypeToColorTransformer

+ (Class)transformedValueClass { return [NSColor class]; }

+ (BOOL)allowsReverseTransformation { return NO; }

-(void)dealloc {
    NSLog(@"DEALLOC");
}

-(id)init
{
    self = [super init];
    if(self)
    {
       [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(defaultsChanged:)
                                                    name:NSUserDefaultsDidChangeNotification object:nil];

       [self _loadColors];
    }
    
    return self;
}


-(BOOL)enabled {
    return [BracezPreferences sharedPreferences].enableTreeViewSyntaxColoring;
}

-(void)defaultsChanged:(NSNotification*)aNotify
{
   [self _loadColors];
}
                        
-(void) _loadColors;
{
    BracezPreferences *lPrefs = [BracezPreferences sharedPreferences];
    
    defaultColor = lPrefs.editorColorDefault;
    
    if(self.enabled) {
        stringColor = lPrefs.editorColorString;
        keyColor = lPrefs.editorColorKey;
        keywordColor = lPrefs.editorColorKeyword;
        numberColor = lPrefs.editorColorNumber;
    } else {
        stringColor = defaultColor;
        keyColor = defaultColor;
        keywordColor = defaultColor;
        numberColor = defaultColor;
    }
}

- (id)transformedValue:(id)value {
   NSNumber *lInput = (NSNumber *)value;
   return [self colorForNodeType:[lInput intValue]];
}

-(NSColor*)keyColor
{
   return keyColor;
}

-(NSColor*)colorForNodeType:(int)aNodeType
{
    
   // return nil;
   switch(aNodeType)
   {
      case ntNull:
      case ntBoolean:
         return keywordColor;
         
      case ntNumber:
         return numberColor;
                 
      case ntString:
         return stringColor;
         
      case ntObject:
      case ntArray:        
      default:
         return defaultColor;
   }   
}


@end
