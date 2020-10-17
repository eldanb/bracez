//
//  GuiModeControl.h
//  JsonMockup
//
//  Created by Eldan on 6/5/10.
//  Copyright 2010 Eldan Ben-Haim. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@interface GuiModeControl : NSObject {
   BOOL treeEditor;
   BOOL browserEditor;
   BOOL textEditor;
   
   BOOL problemsList;
   BOOL bookmarksList;
   BOOL findPanel;
   
   
   BOOL verticalSplit;

   IBOutlet NSTabView *visualEditorTabView;
   IBOutlet NSScrollView *textEditorView;
   IBOutlet NSSplitView *editorSplit;
   
   IBOutlet NSTabView *navTabView;
   IBOutlet NSSplitView *navSplit;
}

-(id)init;

-(void)awakeFromNib;

-(void)setShowTree:(NSNumber*)aValue;
-(NSNumber*)showTree;

-(void)setShowBrowser:(NSNumber*)aValue;
-(NSNumber*)showBrowser;

-(void)setShowTextEditor:(NSNumber*)aValue;
-(NSNumber*)showTextEditor;



-(void)setShowProblemsList:(NSNumber*)aValue;
-(NSNumber*)showProblemsList;

-(void)setShowBookmarksList:(NSNumber*)aValue;
-(NSNumber*)showBookmarksList;

-(void)setShowFindPanel:(NSNumber*)aValue;
-(NSNumber*)showFindPanel;


-(void)updateNavPaneWithModeIndex:(int)aIdx value:(BOOL)aVal;


-(void)setVertical:(NSNumber*)aValue;
-(NSNumber*)vertical;

-(void)updateGui;

@end
