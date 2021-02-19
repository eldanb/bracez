//
//  JSONWindow.m
//
//  Created by Eldan on 7/5/10.
//  Copyright 2010 __MyCompanyName__. All rights reserved.
//

#import "JSONWindow.h"
#import "TextEditorGutterView.h"
#import "JsonTreeDataSource.h"
#import "NodeTypeToColorTransformer.h"
#import "JsonDocument.h"
#import "JsonMarker.h"
#import "GuiModeControl.h"
#import "NodeSelectionController.h"
#import "JsonPathSearchController.h"
#import "BracezPreferences.h"

extern "C" {
#include "extlib/jq/include/jq.h"
}

@interface JSONWindow () {
    JsonTreeDataSource *_treeDataSource;
    TextEditorGutterView *gutterView;
    IBOutlet GuiModeControl *guiModeControl;
}

@end

@implementation JSONWindow

+(void)initialize
{
    [super initialize];
    [NSValueTransformer setValueTransformer:[[NodeTypeToColorTransformer alloc] initWithEnableKey:@"TreeViewSyntaxColoring"] forName:@"treeViewNodeColors"];
}

-(void)awakeFromNib
{
    [textEditor setHorizontallyResizable:YES];
    [textEditor setAutoresizingMask:(NSViewWidthSizable|NSViewHeightSizable)];
    [[textEditor textContainer] setWidthTracksTextView:NO];
    [[textEditor textContainer] setContainerSize:NSMakeSize(MAXFLOAT, MAXFLOAT)];
    textEditor.automaticQuoteSubstitutionEnabled = NO;
    textEditor.enabledTextCheckingTypes = 0;
    [textEditor setMaxSize:NSMakeSize(MAXFLOAT, MAXFLOAT)];
    
    [textEditor.layoutManager replaceTextStorage:self.document.textStorage];
    
    gutterView = [[TextEditorGutterView alloc] initWithScrollView:textEditorScroll];
    [gutterView setModel:(id<GutterViewModel>)selectionController];
    [textEditorScroll setVerticalRulerView:gutterView];
    
    _treeDataSource = [[JsonTreeDataSource alloc] init];
    [treeView setDataSource:_treeDataSource];
    [treeView registerForDraggedTypes:[NSArray arrayWithObject:@"JsonNode"]];
    [treeView setDraggingSourceOperationMask:NSDragOperationEvery forLocal:YES];
    
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(defaultsChanged:) name:NSUserDefaultsDidChangeNotification object:nil];
    
    
    [self loadDefaults];
}


-(void)defaultsChanged:(NSNotification*)aNotification
{
    [self loadDefaults];
}

-(void)loadDefaults
{
    BracezPreferences *prefs = [BracezPreferences sharedPreferences];

    [textEditorScroll setRulersVisible:prefs.gutterMasterSwitch];
    [gutterView setShowLineNumbers:prefs.gutterLineNumbers];
    treeView.needsDisplay = YES;
    
    textEditor.font = prefs.editorFont;
    
    jqQueryInput.font = prefs.editorFont;
    jqQueryResult.font = prefs.editorFont;
    
    [jsonPathSearchController loadDefaults];
}

-(void)removeJsonNode:(id)aSender
{
    [domController remove:aSender];
}


struct ForwardedActionInfo
{
    const char *selectorString;
    enum {
        faiDomController,
        faiSelectionController,
        faiSelf,
        faiTextEdit
    } forwardee;
    SEL entSelector;
    
} ;


struct ForwardedActionInfo glbForwardedActions[]  =  {
    { "toggleBookmark:", ForwardedActionInfo::faiSelectionController, 0  },
    { "nextBookmark:", ForwardedActionInfo::faiSelectionController, 0  },
    { "prevBookmark:", ForwardedActionInfo::faiSelectionController, 0  },
    { "goFirstChild:", ForwardedActionInfo::faiSelectionController, 0  },
    { "goNextSibling:", ForwardedActionInfo::faiSelectionController, 0  },
    { "goPrevSibling:", ForwardedActionInfo::faiSelectionController, 0  },
    { "goParentNode:", ForwardedActionInfo::faiSelectionController, 0  },
    
    { "copyPath:", ForwardedActionInfo::faiSelectionController, 0  },
    
    { "navigateBack:", ForwardedActionInfo::faiSelectionController, 0  },
    { "navigateForward:", ForwardedActionInfo::faiSelectionController, 0  },
    
    { "performFindPanelAction:", ForwardedActionInfo::faiTextEdit, 0  },
    
    { NULL, ForwardedActionInfo::faiSelf, 0 }
};

-(id)_getActionForwardingTarget:(SEL)aSelector
{
    int lIdx=0;
    while(glbForwardedActions[lIdx].selectorString)
    {
        if(!glbForwardedActions[lIdx].entSelector)
        {
            glbForwardedActions[lIdx].entSelector = NSSelectorFromString([NSString stringWithCString:glbForwardedActions[lIdx].selectorString encoding:NSUTF8StringEncoding]);
        }
        
        if(glbForwardedActions[lIdx].entSelector == aSelector)
        {
            break;
        }
        
        lIdx++;
    }
    
    
    switch(glbForwardedActions[lIdx].forwardee)
    {
        case ForwardedActionInfo::faiDomController:
            return domController;
        case ForwardedActionInfo::faiSelectionController:
            return selectionController;
        case ForwardedActionInfo::faiTextEdit:
            return textEditor;
        default:
            return self;
    }
    
    return self;
}

- (BOOL)respondsToSelector:(SEL)aSelector
{
    id lResponder = [self _getActionForwardingTarget:aSelector];
    
    if(lResponder != self)
    {
        BOOL lRet= [lResponder respondsToSelector:aSelector];
        return lRet;
    } else {
        return [super respondsToSelector:aSelector];
    }
}

- (NSMethodSignature *)	methodSignatureForSelector:(SEL) aSelector
{
    id lResponder = [self _getActionForwardingTarget:aSelector];
    
    if(lResponder != self)
    {
        return [lResponder methodSignatureForSelector:aSelector];
    }
    else
    {
        return [super methodSignatureForSelector:aSelector];
    }
}

- (void)forwardInvocation:(NSInvocation *)anInvocation
{
    id lResponder = [self _getActionForwardingTarget:[anInvocation selector]];
    
    if(lResponder != self)
    {
        [anInvocation invokeWithTarget:lResponder];
    }
    else
    {
        [super forwardInvocation:anInvocation];
    }
}

- (BOOL)validateUserInterfaceItem:(id <NSValidatedUserInterfaceItem>)anItem
{
    id lResponder = [self _getActionForwardingTarget:[anItem action]];
    
    if(lResponder != self)
    {
        return [lResponder validateUserInterfaceItem:anItem];
    } else {
        return YES;
    }
}

-(void)indentSelectionAction:(id)sender {
    NSRange selectionRange = textEditor.selectedRange;
    if(selectionRange.length <= 0) {
        selectionRange = NSMakeRange(0, textEditor.textStorage.length);
    }
    
    [self.document reindentStartingAt:selectionRange.location len:selectionRange.length];
}

- (IBAction)executeJqQuery:(id)sender {
    NSString *docText = self.document.textStorage.string;
    
    jv doc = jv_parse(docText.UTF8String);
    if(!jv_is_valid(doc)) {
        [jqQueryResult.textStorage setAttributedString:
            [[NSAttributedString alloc] initWithString:@"Invalid input document."
                                            attributes:@{
                                                NSForegroundColorAttributeName: [NSColor systemRedColor],
                                                NSFontAttributeName: jqQueryResult.font
                                            }]];
    } else {
        jq_state *state = jq_init();
        int compileResult = jq_compile(state, jqQueryInput.stringValue.UTF8String);
        if(!compileResult) {
            [jqQueryResult.textStorage setAttributedString:
                [[NSAttributedString alloc] initWithString:@"Invalid query."
                                                attributes:@{
                                                    NSForegroundColorAttributeName: [NSColor systemRedColor],
                                                    NSFontAttributeName: jqQueryResult.font
                                                }]];
        } else {
            NSMutableString *resultText = [NSMutableString string];
            
            jq_start(state, doc, 0);
            jv result = jq_next(state);
            while(jv_is_valid(result)) {
                jv dumped = jv_dump_string(result, 0);
                const char *str = jv_string_value(dumped);
                
                [resultText appendFormat:@"%s\n", str];
                
                result = jq_next(state);
            }
            
            NSAttributedString *resultAS = [[NSAttributedString alloc] initWithString:resultText
                                                                           attributes:@{
                                                                               NSFontAttributeName: jqQueryResult.font
                                                                           }];
            [jqQueryResult.textStorage setAttributedString:resultAS];
        }
        
        jq_teardown(&state);
    }
}



- (IBAction)jqHelpClicked:(id)sender {
    [[NSWorkspace sharedWorkspace]
     openURL:[NSURL URLWithString:@"https://stedolan.github.io/jq/manual/"]];
}


-(IBAction)showJqEval:(id)sender {
    [guiModeControl setShowJqPanel:@(YES)];
    [guiModeControl setShowNavPanel:YES];
}

- (void)findByJsonPathWithQuery:(NSString*)query {
    [guiModeControl setShowJsonPathPanel:@(YES)];
    [guiModeControl setShowNavPanel:YES];
    [jsonPathSearchController startJsonPathQuery:query]; 
}


- (IBAction)findByJSONPath:(id)sender {
    [self findByJsonPathWithQuery:nil];
}

-(IBAction)findByJSONPathHere:(id)sender {
    [self findByJsonPathWithQuery:selectionController.currentPathAsJsonQuery];
}
-(JsonDocument*)document {
    return (JsonDocument*)self.delegate;
}

- (IBAction)showViewModeMenu:(id)sender {
    [NSMenu popUpContextMenu:ViewModeMenu
                   withEvent:[NSApp currentEvent]
                     forView:((NSToolbarItem*)sender).view];
}

@end
