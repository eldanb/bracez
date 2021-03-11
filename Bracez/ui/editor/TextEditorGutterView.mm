//
//  TextEditorGutterView.mm
//  JsonMockup
//
//  Created by Eldan on 18/6/10.
//  Copyright 2010 Eldan Ben-Haim. All rights reserved.
//

#import "TextEditorGutterView.h"
#import <CoreText/CoreText.h>

@implementation TextEditorGutterView


- (id)initWithScrollView:(NSScrollView *)aScrollView
{
   self = [super initWithScrollView:aScrollView orientation:NSVerticalRuler];
   
   if(self)
   {
      textView = [aScrollView documentView];

      marginAttributes = [[NSMutableDictionary alloc] init];

      [marginAttributes setObject:[NSFont boldSystemFontOfSize:8] forKey: NSFontAttributeName];
      [marginAttributes setObject:[NSColor lightGrayColor] forKey: NSForegroundColorAttributeName];

       marginAttributesWithMarkers = [marginAttributes mutableCopy];
       [marginAttributesWithMarkers setObject:[NSColor whiteColor] forKey: NSForegroundColorAttributeName];
       
      shouldShowLineNumbers = NO;
      shownFlagsMask = 0xffffffff;
       
       [self setAutoresizingMask:0];
      
      [self setRuleThickness:35];
   }
   
   return self;
}

- (void)setShowLineNumbers:(BOOL)aNumbers
{
   shouldShowLineNumbers = aNumbers;
   [self setNeedsDisplay:YES];
}

- (void)setModel:(id<GutterViewModel>)aModel
{
   [model connectGutterView:nil];
   model = aModel;
   [model connectGutterView:self];
}

-(void)setShownFlagsMask:(int)aMask
{
   shownFlagsMask = aMask;
   [self setNeedsDisplay:YES];
}

-(void)markersChanged
{
   [self setNeedsDisplay:YES];
}


- (void)getFirstShownLineNumber:(int*)aNumber andRect:(NSRect*)aRect
{
	NSUInteger  index, lineNumber;
	NSRange		lineRange;
	NSRect		lineRect;
   
	NSLayoutManager* layoutManager = [textView layoutManager];
	NSTextContainer* textContainer = [textView textContainer];
      
	// Only get the visible part of the scroller view
	NSRect documentVisibleRect = [[self scrollView] documentVisibleRect];
   
	// Find the glyph range for the visible glyphs
	NSRange glyphRange = [layoutManager glyphRangeForBoundingRect: documentVisibleRect inTextContainer: textContainer];
   
   if(model)
   {
      NSUInteger lCharIdx = [layoutManager characterIndexForGlyphAtIndex:glyphRange.location];
      *aNumber = [model lineNumberForCharacterIndex:lCharIdx];
      *aRect = [layoutManager lineFragmentRectForGlyphAtIndex:glyphRange.location effectiveRange:nil];
   } else
   {
      // Calculate the start and end indexes for the glyphs	
      unsigned long start_index = glyphRange.location;
      
      index = 0;
      lineNumber = 0;
      
      // Skip all lines that are visible at the top of the text view (if any)
      do
      {
         lineRect = [layoutManager lineFragmentRectForGlyphAtIndex:index effectiveRange:&lineRange];
         index = NSMaxRange( lineRange );
         ++lineNumber;
      } while (index < start_index);

      *aNumber = (int)lineNumber;
      *aRect = lineRect;
   }
}

- (void)drawHashMarksAndLabelsInRect:(NSRect)aRect
{
   // Draw background and border
   aRect.size.height+=10;
   [[NSColor whiteColor] set];
   [NSBezierPath fillRect: aRect]; 

   /* [[NSColor controlShadowColor] set];
   
   NSBezierPath *lLine = [NSBezierPath bezierPath];
   [lLine moveToPoint:NSMakePoint(aRect.origin.x+aRect.size.width, aRect.origin.y)];
   [lLine lineToPoint:NSMakePoint(aRect.origin.x+aRect.size.width, aRect.origin.y+aRect.size.height)];
   [lLine stroke]; */

/*   [[NSColor contr] set];
   [lLine moveToPoint:NSMakePoint(aRect.origin.x, aRect.origin.y)];
   [lLine lineToPoint:NSMakePoint(aRect.origin.x, aRect.origin.y+aRect.size.height)];
   [lLine stroke];*/
   
   NSLayoutManager* layoutManager = [textView layoutManager];
   CGFloat lDefaultLineHeight = [layoutManager defaultLineHeightForFont:textView.font];
   if(!lDefaultLineHeight)
   {
      return;
   }
    
    CGFloat lineRectWidth = [self requiredThickness];
      
   int lCurLineNo;
   NSRect lCurLineRect;
   [self getFirstShownLineNumber:&lCurLineNo andRect:&lCurLineRect];
   
   // Only get the visible part of the scroller view
	NSRect documentVisibleRect = [[self scrollView] documentVisibleRect];

   lCurLineRect.origin.y -= documentVisibleRect.origin.y;
   while(lCurLineRect.origin.y <= aRect.origin.y+aRect.size.height)
   {
       NSUInteger charIndex = [model characterIndexForFirstCharOfLine:lCurLineNo];
       NSUInteger glyphIndex = [layoutManager glyphIndexForCharacterAtIndex:charIndex];
       if(glyphIndex<layoutManager.numberOfGlyphs) {
           lCurLineRect = [layoutManager lineFragmentRectForGlyphAtIndex:glyphIndex effectiveRange:nil];
       } else {
           lCurLineRect.origin.y = (lCurLineNo-1) * lDefaultLineHeight;
           lCurLineRect.size.height = lDefaultLineHeight;
       }

       lCurLineRect.origin.y -= documentVisibleRect.origin.y;
       
       lCurLineRect.origin.x = 0;
       lCurLineRect.size.width  = lineRectWidth;

      UInt32 lCurLineFlags=0;
      
      if(shownFlagsMask && model)
      {
         lCurLineFlags = [model markersForLine:lCurLineNo];
      }
          
      if(lCurLineFlags)
      {
         [self _drawFlags:lCurLineFlags inRect:lCurLineRect];
      }

      if(shouldShowLineNumbers)
      {
         [self _drawOneNumberInMargin:lCurLineNo inRect:lCurLineRect withFlags:lCurLineFlags];
      }
            
      lCurLineNo ++;
   }

}


-(void)_drawMarkerWithColor:(NSColor*)aBaseColor inRect:(NSRect)aRect
{
   [aBaseColor setFill];
   [[aBaseColor shadowWithLevel:0.5] setStroke];
   
   NSBezierPath *lMarker = [NSBezierPath bezierPath];
   
   [lMarker moveToPoint:NSMakePoint(aRect.origin.x+4, aRect.origin.y+2)];
   [lMarker lineToPoint:NSMakePoint(aRect.origin.x+aRect.size.width - 7, aRect.origin.y+2)];
   [lMarker lineToPoint:NSMakePoint(aRect.origin.x+aRect.size.width - 2, aRect.origin.y+aRect.size.height/2)];
   [lMarker lineToPoint:NSMakePoint(aRect.origin.x+aRect.size.width - 7, aRect.origin.y+aRect.size.height-2)];
   [lMarker lineToPoint:NSMakePoint(aRect.origin.x+4, aRect.origin.y+aRect.size.height-2)];
   [lMarker closePath];
   
   [lMarker stroke];
   [lMarker fill];
   
}

-(void)_drawFlags:(UInt32)aFlags inRect:(NSRect)aRect
{
   if(aFlags&1)
   {
      [self _drawMarkerWithColor:[NSColor blueColor] inRect:aRect];
   }

   if(aFlags&2)
   {
      [self _drawMarkerWithColor:[NSColor redColor] inRect:aRect];
   }
}

-(void)_drawOneNumberInMargin:(unsigned) aNumber inRect:(NSRect)r withFlags:(int)flags
{
   NSString    *s;
   NSSize      stringSize;
   
    NSDictionary *lineNumberAttributes = flags ? marginAttributesWithMarkers : marginAttributes;
    
   s = [NSString stringWithFormat:@"%d", aNumber, nil];
   stringSize = [s sizeWithAttributes:lineNumberAttributes];
   
   // Simple algorithm to center the line number next to the glyph.
   [s drawAtPoint: NSMakePoint( r.origin.x + r.size.width - 6 - stringSize.width, 
                               r.origin.y + ((r.size.height / 2) - (stringSize.height / 2))) 
      withAttributes:lineNumberAttributes];
}

@end
