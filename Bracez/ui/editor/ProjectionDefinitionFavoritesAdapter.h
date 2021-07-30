//
//  ProjectionDefinitionFavoritesAdapter.h
//  Bracez
//
//  Created by Eldan Ben Haim on 30/07/2021.
//

#import <Foundation/Foundation.h>
#import "HistoryAndFavoritesControl.h"

NS_ASSUME_NONNULL_BEGIN

@class ProjectionDefinition;

@protocol ProjectionDefinitionFavoritesAdapterDelegate

-(ProjectionDefinition*)projectionDefintitionForFavorite:(id)sender;
-(void)recallProjectionDefinition:(ProjectionDefinition*)definition;

@end

@interface ProjectionDefinitionFavoritesAdapter : NSObject <HistoryAndFavoritesControlDelegate>

@property (weak) IBOutlet id<ProjectionDefinitionFavoritesAdapterDelegate> delegate;

@end

NS_ASSUME_NONNULL_END
