//
//  BracezTextView.m
//  Bracez
//
//  Created by Eldan Ben Haim on 28/02/2021.
//

#import "BracezTextView.h"

@implementation BracezTextView

-(void)handleNewline {
    int indent = 0;

    [self.btvDelegate bracezTextView:self
                        forNewLineAt:self.selectedRange.location
                       suggestIndent:&indent];
    
    NSString *newLineAndIndent = [@"\n" stringByPaddingToLength:(indent+1) withString:@" " startingAtIndex:0];

    [self.textStorage replaceCharactersInRange:self.selectedRange withString:newLineAndIndent];
    [self scrollRangeToVisible:NSMakeRange(self.selectedRange.location, newLineAndIndent.length)];
}

-(void)handleEndContainerWith:(NSString*)terminator {
    int indent = 0;
    int lineStart;

    [self.btvDelegate bracezTextView:self
                     forCloseParenAt:self.selectedRange.location
                       suggestIndent:&indent
                        getLineStart:&lineStart];
    
    NSString *newLineAndIndent = [[@"" stringByPaddingToLength:(indent)
                                                    withString:@" "
                                               startingAtIndex:0]
                                  stringByAppendingString:terminator];
    
    NSRange rangeFromLineStart = NSMakeRange(lineStart+1,
                                             self.selectedRange.location + self.selectedRange.length - (lineStart+1));

    if([self.textStorage.string rangeOfCharacterFromSet:[[NSCharacterSet whitespaceCharacterSet] invertedSet]
                                                options:0
                                                  range:rangeFromLineStart].location == NSNotFound) {
        [[self textStorage] replaceCharactersInRange:rangeFromLineStart
                                          withString:newLineAndIndent];
        [self scrollRangeToVisible:NSMakeRange(self.selectedRange.location, newLineAndIndent.length)];
    } else {
        [[self textStorage] replaceCharactersInRange:self.selectedRange
                                          withString:terminator];
        [self scrollRangeToVisible:self.selectedRange];
    }
}

-(void)keyDown:(NSEvent *)event {
    NSString *chars = [event charactersIgnoringModifiers];
    if([chars isEqualToString:@"\r"]) {
        [self handleNewline];
    } else
    if([chars isEqualToString:@"}"] ||
       [chars isEqualToString:@"]"]) {
        [self handleEndContainerWith:chars];
    }  else {
        [super keyDown:event];
    }
}

@end
