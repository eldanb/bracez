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
         forNewLineAt:(int)where
        suggestIndent:(int*)indent;

-(void)bracezTextView:(id)sender
      forCloseParenAt:(int)where
        suggestIndent:(int*)indent
         getLineStart:(int*)lineStart;

@end


@interface BracezTextView : NSTextView

@property IBOutlet id<BracezTextViewDelegate> btvDelegate;

@end

NS_ASSUME_NONNULL_END
