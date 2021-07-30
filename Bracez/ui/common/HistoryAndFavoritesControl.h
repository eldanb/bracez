//
//  HistoryAndFavoritesConrol.h
//  Bracez
//
//  Created by Eldan Ben Haim on 23/02/2021.
//

#import <Cocoa/Cocoa.h>

NS_ASSUME_NONNULL_BEGIN

@class HistoryAndFavoritesControl;


@protocol HistoryAndFavoritesControlDelegate

-(NSString*)hfControl:(HistoryAndFavoritesControl*)sender wantsNameForFavoriteItem:(id)favoriteItem;

-(void)hfControl:(HistoryAndFavoritesControl*)sender wantsNewFavoriteObjectWithCompletion:(void(^)(id __nullable favObject, NSError * __nullable error))completionHandler;

-(void)hfControl:(HistoryAndFavoritesControl*)sender recallItem:(id)recalledItem;

//-(HistoryAndFavoritesControlDelegateCapabilities)capabilitiesForHfControl:(HistoryAndFavoritesControl*)sender;


@optional
-(void)hfControl:(HistoryAndFavoritesControl*)sender wantsEditForItem:(id)favoriteItem withCompletion:(void(^)(id favObject, NSError * __nullable error))completionHandler;


@end

@interface HistoryAndFavoritesControl : NSView

-(void)accumulateHistory;
-(id<HistoryAndFavoritesControlDelegate>)effectiveDelegate;

-(NSString*)nameForItem:(id)favoriteItem;

@property NSString *historyListName;
@property BOOL disableHistory;
@property IBOutlet NSTextField *boundField;
@property NSArray *favoritesList;

@property (weak) IBOutlet id<HistoryAndFavoritesControlDelegate> delegate;
@end

NS_ASSUME_NONNULL_END
