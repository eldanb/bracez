//
//  GuiModeControl.m
//  JsonMockup
//
//  Created by Eldan on 6/5/10.
//  Copyright 2010 Eldan Ben-Haim. All rights reserved.
//

#import "GuiModeControl.h"
#import "SplitterAnimation.h"

@implementation GuiModeControl

-(id)init
{
   self = [super init];
   if(self)
   {
      textEditor=YES;
      treeEditor = YES;
      verticalSplit = YES;
   }
   
   return self;
}

-(void)awakeFromNib
{
   [self updateGui];
}

-(void)setShowTree:(NSNumber*)aValue
{
   treeEditor = [aValue boolValue];
   
   if(treeEditor && browserEditor)
   {
      [self setShowBrowser:[NSNumber numberWithBool:NO]];
   } else
   if(!treeEditor && !browserEditor)
   {
      [self setShowTextEditor:[NSNumber numberWithBool:YES]];
   } else
   {
      [self updateGui];
   }
}

-(NSNumber*)showTree
{
   return [NSNumber numberWithBool:treeEditor];
}


-(void)setShowBrowser:(NSNumber*)aValue
{
   browserEditor = [aValue boolValue];
   
   if(treeEditor && browserEditor)
   {
      [self setShowTree:[NSNumber numberWithBool:NO]];
   } else
   if(!treeEditor && !browserEditor)
   {
      [self setShowTextEditor:[NSNumber numberWithBool:YES]];
   } else
   {
      [self updateGui];
   }
}


-(NSNumber*)showBrowser
{
   return [NSNumber numberWithBool:browserEditor];
}


-(void)setShowTextEditor:(NSNumber*)aValue
{
   textEditor = [aValue boolValue];
 
   if(!textEditor && !treeEditor && !browserEditor)
   {
      [self setShowTree:[NSNumber numberWithBool:YES]];
   }
   else 
   {
      [self updateGui];
   }
}

-(NSNumber*)showTextEditor
{
   return [NSNumber numberWithBool:textEditor];
}


-(void)updateNavPaneWithModeIndex:(int)aIdx value:(BOOL)aVal
{
   [self willChangeValueForKey:@"showProblemsList"];
   [self willChangeValueForKey:@"showBookmarksList"];
   [self willChangeValueForKey:@"showFindPanel"];
   
   problemsList = (aIdx==0) * aVal;
   bookmarksList = (aIdx==1) * aVal;
   findPanel = (aIdx==2) * aVal;
   
   [self didChangeValueForKey:@"showProblemsList"];
   [self didChangeValueForKey:@"showBookmarksList"];
   [self didChangeValueForKey:@"showFindPanel"];
   
   [self updateGui];   
}



-(void)setShowBookmarksList:(NSNumber*)aValue
{
   [self updateNavPaneWithModeIndex:1 value:[aValue boolValue]];
}

-(NSNumber*)showBookmarksList
{
   return [NSNumber numberWithBool:bookmarksList];
}


-(void)setShowFindPanel:(NSNumber*)aValue
{
   [self updateNavPaneWithModeIndex:2 value:[aValue boolValue]];
}

-(NSNumber*)showFindPanel
{
   return [NSNumber numberWithBool:findPanel];
}


-(void)setShowProblemsList:(NSNumber*)aValue
{
   [self updateNavPaneWithModeIndex:0 value:[aValue boolValue]];
}

-(NSNumber*)showProblemsList
{
   return [NSNumber numberWithBool:problemsList];
}

-(void)setVertical:(NSNumber*)aValue
{
   verticalSplit = [aValue boolValue];
        
   [self updateGui];
}

-(NSNumber*)vertical
{
   return [NSNumber numberWithBool:verticalSplit];
}


static void animateSplitterPane(NSSplitView *aView, int aSize)
{
   
   NSAnimation *lAnimation = [[SplitterAnimation alloc] initWithTarget:aView divIndex:0 endValue:aSize];

   [lAnimation setAnimationBlockingMode:NSAnimationBlocking];
   [lAnimation setDuration:0.5];
   [lAnimation startAnimation];
   
}

-(void)updateGui
{
   // Get current presentation state
   NSRect lFrame = [visualEditorTabView frame];
   BOOL lVisualShown = (lFrame.size.height * lFrame.size.width) > 0;
   lFrame = [textEditorView frame];
   BOOL lTextEditorShown = (lFrame.size.height * lFrame.size.width) > 0;
   lFrame = [navTabView frame];
   BOOL lNavPaneShown = (lFrame.size.height * lFrame.size.width) > 0;
      
   // Hide components as needed
   bool lHideVisual = (!treeEditor && !browserEditor);

   // Select vertical/horizontal split
   if(verticalSplit != [editorSplit isVertical])
   {
      [editorSplit setVertical:verticalSplit];
      
      // If need to hide text editor, make sure it's hidden
      if(!textEditor)
      {
         [editorSplit setPosition:verticalSplit?[editorSplit frame].size.width:[editorSplit frame].size.height ofDividerAtIndex:0];
      } else if(lHideVisual) {
         [editorSplit setPosition:-[editorSplit dividerThickness] ofDividerAtIndex:0];
      }

      
      [editorSplit adjustSubviews];
   }

   // Select visual view
   if(treeEditor||browserEditor)
   {
      [visualEditorTabView selectTabViewItemWithIdentifier:(treeEditor?@"tree":@"browser")];
   }
   
   // Move splitter pane if necessary
   if(lHideVisual && lVisualShown)
   {
      animateSplitterPane(editorSplit, -[editorSplit dividerThickness]);
   } else if(!textEditor && lTextEditorShown ) {
      animateSplitterPane(editorSplit, verticalSplit?[editorSplit frame].size.width:[editorSplit frame].size.height);
   } else if(textEditor && !lHideVisual && !(lTextEditorShown && lVisualShown))
   {
      animateSplitterPane(editorSplit, (verticalSplit?[editorSplit frame].size.width:[editorSplit frame].size.height)/2);
   }
   
   
   // Update nav pane: first the tab
   if(problemsList)
   {
      [navTabView selectTabViewItemWithIdentifier:@"problemsList"];
   } else 
   if(bookmarksList)
   {
      [navTabView selectTabViewItemWithIdentifier:@"bookmarksList"];     
   } else 
   if(findPanel)
   {
      [navTabView selectTabViewItemWithIdentifier:@"findPanel"];           
   }
  
   bool lNavPaneRequired = problemsList || bookmarksList || findPanel;
   if(lNavPaneShown != lNavPaneRequired)
   {
       CGFloat frameWidth = [navSplit frame].size.width;
       animateSplitterPane(navSplit, frameWidth - (lNavPaneRequired?frameWidth/3:-[navSplit dividerThickness]));
   }
}


-(BOOL)splitView:(NSSplitView*)aView shouldHideDividerAtIndex:(int)aIdx
{
   if(aView == editorSplit)
   {      
      return !textEditor || !(treeEditor || browserEditor);
   } else {
      return (aIdx==0) && !problemsList;
   } 
}



      
@end
