//
//  PathView.h
//  JsonMockup
//
//  Created by Eldan on 6/5/10.
//  Copyright 2010 Eldan Ben-Haim. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@class PathView;

@protocol PathViewComponentCellNotifications
- (void)pathView:(PathView *)aPathView didClickOnPathComponentIndex:(int)aIdx;
- (void)pathView:(PathView *)aPathView preparePopupMenu:(NSMenu**)aMenu forIndex:(int)aIdx;

- (void)pathView:(PathView *)aPathView extendByString:(NSString*)string;
- (void)pathView:(PathView *)aPathView previewPathExtensionString:(NSString*)string;
- (NSArray<NSString*>*)pathView:(PathView *)aPathView proposeCompletionsForString:(NSString*)string;
- (NSString*)pathView:(PathView *)aPathView updateExtensionValue:(NSString*)curExtension forSelectedCompletion:(NSString*)completion;
- (void)pathViewRequestMoveToParent:(PathView*)aPathView;
@end

@interface PathViewComponentCell : NSTextFieldCell {
}

-(void)drawWithFrame:(NSRect)cellFrame inView:(NSView *)controlView;
- (NSSize)cellSize;

@end

@interface PathView : NSView {
   NSButtonCell *_bkCell;
   PathViewComponentCell *_ellipsisCell;
   NSArray *_pathCells;   

   NSCell *_currentHilightedCell;
   id _delegate;
}

@property id<PathViewComponentCellNotifications> delegate;
@property (readonly) BOOL pathExtensionActive;
@property NSArray *shownPath;

- (void)drawRect:(NSRect)dirtyRect;

-(PathViewComponentCell*)cellForPathComponent:(NSObject*)aComponent;
-(NSString*)pathStringAtIndex:(int)aIdx;

-(void)iterateWithShownCellsAndRects:(void (^)(NSCell *, NSRect))block;

-(PathViewComponentCell*)hitTestPathElement:(NSPoint)aPoint andGetFrame:(NSRect*)aFrame;

-(void)startPathExtensionEntryWithDefault:(NSString*)defaultPath;
-(void)abortPathExtensionEntry;

-(void)_secondaryClick:(NSEvent*)theEvent;

@end

