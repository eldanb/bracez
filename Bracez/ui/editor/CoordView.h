//
//  CoordView.h
//  JsonMockup
//
//  Created by Eldan on 21/5/10.
//  Copyright 2010 Eldan Ben-Haim. All rights reserved.
//

#import <Cocoa/Cocoa.h>


@interface CoordView : NSView {

   NSButtonCell *_btnCell;
}

- (void)dealloc;
- (void)drawRect:(NSRect)dirtyRect;

-(void)setCoordinateRow:(NSUInteger)aRow col:(NSUInteger)aCol;
@end
