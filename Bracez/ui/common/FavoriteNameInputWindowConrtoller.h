//
//  FavoriteNameInputWindowConrtoller.h
//  Bracez
//
//  Created by Eldan Ben Haim on 30/07/2021.
//

#import <Cocoa/Cocoa.h>

NS_ASSUME_NONNULL_BEGIN

@interface FavoriteNameInputWindowConrtoller : NSWindowController

-(instancetype)initWithInitialName:(NSString*)name
                     existingNames:(NSArray<NSString*>*)names;

@property NSString *inputName;

@end

NS_ASSUME_NONNULL_END
