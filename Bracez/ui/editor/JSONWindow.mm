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
#import "HistoryAndFavoritesControl.h"
#import "BracezTextView.h"

#import "ProjectionTableController.h"
#import "ProjectionDefinitionEditor.h"

#include <UniformTypeIdentifiers/UniformTypeIdentifiers.h>

#include <algorithm>

extern "C" {
#include "jq.h"
}

@interface JSONWindow () <BracezTextViewDelegate> {
    JsonTreeDataSource *_treeDataSource;
    TextEditorGutterView *gutterView;
    IBOutlet GuiModeControl *guiModeControl;
    IBOutlet ProjectionTableController *projectionTableController;
    IBOutlet HistoryAndFavoritesControl *projectionDefinitionsHistory;
    
    BOOL hasValidJqResult;
}
@end

@implementation JSONWindow

+(void)initialize
{
    [super initialize];
    [NSValueTransformer setValueTransformer:[NodeTypeToColorTransformer sharedInstance] forName:@"treeViewNodeColors"];
}

-(void)dealloc {
    NSLog(@"DEALALOC");
}

-(void)awakeFromNib
{
    [super awakeFromNib];
    
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
    
    _treeDataSource = [[JsonTreeDataSource alloc] initWithWindow:self];
    [treeView setDataSource:_treeDataSource];
    [treeView registerForDraggedTypes:[NSArray arrayWithObject:@"JsonNode"]];
    [treeView setDraggingSourceOperationMask:NSDragOperationEvery forLocal:YES];
    
    [projectionTable registerNib:[[NSNib alloc] initWithNibNamed:@"ProjectionTableCell" bundle:nil] forIdentifier:@"projectionCell"];

    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(preferencesChanged:) name:BracezPreferencesChangedNotification object:nil];

    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(semanticModelChanged:) name:JsonDocumentSemanticModelUpdatedNotification
                                               object:self.document];

    [guiModeControl addObserver:self
                     forKeyPath:@"showProjectionView"
                        options:0
                        context:nil];
        
    [self loadPreferences];
}


-(void)preferencesChanged:(NSNotification*)aNotification
{
    [self loadPreferences];
}

-(void)loadPreferences
{
    BracezPreferences *prefs = [BracezPreferences sharedPreferences];

    [textEditorScroll setRulersVisible:prefs.gutterMasterSwitch];
    [gutterView setShowLineNumbers:prefs.gutterLineNumbers];
    treeView.needsDisplay = YES;
            
    jqQueryInput.font = prefs.editorFont;
    jqQueryResult.font = prefs.editorFont;

    [jsonPathSearchController loadDefaults];
}

-(void)removeJsonNode:(id)aSender
{    
    JsonCocoaNode* selectedNodeParent =  (JsonCocoaNode*)(domController.selectedNodes.firstObject.parentNode.representedObject);
        
    NSIndexPath *selectedNodeIndexPath = domController.selectionIndexPath;
    NSUInteger lastIndexPath = [selectedNodeIndexPath indexAtPosition:selectedNodeIndexPath.length-1];
    
    [selectedNodeParent removeObjectFromChildrenAtIndex:(int)lastIndexPath];

    // Doing domController remove below triggers a bug, probably because during the
    // DOM controller change we reload items which triggers another notification
    // (TreeRemove -> CocoaNode remove -> JsonDoc remove -> notify node changed -> reload node)
    // [domController remove:aSender];
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
    
    TextCoordinate newEndLocation;
    [self.document reindentStartingAt:TextCoordinate(selectionRange.location)
                                  len:selectionRange.length
                suggestNewEndLocation:&newEndLocation];
    dispatch_async(dispatch_get_main_queue(), ^{
        [self->selectionController selectTextRange:NSMakeRange(newEndLocation, 0)];
    });

}

static void onJqCompileError(void *ctxt, jv err) {
    jv formattedError = jq_format_error(err);
    const char *errCStr = jv_string_value(formattedError);
    
    NSMutableArray<NSString*> *outArray = (__bridge NSMutableArray<NSString*>*)ctxt;
    [outArray addObject:[NSString stringWithCString:errCStr encoding:NSUTF8StringEncoding]];
}

- (IBAction)saveJqQueryResults:(id)sender {
    NSSavePanel *panel = [NSSavePanel savePanel];
    [panel setAllowedContentTypes:@[UTTypeText, UTTypeJSON]];
    panel.allowsOtherFileTypes = YES;
    if([panel runModal] == NSModalResponseOK) {
        NSData *resultData = [jqQueryResult.textStorage.string dataUsingEncoding:NSUTF8StringEncoding];
        [resultData writeToURL:panel.URL atomically:YES];
    }
}


- (IBAction)jqQueryResultsToNewTab:(id)sender {

    [self executeJqQuery:sender];
    
    if(hasValidJqResult) {
        NSError *err = nil;
        JsonDocument *newDoc = [[JsonDocument alloc] init];

        [newDoc readFromData:[jqQueryResult.textStorage.string dataUsingEncoding:NSUTF8StringEncoding]
                      ofType:@"application/json"
                       error:&err];
         if(err) {
            [NSAlert alertWithError:err];
            return;
        }

        [newDoc reindentStartingAt:(TextCoordinate)0
                               len:newDoc.textStorage.string.length
             suggestNewEndLocation:nil];

        [[NSDocumentController sharedDocumentController] addDocument:newDoc];
        [newDoc makeWindowControllers];
        [newDoc showWindows];
    }
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
        hasValidJqResult = NO;
        
    } else {
        jq_state *state = jq_init();
        
        NSMutableArray<NSString*> *errors = [NSMutableArray arrayWithCapacity:2];
        jq_set_error_cb(state, onJqCompileError, (__bridge void*)errors);
        int compileResult = jq_compile(state, jqQueryInput.stringValue.UTF8String);
        if(!compileResult) {
            if(!errors.count) {
                [errors addObject:@"(Unknown error)"];
            }
            NSString *errorString = [errors componentsJoinedByString:@"\n"];
            [jqQueryResult.textStorage setAttributedString:
                [[NSAttributedString alloc] initWithString:errorString
                                                attributes:@{
                                                    NSForegroundColorAttributeName: [NSColor systemRedColor],
                                                    NSFontAttributeName: jqQueryResult.font
                                                }]];
            
            hasValidJqResult = NO;
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
                                                                               NSForegroundColorAttributeName: [BracezPreferences sharedPreferences].editorColorDefault,
                                                                               NSFontAttributeName: jqQueryResult.font
                                                                           }];
            [jqQueryResult.textStorage setAttributedString:resultAS];
            hasValidJqResult = YES;
        }
        
        jq_teardown(&state);
    }
    
    [jqQueryFavHist accumulateHistory];
}



- (IBAction)jqHelpClicked:(id)sender {
    [[NSWorkspace sharedWorkspace]
     openURL:[NSURL URLWithString:@"https://stedolan.github.io/jq/manual/"]];
}


-(IBAction)showJqEval:(id)sender {
    [guiModeControl setShowJqPanel:@(YES)];
    [guiModeControl setShowNavPanel:YES];
}

-(IBAction)handleProjectionViewItem:(id)sender {
    guiModeControl.showProjectionView = YES;
    [self autoSuggestProjection];
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

- (IBAction)projectJsonPathResults:(id)sender {
    ProjectionDefinition *jqProj = [[ProjectionDefinition alloc] init];
    jqProj.rowSelector = self->jsonPathSearchController.searchPath;
    
    NSArray<ProjectionFieldDefinition*> *fields = [jqProj suggestFieldsBasedOnDocument:self.document];
    for(ProjectionFieldDefinition* def in [fields subarrayWithRange:NSMakeRange(0, std::min((unsigned long)5, fields.count))]) {
        [jqProj addProjection:def];
    }
    
    ProjectionDefinitionEditor *editor = [[ProjectionDefinitionEditor alloc]
                                          initWithDefinition:jqProj
                                          previewDocument:self.document];
    [self beginSheet:editor.window completionHandler:^(NSModalResponse returnCode) {
        if(returnCode == NSModalResponseOK) {
            self->guiModeControl.showProjectionView = YES;
            [self setProjectionDefinition:editor.editedProjection];
        }
    }];
}

-(JsonDocument*)document {
    return (JsonDocument*)self.delegate;
}

- (IBAction)showViewModeMenu:(id)sender {
    [NSMenu popUpContextMenu:ViewModeMenu
                   withEvent:[NSApp currentEvent]
                     forView:((NSToolbarItem*)sender).view];
}

-(void)bracezTextView:(id)sender forNewLineAt:(NSUInteger)where suggestIndent:(NSUInteger *)indent {
    *indent = [self.document suggestIdentForNewLineAt:TextCoordinate(where)];
}

-(void)bracezTextView:(id)sender
      forCloseParenAt:(NSUInteger)where
        suggestIndent:(NSUInteger*)indent
         getLineStart:(NSUInteger*)lineStart {
    TextCoordinate c;
    *indent = [self.document suggestCloserIndentAt:TextCoordinate(where) getLineStart:&c];
    *lineStart = c.getAddress();
}


-(void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary<NSKeyValueChangeKey,id> *)change context:(void *)context {
    if(object == guiModeControl) {
        if([keyPath isEqualTo:@"showProjectionView"]) {
            [self onGuiModeControlProjectVisibleChanged];
        }
    }
}


-(void)onGuiModeControlProjectVisibleChanged {
    if(guiModeControl.showProjectionView) {
        if(!projectionTableController.projectionDefinition) {
            [self autoSuggestProjection];
        }

        [self updateProjectionData];
    }
}

-(void)semanticModelChanged:(NSNotification *)notification {
    NSLog(@"Editor window handling semantic model changed");
    [self updateProjectionData];

    if([notification.userInfo[JsonDocumentSemanticModelUpdatedNotificationKeyReason] isEqualToString:JsonDocumentSemanticModelUpdatedNotificationReasonReparse]) {
        [selectionController refreshSelectionFromTextView];
    }
    
}

-(void)autoSuggestProjection {
    // First try to use one of the favorite projections
    Node *node = self.document.jsonFile->getDom()->getChildAt(0);
    size_t maxNumRows = 0;
    ProjectionDefinition *selectedDef = nil;
    for(ProjectionDefinition *def in projectionDefinitionsHistory.favoritesList) {
        JsonPathResultNodeList ret = [def compiledRowSelector].execute(node).nodeList;
        size_t retSize = ret.size();
        if(retSize > maxNumRows) {
            maxNumRows = retSize;
            selectedDef = def;
        }
    }
    
    // If no favorite projection matches the doc, try to come up with one on your own
    if(!selectedDef) {
        selectedDef = [[ProjectionDefinition suggestPojectionsForDocument:self.document] firstObject];
    }
    
    if(selectedDef) {
        projectionTableController.projectionDefinition = selectedDef;
    }
}

-(void)editProjection {
    ProjectionDefinitionEditor *editor = [[ProjectionDefinitionEditor alloc]
                                          initWithDefinition:projectionTableController.projectionDefinition
                                          previewDocument:self.document];
    [self beginSheet:editor.window completionHandler:^(NSModalResponse returnCode) {
        if(returnCode == NSModalResponseOK) {
            [self setProjectionDefinition:editor.editedProjection];
        }
    }];
}

- (IBAction)editProjectionClicked:(id)sender {
    [self editProjection];
}


-(void)updateProjectionData {
    if(guiModeControl.showProjectionView) {
        [projectionTableController reloadData];
    }
}

-(void)setProjectionDefinition:(ProjectionDefinition*)definition {
    projectionTableController.projectionDefinition = definition;
}

- (nonnull ProjectionDefinition *)projectionDefintitionForFavorite:(nonnull id)sender {
    return projectionTableController.projectionDefinition;
}

- (void)recallProjectionDefinition:(nonnull ProjectionDefinition *)definition {
    [self setProjectionDefinition:definition];
}

-(NodeSelectionController*)selectionController {
    return self->selectionController;
}

@end
