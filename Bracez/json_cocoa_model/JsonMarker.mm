//
//  JsonProblem.mm
//  JsonMockup
//
//  Created by Eldan on 12/10/10.
//  Copyright 2010 Eldan Ben-Haim. All rights reserved.
//

#import "JsonMarker.h"


@implementation JsonMarker

-(id)initWithDescription:(NSString*)aDesc
              markerType:(JsonMarkerType)markerType
                    code:(int)aCode
              coordinate:(TextCoordinate)aCoordinate
               parentDoc:(JsonDocument*)aDoc
{
   self = [super init];
   
   if(self)
   {
      what = aDesc;
      whatCode = aCode;
      where = aCoordinate;
      doc = aDoc;
      line = -1;
      self->markerType = markerType;
   }
   
   return self;
}

-(NSString*)locationAsText {
    if(!cachedLocationText) {
        if(line != -1) {
            cachedLocationText = [NSString stringWithFormat:@"%@ @ %d", doc.displayName, line];
        } else {
            NSUInteger row, col;
            [doc translateCoordinate:where toRow:&row col:&col];
            cachedLocationText = [NSString stringWithFormat:@"%@ @ %lu:%lu", doc.displayName, row, col];
        }
    }
    return cachedLocationText;
}
    
-(id)initWithLine:(int)aLine
      description:(NSString*)description
       markerType:(JsonMarkerType)markerType
        parentDoc:(JsonDocument*)aDoc
{
   self = [self initWithDescription:description
                         markerType:markerType
                               code:0
                         coordinate:aDoc.bookmarks.getLineStart(aLine-1)
                          parentDoc:aDoc];
   
   if(self)
   {
       line = aLine;
   }
   
   return self;
}

-(JsonMarkerType)markerType {
    return self->markerType;
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
