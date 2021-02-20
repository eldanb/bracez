//
//  BracezPreferences.h
//  Bracez
//
//  Created by Eldan Ben Haim on 19/02/2021.
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface BracezPreferences : NSObject

-(void)setGutterMasterSwitch:(BOOL)value;
-(BOOL)gutterMasterSwitch;

-(void)setGutterLineNumbers:(BOOL)value;
-(BOOL)gutterLineNumbers;

-(void)setEnableTreeViewSyntaxColoring:(BOOL)value;
-(BOOL)enableTreeViewSyntaxColoring;

-(void)setEditorFont:(NSFont*)value;
-(NSFont*)editorFont;

-(void)setEditorColorDefault:(NSColor*)value;
-(NSColor*)editorColorDefault;

-(void)setEditorColorString:(NSColor*)value;
-(NSColor*)editorColorString;

-(void)setEditorColorKey:(NSColor*)value;
-(NSColor*)editorColorKey;

-(void)setEditorColorKeyword:(NSColor*)value;
-(NSColor*)editorColorKeyword;

-(void)setEditorColorNumber:(NSColor*)value;
-(NSColor*)editorColorNumber;

+(BracezPreferences*)sharedPreferences;

@end

NS_ASSUME_NONNULL_END
