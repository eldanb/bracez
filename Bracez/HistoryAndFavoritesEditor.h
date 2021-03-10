//
//  HistoryAndFavoritesEditor.h
//  Bracez
//
//  Created by Eldan Ben Haim on 26/02/2021.
//

#import <Cocoa/Cocoa.h>

NS_ASSUME_NONNULL_BEGIN

@class HistoryAndFavoritesControl;

@interface HistoryAndFavoritesEditor : NSWindowController

-(instancetype)initWithHistoryAndFavorites:(HistoryAndFavoritesControl*)hisotryAndFavorites;

@property NSIndexSet* selectedFavs;
@property (readonly) BOOL hasFavSelection;

@property (weak) IBOutlet NSTableView *favTable;

@end

NS_ASSUME_NONNULL_END
