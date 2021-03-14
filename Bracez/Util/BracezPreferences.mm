//
//  BracezPreferences.m
//  Bracez
//
//  Created by Eldan Ben Haim on 19/02/2021.
//

#import "BracezPreferences.h"


static BracezPreferences* __sharedPreferences = NULL;

@interface BracezPreferences () {
    NSUserDefaults *defaults;
    
    int _indentSize;
    NSFont *_editorFont;
}

@end


@implementation BracezPreferences

+(void)initialize {
    // Load defaults for preferences
    NSString *defaultsDictFilename = [[NSBundle mainBundle] pathForResource:@"PreferencesDefaults"
                                                                     ofType:@"plist"];
    NSDictionary *defaultsDict = [NSDictionary dictionaryWithContentsOfFile:defaultsDictFilename];
    [[NSUserDefaults standardUserDefaults] registerDefaults:defaultsDict];
}

-(instancetype)init {
    self = [super init];
    defaults = [NSUserDefaults standardUserDefaults];
    _indentSize = -1;
    
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(defaultsChanged:)
                                                 name:NSUserDefaultsDidChangeNotification
                                               object:nil];
    

    return self;
}

-(void)dealloc {
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}

-(void)defaultsChanged:(id)sender {
    // Invalidate cache
    _indentSize = -1;
    _editorFont = nil;
}

-(void)setGutterMasterSwitch:(BOOL)value {
    [defaults setBool:value forKey:@"GutterMaster"];
}

-(BOOL)gutterMasterSwitch {
    return [defaults boolForKey:@"GutterMaster"];
}

-(void)setGutterLineNumbers:(BOOL)value {
    [defaults setBool:value forKey:@"GutterLineNumbers"];
}

-(BOOL)gutterLineNumbers {
    return [defaults boolForKey:@"GutterLineNumbers"];
}

-(void)setEditorFont:(NSFont*)value {
    _editorFont = value;
    [self setFont:value forKey:@"TextEditorFont"];
}

-(NSFont*)editorFont {
    if(!_editorFont) {
        _editorFont = [self loadFontWithKey:@"TextEditorFont"];
    }
    
    return _editorFont;
}

-(void)setFont:(NSFont*)font forKey:(NSString*)keyName {
    NSError *err = nil;
    NSData *lFontData = [NSKeyedArchiver archivedDataWithRootObject:font
                                 requiringSecureCoding:YES
                                                 error:&err];
    [defaults setValue:lFontData forKey:keyName];
}

-(NSFont*)loadFontWithKey:(NSString*)key {
    NSData *lFontData =[defaults valueForKey:key];
    return [NSKeyedUnarchiver unarchivedObjectOfClass:NSFont.class
                                             fromData:lFontData
                                                error:nil];
}

-(NSColor*)colorWithDefaultKey:(NSString*)defaultKey {
    NSError *err = nil;
    return [NSKeyedUnarchiver unarchivedObjectOfClass:[NSColor class]
                                             fromData:[defaults dataForKey:defaultKey]
                                                error:&err];
}

-(void)setColor:(NSColor*)color forDefaultKey:(NSString*)keyName {
    NSError *err = nil;

    NSData *lColorData = [NSKeyedArchiver archivedDataWithRootObject:color
                                               requiringSecureCoding:YES
                                                               error:&err];
    [defaults setValue:lColorData forKey:keyName];
}


-(void)setEditorColorDefault:(NSColor*)value {
    [self setColor:value forDefaultKey:@"TextEditorColorDefault"];
}

-(NSColor*)editorColorDefault {
    return [self colorWithDefaultKey:@"TextEditorColorDefault"];
}

-(void)setEditorColorString:(NSColor*)value {
    [self setColor:value forDefaultKey:@"TextEditorColorString"];
}

-(NSColor*)editorColorString {
    return [self colorWithDefaultKey:@"TextEditorColorString"];
}

-(void)setEditorColorKey:(NSColor*)value {
    [self setColor:value forDefaultKey:@"TextEditorColorKey"];
}

-(NSColor*)editorColorKey {
    return [self colorWithDefaultKey:@"TextEditorColorKey"];
}

-(void)setEditorColorKeyword:(NSColor*)value {
    [self setColor:value forDefaultKey:@"TextEditorColorKeyword"];
}

-(NSColor*)editorColorKeyword {
    return [self colorWithDefaultKey:@"TextEditorColorKeyword"];
}

-(void)setEditorColorNumber:(NSColor*)value {
    [self setColor:value forDefaultKey:@"TextEditorColorNumber"];
}

-(NSColor*)editorColorNumber {
    return [self colorWithDefaultKey:@"TextEditorColorNumber"];
}
    
-(void)setEnableTreeViewSyntaxColoring:(BOOL)value {
    [defaults setBool:value forKey:@"TreeViewSyntaxColoring"];
}

-(BOOL)enableTreeViewSyntaxColoring {
    return [defaults boolForKey:@"TreeViewSyntaxColoring"];
}

-(BOOL)validateIndentSize:(NSNumber**)indentSize error:(NSError**)err {
    if([*indentSize intValue]<1) {
        *indentSize = [NSNumber numberWithInt:3];
    }
    
    return YES;
}

-(void)setIndentSize:(int)indentSize {
    if(indentSize<1) {
        indentSize = 3;
    }
    
    _indentSize = indentSize;
    [defaults setInteger:indentSize forKey:@"IndentSize"];
}

-(int)indentSize {
    if(_indentSize<0) {
        _indentSize = (int)[defaults integerForKey:@"IndentSize"];;
    }
    return _indentSize;
}

+(BracezPreferences*)sharedPreferences {
    if(!__sharedPreferences) {
        __sharedPreferences = [[BracezPreferences alloc] init];
    }
    
    return __sharedPreferences;
}

@end
