//
//  BracezPreferences.h
//  Bracez
//
//  Created by Eldan Ben Haim on 19/02/2021.
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface BracezPreferences : NSObject

-(BOOL)gutterMasterSwitch;
-(BOOL)gutterLineNumbers;
-(NSFont*)editorFont;

+(BracezPreferences*)sharedPreferences;

@end

NS_ASSUME_NONNULL_END
