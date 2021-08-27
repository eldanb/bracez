//
//  SyntaxEditingField.m
//  Bracez
//
//  Created by Eldan Ben Haim on 27/08/2021.
//

#import "SyntaxEditingField.h"

@implementation SyntaxEditingField


-(instancetype)initWithCoder:(NSCoder *)coder {
    self = [super initWithCoder:coder];
    if(self) {
        [self configureTextView];
    }
    return self;
}

-(instancetype)initWithFrame:(NSRect)frameRect {
    self = [super initWithFrame:frameRect];
    if(self) {
        [self configureTextView];
    }
    return self;
}

-(instancetype)initWithFrame:(NSRect)frameRect textContainer:(NSTextContainer *)container {
    self = [super initWithFrame:frameRect textContainer:container];
    if(self) {
        [self configureTextView];
    }
    return self;
}

-(void)markErrorRange:(NSRange)errorRange {
    [self.textStorage addAttributes:@{
        NSUnderlineStyleAttributeName: @(NSUnderlineStyleThick),
        NSUnderlineColorAttributeName: [NSColor redColor]
    }
                              range:errorRange];
}

-(void)configureTextView {
    self.automaticQuoteSubstitutionEnabled = NO;
    self.automaticDashSubstitutionEnabled = NO;
    self.automaticTextReplacementEnabled = NO;
    self.enabledTextCheckingTypes = 0;
}

-(void)clearErrorRanges {
    [self.textStorage removeAttribute:NSUnderlineStyleAttributeName
                                range:NSMakeRange(0, self.textStorage.length)];
}

-(void)keyDown:(NSEvent *)event {
    if([[event charactersIgnoringModifiers] isEqualToString:@"\r"]) {
        if(self.syntaxEditingFieldDelegate) {
            [self.syntaxEditingFieldDelegate syntaxEditingFieldChanged:self];
        }
    } else {
        [super keyDown:event];
    }
}


@end
