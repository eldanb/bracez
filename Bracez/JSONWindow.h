//
//  JSONWindow.h
//
//  Created by Eldan on 7/5/10.
//  Copyright 2010 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@class NodeSelectionController;
@class TextEditorGutterView;

@interface JSONWindow : NSWindow {

   IBOutlet NSTextView *textEditor;
   IBOutlet NSScrollView *textEditorScroll;
   IBOutlet NSTreeController *domController;
   IBOutlet NSOutlineView *treeView;

   IBOutlet NodeSelectionController *selectionController;
    
   TextEditorGutterView *gutterView;
}

-(void)loadDefaults;
-(void)removeJsonNode:(id)aSender;
- (void)forwardInvocation:(NSInvocation *)anInvocation;

-(id)_getActionForwardingTarget:(SEL)aSelector;

-(IBAction)indentSelectionAction:(id)sender;

@end
