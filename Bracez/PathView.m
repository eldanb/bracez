//
//  PathView.m
//  JsonMockup
//
//  Created by Eldan on 6/5/10.
//  Copyright 2010 Eldan Ben-Haim. All rights reserved.
//

#import "PathView.h"

@interface PathView () <NSTextFieldDelegate, NSTableViewDataSource, NSTableViewDelegate> {
    NSTextField *_pathExtensionEdit;
    NSTableView *_completionTableView;
    NSWindow *_completionWindow;
    NSArray *_currentSuggestions;
}

@end


@interface CompletionsTableView : NSTableView {
    
}
@end


@interface PathExtensionEditFieldEditor : NSTextView {
    __weak PathView *_pathView;
}

-(instancetype)initWithPathView:(PathView*)pathView;

@end


@interface PathExtensionEditCell : NSTextFieldCell {
    PathExtensionEditFieldEditor *fieldEditor;
    __weak PathView *_pathView;
}

-(instancetype)initWithPathView:(PathView*)pv;

@end

@implementation PathView

@synthesize delegate = _delegate;

- (id)initWithFrame:(NSRect)frame {
    self = [super initWithFrame:frame];
    if (self) {
      _bkCell = [[NSButtonCell alloc] init];
      [_bkCell setBezelStyle:NSSmallSquareBezelStyle];
      [_bkCell setAlignment:NSLeftTextAlignment];
      [_bkCell setWraps:NO];
      [_bkCell setTitle:@""];
      
      _ellipsisCell = [[PathViewComponentCell alloc] init];
      [_ellipsisCell setFont:[self font]];
      [_ellipsisCell setTitle:@"..."];

    }
    return self;
}

-(BOOL) acceptsFirstMouse:(NSEvent *)theEvent
{
   return YES;
}

-(void)mouseDown:(NSEvent *)theEvent
{
   int lModifiers = [theEvent modifierFlags];
   if((lModifiers & NSControlKeyMask) == 0)
   {
      NSPoint lClickPoint = [self convertPoint:[theEvent locationInWindow] fromView:nil];
      PathViewComponentCell *lPathElement =  [self hitTestPathElement:lClickPoint andGetFrame:nil];

      [_currentHilightedCell setHighlighted:NO];
      _currentHilightedCell = lPathElement;
      [_currentHilightedCell setHighlighted:YES];
         
      [self setNeedsDisplay:YES];
   } else
   {
      [self _secondaryClick:theEvent];
   }
}

-(void)_secondaryClick:(NSEvent*)theEvent
{
   NSPoint lClickPoint = [self convertPoint:[theEvent locationInWindow] fromView:nil];
   NSRect lPathElementFrame;
   PathViewComponentCell *lPathElement =  [self hitTestPathElement:lClickPoint andGetFrame:&lPathElementFrame];

   int lIdx = [_pathCells indexOfObject:lPathElement];
   
   if(lIdx!=NSNotFound)
   {
      NSMenu *lMenu = NULL;
      [_delegate pathView:self preparePopupMenu:&lMenu forIndex:lIdx];

      if(lMenu)
      {
         [NSMenu popUpContextMenu:lMenu withEvent:theEvent forView:self withFont:[self font]];
      }
   }
      
   [self setNeedsDisplay:YES];
}

-(void)mouseUp:(NSEvent *)theEvent
{
   if(_currentHilightedCell)
   {
      [_currentHilightedCell setHighlighted:NO];
      int lIdx = [_pathCells indexOfObject:_currentHilightedCell];
      if(lIdx>=0)
      {
         [_delegate pathView:self didClickOnPathComponentIndex:lIdx];
      }
      
      _currentHilightedCell = nil;
      [self setNeedsDisplay:YES];
   }
}

-(void)startPathExtensionEntryWithDefault:(NSString*)defaultPath {
    if(_pathExtensionEdit) {
        [self abortPathExtensionEntry];
    }
    
    __block NSRect lastPathFrame;
    [self iterateWithShownCellsAndRects:^(NSCell *cell, NSRect rect) {
        lastPathFrame = rect;
    }];
    
    NSRect editFrame = NSMakeRect(lastPathFrame.origin.x + lastPathFrame.size.width, lastPathFrame.origin.y, self.frame.size.width, lastPathFrame.size.height);
    _pathExtensionEdit = [[NSTextField alloc] initWithFrame:editFrame];
    _pathExtensionEdit.cell = [[PathExtensionEditCell alloc] initWithPathView:self];
    _pathExtensionEdit.frame = editFrame;
    _pathExtensionEdit.editable = YES;
    _pathExtensionEdit.font = [self font];
    _pathExtensionEdit.focusRingType = NSFocusRingTypeNone;
    _pathExtensionEdit.bordered = NO;
    _pathExtensionEdit.backgroundColor = [NSColor clearColor];
    _pathExtensionEdit.stringValue = defaultPath;
    [_pathExtensionEdit selectText:nil];
    
    CGFloat fittingHeight = [_pathExtensionEdit fittingSize].height;
    editFrame.origin.y = (lastPathFrame.size.height - fittingHeight) / 2 + lastPathFrame.origin.y;
    editFrame.size.height = fittingHeight;
    _pathExtensionEdit.frame = editFrame;
    _pathExtensionEdit.delegate = self;
    
    [self addSubview:_pathExtensionEdit];
    [_pathExtensionEdit becomeFirstResponder];
    
    [self handleEditedExtensionChange];
}

-(void)handleEditedExtensionChange {
    [_completionTableView deselectAll:_completionTableView];
    
    NSString *curEditValue = _pathExtensionEdit.stringValue;
    [self.delegate pathView:self previewPathExtensionString:curEditValue];
    _currentSuggestions = [self.delegate pathView:self proposeCompletionsForString:curEditValue];
    [_completionTableView reloadData];
    
    [self showCompletionListAtCaret];
}

-(BOOL)processCompletionListKeyDownEvent:(NSEvent*)keyEvent {
    unichar keyboardChar = [[keyEvent charactersIgnoringModifiers] characterAtIndex:0];
    switch(keyboardChar) {
        case NSEnterCharacter:
        case NSCarriageReturnCharacter:
            [self handleCompletionSelectionWithTerminatingDot:NO];
            return FALSE;

        case '.':
            [self handleCompletionSelectionWithTerminatingDot:YES];
            return TRUE;

        case NSDownArrowFunctionKey:
            if(_completionTableView.selectedRow < [_completionTableView.dataSource numberOfRowsInTableView:_completionTableView]-1) {
                int selectedRow = _completionTableView.selectedRow+1;
                [_completionTableView selectRowIndexes:[NSIndexSet indexSetWithIndex:selectedRow]
                                  byExtendingSelection:NO];
                [_completionTableView scrollRowToVisible:selectedRow];
            }
            return TRUE;
            
        case NSUpArrowFunctionKey:
            if(_completionTableView.selectedRow > 0) {
                int selectedRow = _completionTableView.selectedRow-1;
                [_completionTableView selectRowIndexes:[NSIndexSet indexSetWithIndex:selectedRow]
                                  byExtendingSelection:NO];
                [_completionTableView scrollRowToVisible:selectedRow];
            }
            return TRUE;
            
        default:
            return FALSE;
    }
}

-(void)abortPathExtensionEntry {
    [self _endPathExtensionEntry];
}

-(void)endPathExtensionEntry {
    NSString *extString = [_pathExtensionEdit stringValue];
    [self _endPathExtensionEntry];
    [self.delegate pathView:self extendByString:extString];
}

-(void)_endPathExtensionEntry {
    [_pathExtensionEdit removeFromSuperview];
    _pathExtensionEdit = nil;
    
    
    [_completionWindow orderOut:nil];
    
    _completionTableView = nil;
    _completionWindow = nil;
}

-(BOOL)pathExtensionActive {
    return !!_pathExtensionEdit;
}

-(void)rightMouseDown:(NSEvent *)theEvent
{
   [self _secondaryClick:theEvent];
}

- (void)drawRect:(NSRect)dirtyRect {
   [_bkCell drawWithFrame:[self bounds] inView:self];
    
    [self iterateWithShownCellsAndRects:^(NSCell *aCell, NSRect aRect) {
       [aCell drawWithFrame:aRect inView:self];
    }];    
}

-(PathViewComponentCell*)hitTestPathElement:(NSPoint)aPoint andGetFrame:(NSRect*)aFrame
{
    __block PathViewComponentCell* foundCell = nil;
    [self iterateWithShownCellsAndRects:^(NSCell *aCell, NSRect aRect) {
        if(NSPointInRect(aPoint, aRect) &&
           [aCell isKindOfClass:[PathViewComponentCell class]])
        {
            foundCell = (PathViewComponentCell*)aCell;
            if(aFrame)
            {
                *aFrame = aRect;
            }
        }
    }];
   return foundCell;
}

-(void)iterateWithShownCellsAndRects:(void (^)(NSCell *, NSRect))block
{
   NSRect lCurFrame = [self bounds];
   lCurFrame.origin.x += 3;
    

   PathViewComponentCell *lLastCell = [_pathCells lastObject];
   PathViewComponentCell *lPreLastCell = [_pathCells count]>1?[_pathCells objectAtIndex:[_pathCells count]-2]:NULL;
   
   CGFloat lWidthLim = lCurFrame.size.width - (lLastCell?[lLastCell cellSize].width:0) - (lPreLastCell?[lPreLastCell cellSize].width:0) - [_ellipsisCell cellSize].width;
   
   for (int lCellIdx=0; lCellIdx<[_pathCells count]; lCellIdx++) {
      NSCell *lCell = [_pathCells objectAtIndex:lCellIdx];
      lCurFrame.size.width = [lCell cellSize].width;      
      
      // Skip rest of cells if needed
      if((lCurFrame.origin.x + lCurFrame.size.width > lWidthLim) && 
         (lCellIdx<[_pathCells count]-2) )
      {      
         lCurFrame.size.width = [_ellipsisCell cellSize].width;
         
         block(_ellipsisCell, lCurFrame);

         lCurFrame.origin.x += lCurFrame.size.width;
         lCellIdx=[_pathCells count]-3;
         
         continue;
      }

      block(lCell, lCurFrame);

      lCurFrame.origin.x += lCurFrame.size.width;
   }
}

-(void)setShownPath:(NSArray*)aPath
{
   NSMutableArray *lArr = [NSMutableArray arrayWithCapacity:[aPath count]];
   for(NSObject *lObj in aPath)
   {
      [lArr addObject:[self cellForPathComponent:lObj]];
   }
   
   _pathCells = lArr;
   
   [self setNeedsDisplay:YES];
}

-(NSArray*)shownPath {
    NSMutableArray *arr = [NSMutableArray arrayWithCapacity:[_pathCells count]];
    for(PathViewComponentCell *cell in _pathCells) {
        [arr addObject:[cell stringValue]];
    }
    
    return arr;
}

-(NSString*)pathStringAtIndex:(int)aIdx
{
   return [(PathViewComponentCell *)[_pathCells objectAtIndex:aIdx] title];
}

-(PathViewComponentCell*)cellForPathComponent:(NSObject*)aComponent
{
    PathViewComponentCell *lFldCell = [[PathViewComponentCell alloc] init];
   [lFldCell setTitle:[aComponent description]];
   [lFldCell setFont:[self font]];

   return lFldCell;
}

-(void)controlTextDidEndEditing:(NSNotification *)obj {
    [self endPathExtensionEntry];
}

-(void)controlTextDidChange:(NSNotification *)obj {
    [self handleEditedExtensionChange];
}


-(void)showCompletionListAtCaret {
    // Calculate position for completion window
    NSTextView *editor = (NSTextView*)[_pathExtensionEdit currentEditor];
    NSRange selection = [editor.selectedRanges.firstObject rangeValue];
    NSLayoutManager *layoutManager = editor.layoutManager;

    int glyphIdx = [layoutManager glyphIndexForCharacterAtIndex:selection.location];
    CGRect glyphLineFrag = [layoutManager lineFragmentRectForGlyphAtIndex:glyphIdx effectiveRange:nil];
    CGPoint glyphPos = [layoutManager locationForGlyphAtIndex:glyphIdx];
    glyphPos.x += glyphLineFrag.origin.x;
    glyphPos.y += glyphLineFrag.origin.y;
    
    glyphPos = [self.window.contentView convertPoint:glyphPos fromView:editor];
    NSRect r;
    r.origin = glyphPos;
    r = [self.window convertRectToScreen:r];
    glyphPos = r.origin;
    glyphPos.y -= (editor.frame.size.height + 3);

    // Create window if it doesn't exist
    if(!_completionTableView) {
        _completionWindow = [[NSWindow alloc] initWithContentRect:CGRectMake(glyphPos.x, glyphPos.y, 0, 0)
                                                        styleMask:0
                                                          backing:NSBackingStoreBuffered
                                                            defer:NO];
        
        _completionWindow.animationBehavior = NSWindowAnimationBehaviorAlertPanel;
        _completionWindow.hasShadow = YES;
        _completionWindow.opaque = NO;
        _completionWindow.backgroundColor = [NSColor clearColor];
        
        _completionTableView = [[CompletionsTableView alloc] initWithFrame:NSZeroRect];
        _completionTableView.font = [self font];
        _completionTableView.headerView = nil;
        [_completionTableView setAction:@selector(completionCellClicked:)];
        NSTableColumn *tabCol = [[NSTableColumn alloc] initWithIdentifier:@"col1"];
        [tabCol.dataCell setFont:[self font]];
        tabCol.editable = NO;
        [_completionTableView addTableColumn:tabCol];
        [_completionTableView setDataSource:self];
        [_completionTableView setDelegate:self];

        NSScrollView *scrollView = [[NSScrollView alloc] initWithFrame:NSZeroRect];
        scrollView.wantsLayer = YES;
        scrollView.layer.masksToBounds = YES;
        scrollView.layer.cornerRadius = 5;
        scrollView.layer.borderColor = [[NSColor lightGrayColor] CGColor];
        scrollView.layer.borderWidth = 1;

        [scrollView setDocumentView:_completionTableView];

        _completionWindow.contentView = scrollView;
        [_completionWindow orderFront:nil];
    }
    
    // Move window to current caret position
    [_completionWindow setFrame:CGRectMake(glyphPos.x, glyphPos.y-150, 300, 150) display:YES];
}

-(void)handleCompletionSelectionWithTerminatingDot:(BOOL)terminatingDot {
    if([_completionTableView selectedRow] != -1) {
        NSString *selectedSuggestion = [_currentSuggestions objectAtIndex:[_completionTableView selectedRow]];
        
        NSString *newPath = [self.delegate pathView:self updateExtensionValue:_pathExtensionEdit.stringValue forSelectedCompletion:selectedSuggestion];
        
        if(terminatingDot) {
            newPath = [newPath stringByAppendingString:@"."];
        }
        
        _pathExtensionEdit.stringValue = newPath;
        _pathExtensionEdit.currentEditor.selectedRange = NSMakeRange([newPath length], 0);

        [self handleEditedExtensionChange];
    }
}

-(void)completionCellClicked:(id)action {
    [self handleCompletionSelectionWithTerminatingDot:YES];
}

-(void)tableViewSelectionDidChange:(NSNotification *)notification {
}

-(NSInteger)numberOfRowsInTableView:(NSTableView *)tableView {
    return [_currentSuggestions count];
}

- (id)tableView:(NSTableView *)tableView objectValueForTableColumn:(NSTableColumn *)tableColumn row:(NSInteger)row {
    return [_currentSuggestions objectAtIndex:row];
}

-(NSFont*)font {
    return [NSFont userFontOfSize:11.0];
}
@end


@implementation PathViewComponentCell 

- (NSSize)cellSize
{
   NSSize lRet =  [super cellSize];
   lRet.width+=10;
   return lRet;
}

-(void)setHighlighted:(BOOL)flag
{
   if(flag)
   {
      [self setTextColor:[NSColor whiteColor]];
      [self setBackgroundColor:[NSColor blueColor]];
      [self setDrawsBackground:YES];
   } else
   {
      [self setDrawsBackground:NO];
      [self setTextColor:[NSColor controlTextColor]];
   }
}

-(void)drawWithFrame:(NSRect)cellFrame inView:(NSView *)controlView;
{
   BOOL lSaveBk = [self drawsBackground];
   [self setDrawsBackground:NO];

   NSBezierPath* lArrow = [NSBezierPath bezierPath];
 
   [[NSColor windowFrameColor] setStroke];

   [lArrow setLineWidth:1];
   [lArrow moveToPoint:NSMakePoint(cellFrame.origin.x+cellFrame.size.width-8, cellFrame.origin.y+cellFrame.size.height-2)];
   [lArrow lineToPoint:NSMakePoint(cellFrame.origin.x+cellFrame.size.width-2, cellFrame.origin.y+cellFrame.size.height/2)];
   [lArrow lineToPoint:NSMakePoint(cellFrame.origin.x+cellFrame.size.width-8, cellFrame.origin.y)];
   [lArrow stroke];

   
   if(lSaveBk)
   {
      [lArrow lineToPoint:NSMakePoint(cellFrame.origin.x-7, cellFrame.origin.y)];
      [lArrow lineToPoint:NSMakePoint(cellFrame.origin.x-1, cellFrame.origin.y+cellFrame.size.height/2)];
      [lArrow lineToPoint:NSMakePoint(cellFrame.origin.x-7, cellFrame.origin.y+cellFrame.size.height-2)];
      
      [[self backgroundColor] setFill];
      [lArrow fill];
   }

   NSRect lTextFrame = cellFrame;
   lTextFrame.origin.y += (cellFrame.size.height-[super cellSize].height)/2-8;
   [super drawWithFrame:lTextFrame inView:controlView];
   
   [self setDrawsBackground:lSaveBk];
}

@end

@implementation CompletionsTableView

-(BOOL)acceptsFirstResponder {
    return NO;
}

@end


@implementation PathExtensionEditFieldEditor

-(instancetype)initWithPathView:(PathView*)pathView {
    self = [super initWithFrame:NSZeroRect];
    if(self) {
        _pathView = pathView;
    }
    return self;
}

- (void)keyDown:(NSEvent *)event {
    if(![_pathView processCompletionListKeyDownEvent:event]) {
        [super keyDown:event];
    }
}
@end



@implementation PathExtensionEditCell

-(instancetype)initWithPathView:(PathView*)pv {
    self = [super initTextCell:@""];
    if(self) {
        _pathView = pv;
    }
    return self;
}

- (NSTextView *)fieldEditorForView:(NSView *)controlView {
    if(!fieldEditor) {
        fieldEditor = [[PathExtensionEditFieldEditor alloc] initWithPathView:_pathView];
        fieldEditor.backgroundColor = [NSColor whiteColor];
        fieldEditor.fieldEditor = YES;
    }
    return fieldEditor;
}

@end


