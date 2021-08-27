//
//  JSONWindow.h
//
//  Created by Eldan on 7/5/10.
//  Copyright 2010 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "ProjectionDefinitionFavoritesAdapter.h"

@class NodeSelectionController;
@class TextEditorGutterView;
@class JsonPathSearchController;
@class HistoryAndFavoritesControl;

@interface JSONWindow : NSWindow <ProjectionDefinitionFavoritesAdapterDelegate> {
    IBOutlet NSTextView *textEditor;
    IBOutlet NSScrollView *textEditorScroll;
    IBOutlet __weak NSTreeController *domController;
    IBOutlet NSOutlineView *treeView;
    
    IBOutlet NSTextField *jqQueryInput;
    IBOutlet NSTextView *jqQueryResult;
    IBOutlet HistoryAndFavoritesControl *jqQueryFavHist;
    
    IBOutlet NSMenu *ViewModeMenu;
    IBOutlet __weak NodeSelectionController *selectionController;
    
    IBOutlet JsonPathSearchController *jsonPathSearchController;
    
    __weak IBOutlet NSTableView *projectionTable;
}

-(void)loadPreferences;
-(void)removeJsonNode:(id)aSender;
- (void)forwardInvocation:(NSInvocation *)anInvocation;

-(id)_getActionForwardingTarget:(SEL)aSelector;

-(IBAction)indentSelectionAction:(id)sender;

@end
