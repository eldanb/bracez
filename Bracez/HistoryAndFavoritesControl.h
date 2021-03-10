//
//  HistoryAndFavoritesConrol.h
//  Bracez
//
//  Created by Eldan Ben Haim on 23/02/2021.
//

#import <Cocoa/Cocoa.h>

NS_ASSUME_NONNULL_BEGIN

@interface HistoryAndFavoritesControl : NSView

-(void)accumulateHistory;

@property NSString *historyListName;
@property IBOutlet NSTextField *boundField;
@property NSArray *favoritesList;

@end

NS_ASSUME_NONNULL_END
