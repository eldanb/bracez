//
//  JsonPathSearchController.h
//  Bracez
//
//  Created by Eldan Ben Haim on 19/02/2021.
//

#import <Foundation/Foundation.h>

@class JsonDocument;

NS_ASSUME_NONNULL_BEGIN

@interface JsonPathSearchController : NSObject {
    IBOutlet JsonDocument* document;
    
    IBOutlet NSTextField *JSONPathInput;
    IBOutlet NSArrayController *JSONPathResultsController;
    IBOutlet NSTextField *JSONPathQueryStatus;
}

-(void)startJsonPathQuery:(NSString*)query;
-(void)loadDefaults;

@property BOOL continuousSearch;
@property BOOL fuzzySearch;

-(void)setSearchPath:(NSString*)searchPath;
-(NSString*)searchPath;
@end

NS_ASSUME_NONNULL_END
