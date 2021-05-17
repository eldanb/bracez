//
//  BracezPreferences.h
//  Bracez
//
//  Created by Eldan Ben Haim on 19/02/2021.
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

enum AppearanceSelection {
    System = 0,
    Light = 1,
    Dark = 2
} ;

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

-(void)setIndentSize:(int)indentSize;
-(int)indentSize;

-(void)setSelectedAppearance:(enum AppearanceSelection)appearance;
-(enum AppearanceSelection)selectedAppearance;

+(BracezPreferences*)sharedPreferences;

@end

extern NSString *BracezPreferencesChangedNotification;

NS_ASSUME_NONNULL_END
