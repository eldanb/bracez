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

- (IBAction)executeJsonPath:(id)sender {
    [JSONPathHistoryFavorites accumulateHistory];
    
    JsonPathResultNodeList nodeList;
    try {
        JsonPathExpression expr = JsonPathExpression::compile(_searchPath.UTF8String);
        JsonPathExpressionOptions opts;
        opts.fuzzy = self.fuzzySearch;
        
        nodeList = expr.execute(document.rootNode.proxiedElement, &opts);
        
        JSONPathQueryStatus.textColor = [NSColor blackColor];
        JSONPathQueryStatus.stringValue = [NSString stringWithFormat:@"%ld results", nodeList.size()];
    } catch(const std::exception &e) {
        JSONPathQueryStatus.textColor = [NSColor redColor];
        JSONPathQueryStatus.stringValue = [NSString stringWithFormat:@"Invalid JSON path syntax: %s", e.what()];
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
