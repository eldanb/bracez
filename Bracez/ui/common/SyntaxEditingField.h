//
//  SyntaxEditingField.h
//  Bracez
//
//  Created by Eldan Ben Haim on 27/08/2021.
//

#import <Cocoa/Cocoa.h>

NS_ASSUME_NONNULL_BEGIN

@class SyntaxEditingField;

@protocol SyntaxEditingFieldDelegate

-(void)syntaxEditingFieldChanged:(SyntaxEditingField*)sender;

@end


@interface SyntaxEditingField : NSTextView

-(void)markErrorRange:(NSRange)errorRange;
-(void)clearErrorRanges;

@property (weak) IBOutlet id<SyntaxEditingFieldDelegate> syntaxEditingFieldDelegate;

@end

NS_ASSUME_NONNULL_END
