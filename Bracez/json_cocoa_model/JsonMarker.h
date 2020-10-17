//
//  JsonProblem.h
//  JsonMockup
//
//  Created by Eldan on 12/10/10.
//  Copyright 2010 Eldan Ben-Haim. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "JsonDocument.h"

@interface JsonMarker : NSObject {

   JsonDocument *doc;
   
   TextCoordinate where;
   NSString *what;
   int whatCode;
   int line;
}

-(id)initWithDescription:(NSString*)aDesc code:(int)aCode coordinate:(TextCoordinate)aCoordinate parentDoc:(JsonDocument*)aDoc;
-(id)initWithLine:(int)aLine parentDoc:(JsonDocument*)aDoc;

-(NSString*)message;
-(int)line;
-(TextCoordinate)coordinate;

@end
