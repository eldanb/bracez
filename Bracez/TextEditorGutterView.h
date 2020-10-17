//
//  TextEditorGutterView.h
//  JsonMockup
//
//  Created by Eldan on 18/6/10.
//  Copyright 2010 Eldan Ben-Haim. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@protocol GutterViewModel
- (void)connectGutterView:(id)aGutterView;
- (int)lineNumberForCharacterIndex:(int)aIdx;
- (UInt32)markersForLine:(int)aLine;
@end

@interface TextEditorGutterView : NSRulerView {

   NSTextView *textView;   
   NSMutableDictionary *marginAttributes;
   id <GutterViewModel> model;
   BOOL shouldShowLineNumbers;
   int shownFlagsMask;
}

- (void)setModel:(id<GutterViewModel>)aModel;
- (id)initWithScrollView:(NSScrollView *)aScrollView;
- (void)drawHashMarksAndLabelsInRect:(NSRect)aRect;

-(void)setShowLineNumbers:(BOOL)aNumbers;
-(void)setShownFlagsMask:(int)aMask;

-(void)markersChanged;

-(void)_drawOneNumberInMargin:(unsigned) aNumber inRect:(NSRect)r;
-(void)_drawFlags:(UInt32)aFlags inRect:(NSRect)aRect;
-(void)_drawMarkerWithColor:(NSColor*)aBaseColor inRect:(NSRect)aRect;

@end


