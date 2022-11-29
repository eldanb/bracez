//
//  JsonPathSearchController.m
//  Bracez
//
//  Created by Eldan Ben Haim on 19/02/2021.
//

#import "JsonPathSearchController.h"
#import "BracezPreferences.h"
#import "JsonCocoaNode.h"
#import "JsonMarker.h"
#import "JsonDocument.h"
#import "stopwatch.h"
#import "HistoryAndFavoritesControl.h"
#include "JsonPathExpressionCompiler.hpp"

enum JsonReplaceMode {
    replaceAll,
    replaceNext
};

@interface JsonPathSearchController () {
    NSString *_searchPath;
}

@end

@implementation JsonPathSearchController

-(void)loadDefaults {
    BracezPreferences *prefs = [BracezPreferences sharedPreferences];
    JSONPathInput.font = prefs.editorFont;
    replaceJSONPathExpressionInput.font = prefs.editorFont;
    
    //testjsonpathexpressionparser();

}

-(void)startJsonPathQuery:(NSString*)query {
    if(query) {
        self.searchPath = query;
    }
    
    [JSONPathInput becomeFirstResponder];
}

- (void)showErrorResult:(NSString*)error
               forRange:(NSRange)range
               inEditor:(SyntaxEditingField*)editor {
    [editor markErrorRange:range withErrorMessage:error];
    
    JSONPathQueryStatus.textColor = [NSColor redColor];
    JSONPathQueryStatus.font = [NSFont monospacedSystemFontOfSize:[NSFont smallSystemFontSize] weight:NSFontWeightRegular];
    JSONPathQueryStatus.stringValue = error;
}

-(void)syntaxEditingFieldChanged:(SyntaxEditingField *)sender {
    [self executeJsonPath:sender];
}

-(BOOL)compileExpressions {
    
    return YES;
}

-(JsonPathExpression)compileExpressionInEditor:(SyntaxEditingField*)jsonPathEditor
                                        ofType:(CompiledExpressionType)expType {
    
    [jsonPathEditor clearErrorRanges];
    
    NSString *expression = jsonPathEditor.textStorage.string;
    try {
        if(!expression.length) {
            throw std::runtime_error("empty expression");
        }
        return JsonPathExpression::compile(expression.UTF8String, expType);
    } catch(const parse_error &e) {
        [self showErrorResult:[NSString stringWithFormat:@"Invalid JSON path syntax: %s; expected: %s",
                               e._what.c_str(), e._expecting.c_str()]
                     forRange:NSMakeRange(
                                          std::min(e._offset_start, jsonPathEditor.textStorage.length-1),
                                          std::max(e._len, (size_t)1))
                     inEditor:jsonPathEditor];
        throw e;
    } catch(const std::exception &e) {
        [self showErrorResult:[NSString stringWithFormat:@"Invalid JSON path syntax: %s", e.what()]
                     forRange:NSMakeRange(0, jsonPathEditor.textStorage.length)
                     inEditor:jsonPathEditor];
        throw e;
    }
}

-(JsonPathResultNodeList)refreshSearchResults {
    // TODO refresh on cursor change
    JsonPathResultNodeList searchResultList;
    
    JsonPathExpression searchExpr =
        [self compileExpressionInEditor:JSONPathInput
                                 ofType:CompiledExpressionType::jsonPath];
    
    JsonPathExpressionOptions opts;
    opts.fuzzy = self.fuzzySearch;
    
    Node *cursorNode = selectionController.currentSelectedNode.proxiedElement;
    searchResultList = searchExpr.execute(document.rootNode.proxiedElement,
                                          &opts, NULL, cursorNode).nodeList;

    JSONPathQueryStatus.textColor = [NSColor labelColor];
    JSONPathQueryStatus.font = [NSFont systemFontOfSize:[NSFont smallSystemFontSize]];
    JSONPathQueryStatus.stringValue = [NSString stringWithFormat:@"%ld results", searchResultList.size()];
        
    searchResultList.sort([](Node *l, Node *r) { return l->getAbsTextRange().start < r->getAbsTextRange().start; });
    
    NSMutableArray *nodesArray = [NSMutableArray arrayWithCapacity:searchResultList.size()];
    std::for_each(searchResultList.begin(), searchResultList.end(),
                  [nodesArray, self](json::Node* node) {
        JsonCocoaNode *nodeForResult = [JsonCocoaNode nodeForElement:node withName:@"Name"];
        JsonMarker *markerForResult = [JsonMarker markerForNode:nodeForResult withParentDoc:document];
        [nodesArray addObject:markerForResult];
    });
    
    [JSONPathResultsController setContent:nodesArray];
    
    return searchResultList;
}

- (IBAction)executeJsonPath:(id)sender {
    if(!_searchPath.length) {
        return;
    }
    
    [JSONPathHistoryFavorites accumulateHistory];
    
    try {
        [self refreshSearchResults];
    } catch(...) {
        // refreshSearchResults already updated the error
    }
}

-(void)setSearchPath:(NSString*)searchPath {
    [self willChangeValueForKey:@"searchPath"];
    _searchPath = searchPath;
    [self didChangeValueForKey:@"searchPath"];
    
    if(self.continuousSearch) {
        [self executeJsonPath:self];
    }
}

-(IBAction)onReplace:(id)sender {
    [self performReplacementWithMode:JsonReplaceMode::replaceNext];
}


-(IBAction)onReplaceAll:(id)sender {
    [self performReplacementWithMode:JsonReplaceMode::replaceAll];
}

-(Node *)performReplacementWithMode:(JsonReplaceMode)replaceMode {
    
    // Refresh and get current search results and replace expression
    // If failed to reresh search result list no point in moving forward.
    // If failed to compile replacement expression -- no point in moving forward
    JsonPathResultNodeList searchResultList;
    JsonPathExpression repExpr;
    try {
        searchResultList = [self refreshSearchResults];
        repExpr =
            [self compileExpressionInEditor:replaceJSONPathExpressionInput
                                     ofType:CompiledExpressionType::jsonPathExpression];
    } catch(...) {
        return NULL;
    }

    // Obtain current text selection
    NSRange cocoaCurrentTextSelection = selectionController.textSelection;
    TextCoordinate currentCoordinate((unsigned long) cocoaCurrentTextSelection.location);
    json::TextRange currentSelectedRange(currentCoordinate, currentCoordinate+cocoaCurrentTextSelection.length);

    // First node to replace
    auto replaceNodeIter = find_if(searchResultList.begin(),
                               searchResultList.end(),
                               [currentCoordinate](Node *node) {
        return node->getAbsTextRange().start >= currentCoordinate;
    });
    
    if(replaceNodeIter == searchResultList.end()) {
        return NULL;
    }

    // If replacing one --- we need to make sure that the
    // replaced node is selected.
    if(replaceMode == JsonReplaceMode::replaceNext) {
        json::TextRange rangeToReplace = (*replaceNodeIter)->getAbsTextRange();
        if(rangeToReplace != currentSelectedRange) {
            [selectionController selectTextRange:NSMakeRange(rangeToReplace.start.getAddress(),
                                                             rangeToReplace.length())];
            return NULL;
        }
    }
    
    // If replacing all, we need to clean nodes that
    // are contained within nodes previously listed;
    // otherwise, when we run the replace on the parent node,
    // we will hold a dangling references to the children (since they're
    // no deleted).
    if(replaceMode == JsonReplaceMode::replaceAll) {
        [self removeSuccessiveContainedNodesFromList:searchResultList startingFrom:replaceNodeIter];
    }
    
    document.jsonFile->beginDeferNotifications();
    Node *replacementResultNode = NULL;
    do {
        replacementResultNode = [self performReplacementForNode:*replaceNodeIter
                                                 withExpression:repExpr];
        replaceNodeIter++;
    } while(replaceNodeIter != searchResultList.end() &&
            replaceMode == JsonReplaceMode::replaceAll);
    document.jsonFile->endDeferNotifications();
            
    if(replaceMode == JsonReplaceMode::replaceNext) {
        if(replaceNodeIter != searchResultList.end()) {
            json::TextRange nextNodeRange = (*replaceNodeIter)->getAbsTextRange();
            NSRange cocoaRange = NSMakeRange(nextNodeRange.start.getAddress(),
                                             nextNodeRange.length());
            [selectionController selectTextRange:cocoaRange];
        }
    }
    
    return replacementResultNode;
}


-(Node*)performReplacementForNode:(Node*)replacedNode withExpression:(JsonPathExpression&)repExpr {
    Node* replacementResultNode;
    
    int replacementIndex = replacedNode->getParent()->getIndexOfChild(replacedNode);
    
    JsonPathExpressionOptions opts;
    
    JsonPathExpressionNodeEvalResult replacementResult;
    replacementResult = repExpr.execute(document.rootNode.proxiedElement, &opts, replacedNode);
    JsonPathResultNodeList &replacementResultNl = replacementResult.nodeList;

    if(replacementResultNl.size()) {
        replacementResultNode = replacementResultNl.front()->clone();
    } else {
        replacementResultNode = new NullNode();
    }
    
    replacedNode->getParent()->setChildAt(replacementIndex, replacementResultNode);
    
    return replacementResultNode;
}

-(void)removeSuccessiveContainedNodesFromList:(JsonPathResultNodeList&)list
                                 startingFrom:(JsonPathResultNodeList::iterator)startIter {
    auto nodeFilterIter = startIter;
    while(nodeFilterIter != list.end()) {
        json::TextRange curNodeRange = (*nodeFilterIter)->getAbsTextRange();
        
        auto nextFilterIter = nodeFilterIter; nextFilterIter++;
        
        while(nextFilterIter != list.end() &&
              (*nextFilterIter)->getAbsTextRange().start < curNodeRange.end) {
            list.erase(nextFilterIter);
            nextFilterIter = nodeFilterIter; nextFilterIter++;
        }

        nodeFilterIter++;
    }
}

-(NSString*)searchPath {
    return _searchPath;
}


@end
