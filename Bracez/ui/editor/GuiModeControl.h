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
   BOOL jqPanel;
   BOOL jsonPathPanel;
   BOOL showNavPanel;

   BOOL verticalSplit;

   IBOutlet __weak NSTabView *visualEditorTabView;
   IBOutlet __weak NSScrollView *textEditorView;
   IBOutlet __weak NSSplitView *editorSplit;
    
   IBOutlet __weak NSTabView *navTabView;
   IBOutlet __weak NSView *navContainer;
   IBOutlet __weak NSSplitView *navSplit;
}

-(id)init;

-(void)awakeFromNib;

-(void)setShowNavPanel:(BOOL)value;
-(BOOL)showNavPanel;

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

-(void)setShowJqPanel:(NSNumber*)aValue;
-(NSNumber*)showJqPanel;

-(void)setShowJsonPathPanel:(NSNumber*)aValue;
-(NSNumber*)showJsonPathPanel;


-(void)updateNavPaneWithModeIndex:(int)aIdx value:(BOOL)aVal;


-(void)setVertical:(NSNumber*)aValue;
-(NSNumber*)vertical;

-(void)updateGui;

@end
