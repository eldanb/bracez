//
//  JsonProblem.h
//  JsonMockup
//
//  Created by Eldan on 12/10/10.
//  Copyright 2010 Eldan Ben-Haim. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "JsonDocument.h"


enum JsonMarkerType {
    JsonMarkerTypeError,
    JsonMarkerTypeBookmark
};

@interface JsonMarker : NSObject {

   JsonDocument *doc;
   
   TextCoordinate where;
   NSString *what;
   int whatCode;
   int line;
   JsonMarkerType markerType;
    
    
    NSString *cachedLocationText;
}

-(id)initWithDescription:(NSString*)aDesc
              markerType:(JsonMarkerType)markerType
                    code:(int)aCode
              coordinate:(TextCoordinate)aCoordinate
               parentDoc:(JsonDocument*)aDoc;

-(id)initWithLine:(int)aLine
      description:(NSString*)description
       markerType:(JsonMarkerType)markerType
        parentDoc:(JsonDocument*)aDoc;

+(JsonMarker*)markerForNode:(JsonCocoaNode*)node withParentDoc:(JsonDocument*)parentDoc;

-(NSString*)message;
-(NSString*)locationAsText;
-(int)line;
-(TextCoordinate)coordinate;
-(JsonMarkerType)markerType;

@end
