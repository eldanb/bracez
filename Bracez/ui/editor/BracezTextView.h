//
//  BracezTextView.h
//  Bracez
//
//  Created by Eldan Ben Haim on 28/02/2021.
//

#import <Cocoa/Cocoa.h>

NS_ASSUME_NONNULL_BEGIN

@protocol BracezTextViewDelegate

-(void)bracezTextView:(id)sender
         forNewLineAt:(NSUInteger)where
        suggestIndent:(NSUInteger*)indent;

-(void)bracezTextView:(id)sender
      forCloseParenAt:(NSUInteger)where
        suggestIndent:(NSUInteger*)indent
         getLineStart:(NSUInteger*)lineStart;

@end


@interface BracezTextView : NSTextView

@property IBOutlet id<BracezTextViewDelegate> btvDelegate;

@end

NS_ASSUME_NONNULL_END
