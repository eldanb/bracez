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
        textEditor    = YES;
        treeEditor    = YES;
        verticalSplit = NO;
        bookmarksList = YES;
        projectionViewShown = NO;
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
    [self willChangeValueForKey:@"showJqPanel"];
    [self willChangeValueForKey:@"showJsonPathPanel"];
    
    if(aVal) {
        problemsList = (aIdx==0);
        bookmarksList = (aIdx==1);
        jqPanel = (aIdx==2);
        jsonPathPanel = (aIdx==3);
    }
    
    [self didChangeValueForKey:@"showProblemsList"];
    [self didChangeValueForKey:@"showBookmarksList"];
    [self didChangeValueForKey:@"showJqPanel"];
    [self didChangeValueForKey:@"showJsonPathPanel"];
    
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


-(void)setShowJqPanel:(NSNumber*)aValue
{
    [self updateNavPaneWithModeIndex:2 value:[aValue boolValue]];
}

-(NSNumber*)showJqPanel
{
    return [NSNumber numberWithBool:jqPanel];
}

-(void)setShowJsonPathPanel:(NSNumber*)aValue {
    [self updateNavPaneWithModeIndex:3 value:[aValue boolValue]];
}

-(NSNumber*)showJsonPathPanel {
    return [NSNumber numberWithBool:jsonPathPanel];
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

-(void)setShowNavPanel:(BOOL)value {
    showNavPanel = value;
    [self updateGui];
}

-(BOOL)showNavPanel {
    return showNavPanel;
}

-(void)setShowProjectionView:(BOOL)value {
    projectionViewShown = value;
    [self updateGui];
}

-(BOOL)showProjectionView {
    return projectionViewShown;
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
    BOOL lProjectionShown = projectionViewSplit.arrangedSubviews.count > 1 && projectionViewSplit.arrangedSubviews[1].frame.size.height > 0;
    
    
    // Hide components as needed
    bool lHideVisual = (!treeEditor && !browserEditor);

    // Select visual view
    if(!lHideVisual)
    {
        [visualEditorTabView selectTabViewItemWithIdentifier:(treeEditor?@"tree":@"browser")];
    }

    CGFloat editorSplitSize = (!verticalSplit ? [editorSplit frame].size.width:[editorSplit frame].size.height);
    
    // Select vertical/horizontal split
    if(verticalSplit != ![editorSplit isVertical])
    {
        [editorSplit setVertical:!verticalSplit];
        
        // If need to hide text editor, make sure it's hidden
        if(!textEditor)
        {
            [editorSplit setPosition:verticalSplit?[editorSplit frame].size.height:[editorSplit frame].size.width ofDividerAtIndex:0];
        } else if(lHideVisual) {
            [editorSplit setPosition:-[editorSplit dividerThickness] ofDividerAtIndex:0];
        } else {
            [editorSplit setPosition:editorSplitSize/3 ofDividerAtIndex:0];
        }
        
        [editorSplit adjustSubviews];
    } else {
        // Move splitter pane if necessary
        if(lHideVisual && lVisualShown)
        {
            animateSplitterPane(editorSplit, -[editorSplit dividerThickness]);
        } else if(!textEditor && lTextEditorShown ) {
            animateSplitterPane(editorSplit, editorSplitSize);
        } else if(textEditor && !lHideVisual && !(lTextEditorShown && lVisualShown))
        {
            animateSplitterPane(editorSplit, editorSplitSize/3);
        }
    }
    
    // Update nav pane: first the tab
    if(problemsList)
    {
        [navTabView selectTabViewItemWithIdentifier:@"problemsList"];
    } else if(bookmarksList)
    {
        [navTabView selectTabViewItemWithIdentifier:@"bookmarksList"];
    } else if(jqPanel)
    {
        [navTabView selectTabViewItemWithIdentifier:@"jqPanel"];
    } else if(jsonPathPanel)
    {
        [navTabView selectTabViewItemWithIdentifier:@"jsonPathPanel"];
    }

    // Update nav pain view
    if((!navContainer.hidden) != showNavPanel)
    {
        if(showNavPanel && [navSplit.arrangedSubviews indexOfObject:navContainer]==NSNotFound) {
            [navSplit addArrangedSubview:navContainer];
            navContainer.hidden = NO;
        }
        CGFloat frameWidth = [navSplit frame].size.width;
        animateSplitterPane(navSplit, frameWidth - (showNavPanel?frameWidth/3:-[navSplit dividerThickness]));
        if(!showNavPanel) {
            if([navSplit.arrangedSubviews indexOfObject:navContainer]!=NSNotFound) {
                [navSplit removeArrangedSubview:navContainer];
            }
            
            navContainer.hidden = YES;
        }
    }
    
    
    // Update projection pane
    if(projectionViewShown != lProjectionShown) {
        projectionViewSplit.arrangesAllSubviews = NO;
        if(projectionViewShown) {
            if([projectionViewSplit.arrangedSubviews indexOfObject:projectionView] == NSNotFound) {
                [projectionViewSplit addArrangedSubview:projectionView];
                projectionView.hidden = NO;
            }
            animateSplitterPane(projectionViewSplit, projectionViewSplit.frame.size.height * 2 / 3);
        } else {
            animateSplitterPane(projectionViewSplit, projectionViewSplit.frame.size.height + 5);
            NSInteger lSubviewIndex = [projectionViewSplit.arrangedSubviews indexOfObject:projectionView];
            if(lSubviewIndex != NSNotFound) {
                [projectionViewSplit removeArrangedSubview:projectionView];
            }
            projectionView.hidden = YES;
        }
    }
}


-(BOOL)splitView:(NSSplitView*)aView shouldHideDividerAtIndex:(int)aIdx
{
    if(aView == editorSplit)
    {
        return !textEditor || !(treeEditor || browserEditor);
    } else {
        return (aIdx==0) && showNavPanel;
    }
}

@end
