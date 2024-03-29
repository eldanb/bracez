//
//  ProjectionDefinitionFavoritesAdapter.m
//  Bracez
//
//  Created by Eldan Ben Haim on 30/07/2021.
//

#import "ProjectionDefinitionFavoritesAdapter.h"
#import "ProjectionDefinition.h"
#import "ProjectionDefinitionEditor.h"
#import "FavoriteNameInputWindowConrtoller.h"

@implementation ProjectionDefinitionFavoritesAdapter

- (nonnull NSString *)hfControl:(nonnull HistoryAndFavoritesControl *)sender wantsNameForFavoriteItem:(nonnull id)favoriteItem {
    return [(ProjectionDefinition*)favoriteItem projectionName];
}

- (void)hfControl:(nonnull HistoryAndFavoritesControl *)sender wantsNewFavoriteObjectWithCompletion:(nonnull void (^)(id _Nullable, NSError * _Nullable))completionHandler {
    ProjectionDefinition *definitionToSave = [[self.delegate projectionDefintitionForFavorite:self] copy];
    
    NSMutableArray<NSString*> *existingNames = [NSMutableArray arrayWithCapacity:sender.favoritesList.count];
    for(id item in sender.favoritesList) {
        [existingNames addObject:[sender nameForItem:item]];
    }
    
    NSString *suggestedName = definitionToSave.projectionName ? definitionToSave.projectionName : @"Untitled";
    FavoriteNameInputWindowConrtoller *nameInput =
        [[FavoriteNameInputWindowConrtoller alloc] initWithInitialName:suggestedName
                                                         existingNames:existingNames];
    
    [sender.window beginSheet:nameInput.window completionHandler:^(NSModalResponse returnCode) {
        if(returnCode == NSModalResponseOK) {
            ProjectionDefinition *definition = [[self.delegate projectionDefintitionForFavorite:self] copy];
            definition.projectionName = nameInput.inputName;
            completionHandler(definition, nil);
        } else {
            completionHandler(nil, nil);
        }
    }];
}

- (void)hfControl:(HistoryAndFavoritesControl *)sender
 wantsEditForItem:(id)favoriteItem
         inWindow:(NSWindow*)window
   withCompletion:(void (^)(id _Nonnull, NSError * _Nullable))completionHandler {
    ProjectionDefinitionEditor *editor = [[ProjectionDefinitionEditor alloc]
                                          initWithDefinition:favoriteItem previewDocument:nil];
    [window beginSheet:editor.window
            completionHandler:^(NSModalResponse returnCode) {
                completionHandler(editor.editedProjection, nil);
    }];
}

- (void)hfControl:(nonnull HistoryAndFavoritesControl *)sender recallItem:(nonnull id)recalledItem {
    [self.delegate recallProjectionDefinition:recalledItem];
}


@end
