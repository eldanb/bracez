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


-(id)initWithLine:(int)aLine
  description:(NSString*)description
   markerType:(JsonMarkerType)markerType
    parentDoc:(JsonDocument*)aDoc
{
    self = [self initWithDescription:description
                         markerType:markerType
                               code:0
                         coordinate:aDoc.jsonFile->getLineFirstCharacter(aLine)
                          parentDoc:aDoc];

    if(self)
    {
       line = aLine;
    }

    return self;
}

+(JsonMarker*)markerForNode:(JsonCocoaNode*)node withParentDoc:(JsonDocument*)parentDoc {
    return [[JsonMarker alloc] initWithDescription:node.longNodeValue
                                        markerType:JsonMarkerTypeNode
                                              code:node.nodeType
                                        coordinate:node.textRange.start
                                         parentDoc:parentDoc];
}

-(NSString*)locationAsText {
    if(!cachedLocationText) {
        if(line != -1) {
            cachedLocationText = [NSString stringWithFormat:@"Line %d in %@", line, doc.displayName];
        } else {
            int row, col;
            [doc translateCoordinate:where toRow:&row col:&col];
            cachedLocationText = [NSString stringWithFormat:@"Pos %d:%d in %@", row, col, doc.displayName];
        }
    }
    return cachedLocationText;
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
      int lRow;
      int lCol;
      
      [doc translateCoordinate:where toRow:&lRow col:&lCol];
      
      line = lRow;
   }
   
   return line; 
}

-(TextCoordinate)coordinate
{
   return where;
}

-(int)code {
    return whatCode;
}
@end
