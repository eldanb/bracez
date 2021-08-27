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

@interface JsonPathSearchController () {
    NSString *_searchPath;
}

@end

@implementation JsonPathSearchController

-(void)loadDefaults {
    BracezPreferences *prefs = [BracezPreferences sharedPreferences];
    JSONPathInput.font = prefs.editorFont;
}

-(void)startJsonPathQuery:(NSString*)query {
    if(query) {
        self.searchPath = query;
    }
    
    [JSONPathInput becomeFirstResponder];
}

- (void)showErrorResult:(NSString*)error forRange:(NSRange)range {
    [JSONPathInput markErrorRange:range];
    
    JSONPathQueryStatus.textColor = [NSColor redColor];
    JSONPathQueryStatus.font = [NSFont monospacedSystemFontOfSize:[NSFont smallSystemFontSize] weight:NSFontWeightRegular];
    JSONPathQueryStatus.stringValue = error;
}

-(void)syntaxEditingFieldChanged:(SyntaxEditingField *)sender {
    [self executeJsonPath:sender];
}

- (IBAction)executeJsonPath:(id)sender {
    if(!_searchPath.length) {
        return;
    }
    
    [JSONPathHistoryFavorites accumulateHistory];
    
    [JSONPathInput clearErrorRanges];

    JsonPathResultNodeList nodeList;
    try {
        JsonPathExpression expr = JsonPathExpression::compile(_searchPath.UTF8String);
        JsonPathExpressionOptions opts;
        opts.fuzzy = self.fuzzySearch;
        
        nodeList = expr.execute(document.rootNode.proxiedElement, &opts);
        
        JSONPathQueryStatus.textColor = [NSColor blackColor];
        JSONPathQueryStatus.font = [NSFont systemFontOfSize:[NSFont smallSystemFontSize]];
        JSONPathQueryStatus.stringValue = [NSString stringWithFormat:@"%ld results", nodeList.size()];
    } catch(const parse_error &e) {
        [self showErrorResult:[NSString stringWithFormat:@"Invalid JSON path syntax: %s; expected: %s",
                               e._what.c_str(), e._expecting.c_str()]
                     forRange:NSMakeRange(
                                          std::min(e._offset_start, JSONPathInput.textStorage.length-1),
                                          std::max(e._len, (size_t)1))];
    } catch(const std::exception &e) {
        [self showErrorResult:[NSString stringWithFormat:@"Invalid JSON path syntax: %s", e.what()]
                     forRange:NSMakeRange(0, JSONPathInput.textStorage.length)];
    }
    
    NSMutableArray *nodesArray = [NSMutableArray arrayWithCapacity:nodeList.size()];
    std::for_each(nodeList.begin(), nodeList.end(),
                  [nodesArray, self](json::Node* node) {
        JsonCocoaNode *nodeForResult = [JsonCocoaNode nodeForElement:node withName:@"Name"];
        JsonMarker *markerForResult = [JsonMarker markerForNode:nodeForResult withParentDoc:document];
        [nodesArray addObject:markerForResult];
    });
    
    [JSONPathResultsController setContent:nodesArray];
}

-(void)setSearchPath:(NSString*)searchPath {
    [self willChangeValueForKey:@"searchPath"];
    _searchPath = searchPath;
    [self didChangeValueForKey:@"searchPath"];
    
    if(self.continuousSearch) {
        [self executeJsonPath:self];
    }
}

-(NSString*)searchPath {
    return _searchPath;
}


@end
