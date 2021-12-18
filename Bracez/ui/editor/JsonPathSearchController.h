//
//  JsonPathSearchController.h
//  Bracez
//
//  Created by Eldan Ben Haim on 19/02/2021.
//

#import <Foundation/Foundation.h>
#import "SyntaxEditingField.h"
#import "NodeSelectionController.h"

@class JsonDocument;
@class HistoryAndFavoritesControl;

NS_ASSUME_NONNULL_BEGIN

@interface JsonPathSearchController : NSObject <SyntaxEditingFieldDelegate> {
    IBOutlet __weak JsonDocument* document;
    IBOutlet __weak NodeSelectionController *selectionController;
    IBOutlet HistoryAndFavoritesControl *JSONPathHistoryFavorites;
    IBOutlet SyntaxEditingField *JSONPathInput;
    IBOutlet NSArrayController *JSONPathResultsController;
    IBOutlet NSTextField *JSONPathQueryStatus;

    IBOutlet SyntaxEditingField *replaceJSONPathExpressionInput;
}

-(void)startJsonPathQuery:(NSString*)query;
-(void)loadDefaults;

@property BOOL continuousSearch;
@property BOOL fuzzySearch;

@property BOOL replaceEnabled;

-(void)setSearchPath:(NSString*)searchPath;
-(NSString*)searchPath;

-(IBAction)onReplace:(id)sender;
-(IBAction)onReplaceAll:(id)sender;

@property NSString* replaceExpression;
@end

NS_ASSUME_NONNULL_END
