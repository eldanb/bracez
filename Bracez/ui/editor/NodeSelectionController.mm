//
//  NodeSelectionController.mm
//  JsonMockup
//
//  Created by Eldan on 15/5/10.
//  Copyright 2010 Eldan Ben-Haim. All rights reserved.
//

#import "NodeSelectionController.h"
#import "JsonCocoaNode.h"
#import "NSIndexPathAdditions.h"
#import "JsonMarker.h"

#include <codecvt>
#include <locale>

@implementation NodeSelectionController 

- (void)awakeFromNib
{
    docListenerBridge = new JsonFileListenerObjCBridge(self);
    [document jsonFile]->addListener(docListenerBridge);
    
    [treeController addObserver:self forKeyPath:@"selectionIndexPaths" options:0 context:nil];
    [pathView setDelegate:self];
    [document addObserver:self forKeyPath:@"bookmarkMarkers" options:0 context:NULL];
    [document addObserver:self forKeyPath:@"problems" options:0 context:NULL];
    
    syncingNodeAndTextSel = false;
    inhibitHistoryRecord = false;
    
    lastNavGroup=NAVGROUP_NONE;
    
    
    inhibitHistoryRecord = true;
    [self refreshSelectionFromTextView];
    inhibitHistoryRecord = false;
}

-(void)dealloc
{
    delete docListenerBridge;
}

-(NSArray<NSString*>*)_nodeNamesForIndexPath:(NSIndexPath*)path lastNodeInto:(JsonCocoaNode**)lastNode {
    
    if([treeController.content count]) {
        // Selection indexes of tree controller were changed.
        
        JsonCocoaNode *lNode = [treeController.content objectAtIndex:0];
        NSUInteger lPathCompLen = [path length];
        
        NSMutableArray *lPathArr = [NSMutableArray arrayWithObject:@"root"];
        
        // Find node
        for(int lPathCompIdx=1; lPathCompIdx<lPathCompLen; lPathCompIdx++)
        {
            lNode = [lNode objectInChildrenAtIndex:(int)[path indexAtPosition:lPathCompIdx]];
            
            [lPathArr addObject:[lNode nodeName]];
        }
        
        if(lastNode) {
            *lastNode = lNode;
        }
        
        return lPathArr;
    } else {
        if(lastNode) {
            *lastNode = nil;
        }
        
        return [NSMutableArray arrayWithObject:@"root"];
    }
}

-(void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context
{
    if([keyPath isEqualTo:@"selectionIndexPaths"])
    {
        [self selectionIndexPathsUpdated];
    } else if([keyPath isEqualTo:@"bookmarkMarkers"]) {
        [self _bookmarksChanged];
    } else if([keyPath isEqualTo:@"problems"]) {
        [self _problemsChanged];
    }

}

-(void)selectionIndexPathsUpdated {
    if(document.isSemanticModelDirty) {
        return;
    }

    if(![treeController selectionIndexPath]) {
        return;
    }
 
    JsonCocoaNode *lLastNode = nil;
    NSArray* lPathArr = [self _nodeNamesForIndexPath:[treeController selectionIndexPath] lastNodeInto:&lLastNode];
    if(!pathView.pathExtensionActive) {
        [pathView setShownPath:lPathArr];
    }
    
    [self.projectionTableDs selectNode:lLastNode];

    if(!syncingNodeAndTextSel)
    {
        if(!document.isSemanticModelUpdateInProgress) {
            [self recordNavigationPointBeforeNavGroup:NAVGROUP_NONE];
        
            syncingNodeAndTextSel = true;
            json::TextRange lNodeRange = [lLastNode textRange].intersectWith(document.jsonFile->getDom()->getTextRange());
            
            NSRange range = NSMakeRange(lNodeRange.start.getAddress(), lNodeRange.end - lNodeRange.start);

            [self selectTextRange:range];

            syncingNodeAndTextSel = false;
        }
    }
}

- (void)pathView:(PathView *)aPathView didClickOnPathComponentIndex:(int)aIdx
{
    NSIndexPath *lPath = [treeController selectionIndexPath];
    
    if(aIdx+1<=[lPath length])
    {
        NSUInteger * lIndices = new NSUInteger[ [lPath length] ];
        [lPath getIndexes:lIndices];
        
        [treeController setSelectionIndexPath:[NSIndexPath indexPathWithIndexes:lIndices length:aIdx]];
        [aPathView startPathExtensionEntryWithDefault:@""];
        delete [] lIndices;
    }
}


-(void)pathMenuItemSelected:(NSMenuItem*)aItem
{
    [treeController setSelectionIndexPath:[aItem representedObject]];
}

- (IBAction) markerListSelectionChanged:(id)aSender
{
    TextCoordinate lTogo;
    
    NSTableView *lSelectingTable = aSender;
    NSArrayController *lArrayCtler = [[lSelectingTable infoForBinding:@"content"] valueForKey:NSObservedObjectKey];
    
    JsonMarker *lSelMarker = NULL;
    if(lArrayCtler.selectedObjects.count) {
        lSelMarker = [[lArrayCtler selectedObjects] objectAtIndex:0];
        
    }
    
    if(lSelMarker)
    {
        lTogo = [lSelMarker coordinate];
        
        [self selectTextRange:NSMakeRange(lTogo.getAddress(), 0)];
        
        lastNavGroup = NAVGROUP_NONE;
        
        [[textView window] makeFirstResponder:textView];
    }
}



- (void)pathView:(PathView *)aPathView preparePopupMenu:(NSMenu**)aMenu forIndex:(int)aIdx
{
    if(aIdx<1)
    {
        return;
    }
    
    // Find node corresponding to clicked path element
    NSIndexPath *lPath = [treeController selectionIndexPath];
    JsonCocoaNode *lNode = [[treeController content] objectAtIndex:0];
    for(int lPathCompIdx=1; lPathCompIdx<aIdx; lPathCompIdx++)
    {
        lNode = [lNode objectInChildrenAtIndex:(int)[lPath indexAtPosition:lPathCompIdx]];
    }
    
    NSUInteger lSelIdx = [lPath indexAtPosition:aIdx];
    
    // Prepare data for checking and building alternate paths
    NSUInteger *lPathIndices = new NSUInteger[[lPath length]];
    [lPath getIndexes:lPathIndices];
    NSMutableArray *lPathSuffixStrings = [NSMutableArray arrayWithCapacity:[lPath length] - aIdx + 2];
    for(int lPathCompIdx=aIdx+1; lPathCompIdx<[lPath length]; lPathCompIdx++)
    {
        [lPathSuffixStrings addObject:[aPathView pathStringAtIndex:lPathCompIdx]];
    }
    
    // Setup menu with siblings of selection
    NSMenu *lMenu = [[NSMenu alloc] initWithTitle:@""];
    for(int lSiblingIdx=0; lSiblingIdx < [lNode countOfChildren]; lSiblingIdx++)
    {
        JsonCocoaNode *lSibling = [lNode objectInChildrenAtIndex:lSiblingIdx];
        
        lPathIndices[aIdx] = lSiblingIdx;
        
        // Attempt to construct a path equivalent to the suffix of the current path
        JsonCocoaNode *lCurSuffixNavNode = lSibling;
        int lCurPathNavIdxStep = aIdx+1;
        bool lGotPathSuffix = [lPathSuffixStrings count]>0;
        for(NSString *lNavPath in lPathSuffixStrings)
        {
            int lCurNavIdx = [lCurSuffixNavNode indexOfChildWithName:lNavPath];
            if( lCurNavIdx<0 )
            {
                lGotPathSuffix = false;
                break;
            }
            
            lPathIndices[lCurPathNavIdxStep] = lCurNavIdx;
            lCurPathNavIdxStep++;
            
            lCurSuffixNavNode = [lCurSuffixNavNode objectInChildrenAtIndex:lCurNavIdx];
        }
        
        // If we got a suffix, first construct a menu for the path with the suffix
        if(lGotPathSuffix)
        {
            NSIndexPath *lIdxPath = [NSIndexPath indexPathWithIndexes:lPathIndices length:[lPath length]];
            
            NSString *lIdxPathDesc = [NSString stringWithFormat:@"%@ ➤ … ➤ %@ (%@)",
                                      [[lNode objectInChildrenAtIndex:lSiblingIdx] nodeName],
                                      [lCurSuffixNavNode nodeName],
                                      [lCurSuffixNavNode nodeValue] ];
            
            NSMenuItem *lItem = [[NSMenuItem alloc] initWithTitle:lIdxPathDesc action:@selector(pathMenuItemSelected:) keyEquivalent:@""];
            [lItem setRepresentedObject:lIdxPath];
            [lItem setTarget:self];
            [lItem setState:lSiblingIdx == lSelIdx ? NSControlStateValueOn : NSControlStateValueOff];
            [lMenu addItem:lItem];
        }
        
        // Next construct a menu item for the path without the suffix
        NSIndexPath *lIdxPath=[NSIndexPath indexPathWithIndexes:lPathIndices length:aIdx+1];
        NSString *lIdxPathDesc=[[lNode objectInChildrenAtIndex:lSiblingIdx] nodeName];
        NSMenuItem *lItem = [[NSMenuItem alloc] initWithTitle:lIdxPathDesc action:@selector(pathMenuItemSelected:) keyEquivalent:@""];
        [lItem setRepresentedObject:lIdxPath];
        [lItem setTarget:self];
        
        if(lGotPathSuffix) // This is an alternate item if we've got a suffix
        {
            [lItem setAlternate:YES];
            [lItem setKeyEquivalentModifierMask:NSEventModifierFlagOption];
        }
        
        [lItem setState:lSiblingIdx == lSelIdx ? NSControlStateValueOn : NSControlStateValueOff];
        [lMenu addItem:lItem];
    }
    
    *aMenu = lMenu;
}

- (void)pathView:(PathView *)aPathView previewPathExtensionString:(NSString *)extensionString {
    
    NSString *lPreviewPath = nil;
    
    NSArray* lPathArr = pathView.shownPath;
    if([lPathArr count]>1) {
        NSArray *lRootTrimmedPathArr = [lPathArr subarrayWithRange:NSMakeRange(1, [lPathArr count]-1)];
        NSString *lBasePath = [lRootTrimmedPathArr componentsJoinedByString:@"."];
        lPreviewPath = [NSString stringWithFormat:@"%@.%@", lBasePath, extensionString];
    } else {
        lPreviewPath = extensionString;
    }
    
    NSIndexPath *lCalculatedPath = [document pathFromJsonPathString:lPreviewPath];
    
    if(lCalculatedPath) {
        [treeController setSelectionIndexPath:lCalculatedPath];
    }
}

- (void)pathView:(PathView *)aPathView extendByString:(NSString*)string {
    [self pathView:aPathView previewPathExtensionString:string];
}


-(void)pathViewRequestMoveToParent:(PathView*)pathView {
    if(self.canGoParentNode) {
        [self goParentNode:self];
    }
}

- (NSArray<NSString *> *)pathView:(PathView *)aPathView proposeCompletionsForString:(NSString *)extensionString {
    
    NSArray* lPathArr = pathView.shownPath;
    if([lPathArr count]>0) {
        lPathArr = [lPathArr subarrayWithRange:NSMakeRange(1, [lPathArr count]-1)];
    }
    
    NSArray* lExtPathArray = [extensionString componentsSeparatedByString:@"."];
    NSArray* lTrimmedExtPathArray = [lExtPathArray subarrayWithRange:NSMakeRange(0, lExtPathArray.count-1)];
    NSArray *lQueryPathArray = [lPathArr arrayByAddingObjectsFromArray:lTrimmedExtPathArray];
    
    NSString *lastPathComponent = [lExtPathArray lastObject];
    
    NSIndexPath *lQueryPath = [document pathFromJsonPathString:[lQueryPathArray componentsJoinedByString:@"."]];
    
    JsonCocoaNode *lLastNode = nil;
    [self _nodeNamesForIndexPath:lQueryPath lastNodeInto:&lLastNode];
    if(lLastNode) {
        int lChildCount = [lLastNode countOfChildren];
        NSMutableArray *lRet = [NSMutableArray arrayWithCapacity:lChildCount];
        for(int idx=0; idx<lChildCount; idx++) {
            NSString *proposal = [[lLastNode objectInChildrenAtIndex:idx] nodeName];
            if(![lastPathComponent length] ||
               [proposal hasPrefix:lastPathComponent]) {
                [lRet addObject:proposal];
            }
        }
        return lRet;
    } else {
        return @[];
    }
}

- (NSString*)pathView:(PathView *)aPathView updateExtensionValue:(NSString*)curExtension forSelectedCompletion:(NSString*)completion {
    NSArray* lExtPathArray = [curExtension componentsSeparatedByString:@"."];
    NSArray* lTrimmedExtPathArray = [lExtPathArray subarrayWithRange:NSMakeRange(0, lExtPathArray.count-1)];
    NSArray *lCompletedPathArray = [lTrimmedExtPathArray arrayByAddingObject:completion];
    return [lCompletedPathArray componentsJoinedByString:@"."];
}

-(void)refreshSelectionFromTextView;
{
    syncingNodeAndTextSel = true;
    
    int lRow, lCol;
    [document translateCoordinate:TextCoordinate([textView selectedRange].location) toRow:&lRow col:&lCol];
    [coordView setCoordinateRow:lRow col:lCol];
    
    if(!document.isSemanticModelDirty) {
        NSIndexPath *lNewSelIndexPath = [document findPathContaining:TextCoordinate([textView selectedRange].location)];
        if(![treeController.selectionIndexPath isEqualTo:lNewSelIndexPath]) {
            [treeController setSelectionIndexPath:lNewSelIndexPath];
        }
    }

    syncingNodeAndTextSel = false;
}

-(NSString*)currentPathAsJsonQuery {
    static NSCharacterSet *nonBarewordCharacters = [[NSCharacterSet characterSetWithCharactersInString:@"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_"] invertedSet];
    using convert_type = std::codecvt_utf8<wchar_t>;
    static std::wstring_convert<convert_type, wchar_t> wide_utf8_converter;

    NSIndexPath *selIndexPath = [document findPathContaining:TextCoordinate([textView selectedRange].location)];
    JsonCocoaNode *curNode = document.rootNode;
    NSMutableString *path = [NSMutableString stringWithString:@"$"];
    
    for(int i=1; i<selIndexPath.length; i++) {
        int curIndex = (int)[selIndexPath indexAtPosition:i];
        JsonCocoaNode *nextNode = [curNode objectInChildrenAtIndex:curIndex];
        if(curNode.nodeType == ntArray) {
            [path appendFormat:@"[%d]", curIndex];
        } else
        if([nextNode.nodeName rangeOfCharacterFromSet:nonBarewordCharacters options:0].length != 0) {
            std::wstring jsonName = json::jsonizeString(wide_utf8_converter.from_bytes([nextNode.nodeName UTF8String]));
            
            [path appendFormat:@"[%@]", [[NSString alloc] initWithBytes:jsonName.c_str()
                                                                 length:jsonName.length() * sizeof(wchar_t)
                                                               encoding:NSUTF32LittleEndianStringEncoding]];
        } else
        {
            [path appendFormat:@".%@", nextNode.nodeName];
        }
        
        curNode = nextNode;
    }
    
    return path;
}

- (void)projectionTableController:(ProjectionTableController *)sender didSelectNode:(Node *)node {
    if(!syncingNodeAndTextSel) {
        json::TextRange nodeRange = node->getAbsTextRange();
        [self selectTextRange:NSMakeRange(nodeRange.start, nodeRange.length())];
    }
}

- (void)textViewDidChangeSelection:(NSNotification *)aNotification
{
    if(!syncingNodeAndTextSel &&  // Don't reflect selection change if sync in progress
       ![document isSemanticModelTextChangeInProgress] // Don't update selection if text is changed due to semantic model changes;
       // if this is a tree change and then we notify selection change
       // the tree may recursively trigger node updates
       )
    {
        [self recordNavigationPointBeforeNavGroup:NAVGROUP_CURSOR_MOVEMENT];
        [self refreshSelectionFromTextView];
    }
}

-(BOOL)canGoNextSibling
{
    json::Node *lNode = [[[treeController selectedObjects] objectAtIndex:0] proxiedElement];
    if(!lNode)
    {
        return NO;
    }
    
    return lNode->getParent()->getIndexOfChild(lNode)<lNode->getParent()->getChildCount()-1;
}


-(IBAction)goNextSibling:(id)aSender
{
    
    if(![self canGoNextSibling])
    {
        return;
    }
    
    NSIndexPath *lCurPath = [treeController selectionIndexPath];
    NSIndexPath *lNewPath = [lCurPath indexPathOfNextSibling];
    
    [self recordNavigationPointBeforeNavGroup:NAVGROUP_NODE_STEP];
    inhibitHistoryRecord = true;
    [treeController setSelectionIndexPath:lNewPath];
    inhibitHistoryRecord = false;
}

-(BOOL)canGoPrevSibling
{
    NSIndexPath *lCurPath = [treeController selectionIndexPath];
    NSIndexPath *lNewPath = [lCurPath indexPathOfPrevSibling];
    return lNewPath != nil;
}

-(IBAction)goPrevSibling:(id)aSender
{
    NSIndexPath *lCurPath = [treeController selectionIndexPath];
    NSIndexPath *lNewPath = [lCurPath indexPathOfPrevSibling];
    
    if(lNewPath)
    {
        [self recordNavigationPointBeforeNavGroup:NAVGROUP_NODE_STEP];
        inhibitHistoryRecord = true;
        [treeController setSelectionIndexPath:lNewPath];
        inhibitHistoryRecord = false;
    }
}


-(BOOL)canGoParentNode
{
    return [[treeController selectionIndexPath] length]>1;
}

-(IBAction)goParentNode:(id)aSender
{
    if([self canGoParentNode])
    {
        NSIndexPath *lNewPath = [[treeController selectionIndexPath] indexPathByRemovingLastIndex];
        [self recordNavigationPointBeforeNavGroup:NAVGROUP_NODE_STEP];
        inhibitHistoryRecord = true;
        [treeController setSelectionIndexPath:lNewPath];
        inhibitHistoryRecord = false;
    }
}

-(BOOL)canGoFirstChild
{
    return treeController.selectedObjects.count && [[treeController.selectedObjects objectAtIndex:0] countOfChildren] > 0;
}

-(IBAction)goFirstChild:(id)aSender
{
    if([self canGoFirstChild])
    {
        NSIndexPath *lNewPath = [[treeController selectionIndexPath] indexPathByAddingIndex:0];
        
        [self recordNavigationPointBeforeNavGroup:NAVGROUP_NODE_STEP];
        inhibitHistoryRecord = true;
        [treeController setSelectionIndexPath:lNewPath];
        inhibitHistoryRecord = false;
    }
}

-(BOOL)canNavigateBack
{
    return !backNavs.empty();
}

- (IBAction)navigateBack:(id)aSender
{
    if(backNavs.empty())
        return;
    
    inhibitHistoryRecord = true;
    
    TextCoordinate lCurCoord = TextCoordinate([textView selectedRange].location);
    
    forwardNavs.push_back(lCurCoord);
    
    TextCoordinate lTogo = backNavs.back();
    backNavs.pop_back();
    
    [self selectTextRange:NSMakeRange(lTogo.getAddress(), 0)];
    
    lastNavGroup = NAVGROUP_NONE;
    
    inhibitHistoryRecord = false;
    
    [[textView window] makeFirstResponder:textView];
}

-(BOOL)canNavigateForward
{
    return !forwardNavs.empty();
}

- (IBAction)navigateForward:(id)aSender
{
    if(forwardNavs.empty())
        return;
    
    inhibitHistoryRecord = true;
    
    TextCoordinate lCurCoord = TextCoordinate([textView selectedRange].location);
    
    backNavs.push_back(lCurCoord);
    
    TextCoordinate lTogo = forwardNavs.back();
    forwardNavs.pop_back();
    
    [self selectTextRange:NSMakeRange(lTogo.getAddress(), 0)];
    
    lastNavGroup = NAVGROUP_NONE;
    
    inhibitHistoryRecord = false;
    
    [[textView window] makeFirstResponder:textView];
}

- (void)recordNavigationPointBeforeNavGroup:(int)aGroup;
{
    if(inhibitHistoryRecord)
        return;
    
    if(lastNavGroup<0 ||
       aGroup != lastNavGroup)
    {
        TextCoordinate lCurCoord = TextCoordinate([textView selectedRange].location);
        
        forwardNavs.clear();
        backNavs.push_back(lCurCoord);
    }
    
    lastNavGroup = aGroup;
}

- (IBAction) toggleBookmark:(id)aSender
{
    int lLine = [self lineNumberForCharacterIndex:[textView selectedRange].location];
    if(document.bookmarks.hasBookmarkAt(lLine))
    {
        document.bookmarks.removeBookmark(lLine);
    } else {
        document.bookmarks.addBookmark(lLine);
    }
}

- (IBAction) nextBookmark:(id)aSender
{
    int lLine = [self lineNumberForCharacterIndex:[textView selectedRange].location];
    if(document.bookmarks.findNextBookmark(lLine))
    {
        TextCoordinate lLineStart = document.jsonFile->getLineFirstCharacter(lLine);
        
        [self selectTextRange:NSMakeRange(lLineStart.getAddress(), 0)];
        [[textView window] makeFirstResponder:textView];
    }
}

- (IBAction) prevBookmark:(id)aSender
{
    int lLine = [self lineNumberForCharacterIndex:[textView selectedRange].location];
    if(document.bookmarks.findPrevBookmark(lLine))
    {
        TextCoordinate lLineStart = document.jsonFile->getLineFirstCharacter(lLine);
        [self selectTextRange:NSMakeRange(lLineStart.getAddress(), 0)];
        [[textView window] makeFirstResponder:textView];
    }
}

-(void)selectTextRange:(NSRange)range {
    [textView setSelectedRange:range];
    range.length = 0;
    [textView scrollRangeToVisible:range];
}

-(NSRange)textSelection {
    return textView.selectedRange;
}

-(BOOL)canCopyPath
{
    return YES;
}

-(IBAction)copyPath:(id)aSender
{
    NSString *lPath = [self currentPathAsJsonQuery];
    
    NSPasteboard *lPasteBoard = [NSPasteboard generalPasteboard];
    [lPasteBoard declareTypes:[NSArray arrayWithObject:NSPasteboardTypeString] owner:nil];
    [lPasteBoard setString:lPath forType:NSPasteboardTypeString];
}

- (void)connectGutterView:(id)aGutterView
{
    gutterView = aGutterView;
}

- (int)lineNumberForCharacterIndex:(NSUInteger)aIdx
{
    int lRow, lCol;
    
    [document translateCoordinate:TextCoordinate(aIdx) toRow:&lRow col:&lCol];
    
    return lRow;
}

- (NSUInteger)characterIndexForFirstCharOfLine:(int)aIdx {
    return [document characterIndexForFirstCharOfLine:aIdx];
}


- (UInt32)markersForLine:(int)aLine 
{
    UInt32 lRet = 0;
    
    if(document.bookmarks.hasBookmarkAt(aLine))
    {
        lRet |= 1;
    }
    
    TextCoordinate lLineStart = document.jsonFile->getLineStart(aLine);
    TextCoordinate lLineEnd = document.jsonFile->getLineEnd(aLine);
    
    
    TextCoordinate lNextError = lLineStart;
    if([document errors]->nextMarker(lNextError) &&
       lNextError<=lLineEnd)
    {
        lRet |= 2;
    }
    
    return lRet;
}

- (NSUInteger)lineCount {
    return [document lineCount];
}


- (void)_bookmarksChanged
{
    [gutterView markersChanged];
}

- (void)_problemsChanged {
    [gutterView markersChanged];
}

static void adjustNavEntries(vector<TextCoordinate> &aNavEntries, TextCoordinate aOldOffset, TextLength aOldLength, TextLength aNewLength)
{
    int lIdx=0;
    while(lIdx<aNavEntries.size())
    {
        if(aNavEntries[lIdx]>aOldOffset)
        {
            if(aNavEntries[lIdx]<aOldOffset+aOldLength)
            {
                aNavEntries.erase(aNavEntries.begin()+lIdx);
            } else
            {
                aNavEntries[lIdx] += aNewLength - aOldLength;
                lIdx++;
            }
        } else {
            lIdx++;
        }
        
    }
}


- (BOOL)validateUserInterfaceItem:(id <NSValidatedUserInterfaceItem>)anItem
{
    NSString *lActionName = NSStringFromSelector([anItem action]);
    NSString *lValidatorName = [NSString stringWithFormat:@"can%c%@", toupper([lActionName characterAtIndex:0]), [lActionName substringWithRange:NSMakeRange(1, [lActionName length]-2)]];
    SEL lValidatorSelector = NSSelectorFromString(lValidatorName);
    
    if([self respondsToSelector:lValidatorSelector])
    {
        BOOL lRet;
        NSInvocation *lCanDoInvocation = [NSInvocation invocationWithMethodSignature:[self methodSignatureForSelector:lValidatorSelector]];
        [lCanDoInvocation setSelector:lValidatorSelector];
        [lCanDoInvocation invokeWithTarget:self];
        [lCanDoInvocation getReturnValue:&lRet];
        
        return lRet;
    }
    
    return YES;
}

-(void)notifyJsonTextSpliced:(json::JsonFile *)aSender from:(TextCoordinate)aOldOffset length:(TextLength)aOldLength newLength:(TextLength)aNewLength affectedLineStart:(TextCoordinate)aOldLineStart numDeletedLines:(TextLength)aOldLineLength numAddedLines:(TextLength)aNewLineLength
{
    // TODO use markerlist?
    adjustNavEntries(backNavs, aOldOffset, aOldLength, aNewLength);
    adjustNavEntries(forwardNavs, aOldOffset, aOldLength, aNewLength);
}

-(void)notifyErrorsChanged:(json::JsonFile*)aSender
{
}

- (void)notifyNodeInvalidated:(json::JsonFile *)aSender nodePath:(const json::JsonPath &)nodePath {
    
}

@end
