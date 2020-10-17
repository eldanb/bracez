//
//  NodeTypeToColorTransformer.m
//  JsonMockup
//
//  Created by Eldan on 9/1/10.
//  Copyright 2010 Eldan Ben-Haim. All rights reserved.
//

#import "NodeTypeToColorTransformer.h"

#include "json_file.h"

using namespace json;

@implementation NodeTypeToColorTransformer

+ (Class)transformedValueClass { return [NSColor class]; }

+ (BOOL)allowsReverseTransformation { return NO; }

-(void)dealloc {
    NSLog(@"DEALLOC");
}

-(id)init
{
   return [self initWithEnableKey:nil];
}

-(id)initWithEnableKey:(NSString*)aEnableKey
{
   self = [super init];
   
   if(self)
   {
      enableKey = aEnableKey;

      [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(defaultsChanged:) 
                                                   name:NSUserDefaultsDidChangeNotification object:nil];

      [self _loadColors];
   }
   
   return self;
}

-(void)defaultsChanged:(NSNotification*)aNotify
{
   [self _loadColors];
}
                        
-(void) _loadColors;
{
   NSUserDefaults *lDefaults = [NSUserDefaults standardUserDefaults];

   defaultColor = [NSUnarchiver unarchiveObjectWithData:[lDefaults dataForKey:@"TextEditorColorDefault"]];

   if(!enableKey || [lDefaults boolForKey:enableKey])
   {
      stringColor = [NSUnarchiver unarchiveObjectWithData:[lDefaults dataForKey:@"TextEditorColorString"]];
      keyColor = [NSUnarchiver unarchiveObjectWithData:[lDefaults dataForKey:@"TextEditorColorKey"]];
      keywordColor = [NSUnarchiver unarchiveObjectWithData:[lDefaults dataForKey:@"TextEditorColorKeyword"]];
      numberColor = [NSUnarchiver unarchiveObjectWithData:[lDefaults dataForKey:@"TextEditorColorNumber"]];
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
