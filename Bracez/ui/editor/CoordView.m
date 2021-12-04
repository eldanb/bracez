//
//  CoordView.m
//  JsonMockup
//
//  Created by Eldan on 21/5/10.
//  Copyright 2010 Eldan Ben-Haim. All rights reserved.
//

#import "CoordView.h"


@implementation CoordView

- (id)initWithFrame:(NSRect)frame {
    self = [super initWithFrame:frame];
    if (self) {
      _btnCell = [[NSButtonCell alloc] init];
      [_btnCell setBezelStyle:NSBezelStyleShadowlessSquare];
      [_btnCell setBordered:NO];
      [_btnCell setAlignment:NSTextAlignmentCenter];
      [_btnCell setFont:[NSFont userFontOfSize:11.0]];
      [_btnCell setWraps:NO];
    }
    return self;
}

- (void)drawRect:(NSRect)dirtyRect {
   [_btnCell drawWithFrame:[self bounds] inView:self];
}

-(void)setCoordinateRow:(NSUInteger)aRow col:(NSUInteger)aCol;
{
   [_btnCell setTitle:[NSString stringWithFormat:@"%lu : %lu",
                       (unsigned long)aRow,
                       (unsigned long)aCol]];
   [self setNeedsDisplay:YES];
}


@end
