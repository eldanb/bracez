//
//  NodeSelectionController.h
//  JsonMockup
//
//  Created by Eldan on 15/5/10.
//  Copyright 2010 Eldan Ben-Haim. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "PathView.h"
#import "CoordView.h"
#import "JsonDocument.h"
#import "TextEditorGutterView.h"
#import "JsonFileListenerObjCBridge.h"

#include "LinesAndBookmarks.h"

#define NAVGROUP_NONE   -1
#define NAVGROUP_CURSOR_MOVEMENT   0
#define NAVGROUP_NODE_STEP 1

@interface NodeSelectionController : NSObject<NSUserInterfaceValidations, GutterViewModel,
                                                ObjCJsonFileChangeListener, PathViewComponentCellNotifications> {
   IBOutlet NSObjectController *docController;
   IBOutlet NSTreeController *treeController;

   IBOutlet NSTextView *textView;
   IBOutlet PathView *pathView;
   IBOutlet CoordView *coordView;
   
   JsonDocument *document;   
   
   bool syncingNodeAndTextSel;
   bool inhibitHistoryRecord;
   
   NSRange lastSelection;
   
   TextEditorGutterView *gutterView;

   JsonFileListenerObjCBridge *docListenerBridge;
   
   vector<TextCoordinate> backNavs;
   vector<TextCoordinate> forwardNavs;
   int lastNavGroup;
}

- (void)awakeFromNib;
- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context;
- (void)textViewDidChangeSelection:(NSNotification *)aNotification;

- (void)pathView:(PathView *)aPathView didClickOnPathComponentIndex:(int)aIdx;
- (void)pathView:(PathView *)aPathView preparePopupMenu:(NSMenu**)aMenu forIndex:(int)aIdx;

- (void)pathMenuItemSelected:(NSMenuItem*)aItem;
- (IBAction) markerListSelectionChanged:(id)aSender;

-(void)notifyJsonTextSpliced:(json::JsonFile*)aSender from:(TextCoordinate)aOldOffset length:(TextLength)aOldLength newLength:(TextLength)aNewLength;

- (void)refreshSelectionFromTextView;

-(BOOL)canNavigateBack;
- (IBAction)navigateBack:(id)aSender;
-(BOOL)canNavigateForward;
- (IBAction)navigateForward:(id)aSender;
- (void)recordNavigationPointBeforeNavGroup:(int)aGroup;

- (IBAction) toggleBookmark:(id)aSender;
- (IBAction) nextBookmark:(id)aSender;
- (IBAction) prevBookmark:(id)aSender;

-(BOOL)canGoNextSibling;
-(IBAction)goNextSibling:(id)aSender;
-(BOOL)canGoPrevSibling;
-(IBAction)goPrevSibling:(id)aSender;
-(BOOL)canGoParentNode;
-(IBAction)goParentNode:(id)aSender;
-(BOOL)canGoFirstChild;
-(IBAction)goFirstChild:(id)aSender;

-(BOOL)canCopyPath;
-(IBAction)copyPath:(id)aSender;

- (int)lineNumberForCharacterIndex:(NSUInteger)aIdx;
- (UInt32)markersForLine:(int)aLine;
- (void)connectGutterView:(id)aGutterView;
- (NSUInteger)characterIndexForFirstCharOfLine:(int)aIdx;

-(NSString*)currentPathAsJsonQuery;

- (void)_bookmarksChanged:(id)aUserData;

@end
