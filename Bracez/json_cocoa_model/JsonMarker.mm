//
//  JsonProblem.mm
//  JsonMockup
//
//  Created by Eldan on 12/10/10.
//  Copyright 2010 Eldan Ben-Haim. All rights reserved.
//

#import "JsonMarker.h"


@implementation JsonMarker

-(id)initWithDescription:(NSString*)aDesc code:(int)aCode coordinate:(TextCoordinate)aCoordinate parentDoc:(JsonDocument*)aDoc
{
   self = [super init];
   
   if(self)
   {
      what = aDesc;
      whatCode = aCode;
      where = aCoordinate;
      doc = aDoc;
      line = -1;
   }
   
   return self;
}

-(id)initWithLine:(int)aLine parentDoc:(JsonDocument*)aDoc
{
   self = [self initWithDescription:@"" code:0 coordinate:0 parentDoc:aDoc];
   line = aLine;
   
   if(self)
   {
      what = [NSString stringWithFormat:@"Line %d", [self line]];
   }
   
   return self;
}


-(NSString*)message
{
   return what;
}

-(int)line
{
   if(line == -1)
   {
      NSUInteger lRow;
      NSUInteger lCol;
      
      [doc translateCoordinate:where toRow:&lRow col:&lCol];
      
      line = lRow;
   }
   
   return line; 
}

-(TextCoordinate)coordinate
{
   return where;
}

@end
