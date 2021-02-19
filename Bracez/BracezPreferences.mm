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
    
    NSFont *_editorFont;
}

-(BOOL)gutterMasterSwitch;
-(BOOL)gutterLineNumbers;

-(NSFont*)editorFont;
@end


@implementation BracezPreferences

-(instancetype)init {
    self = [super init];
    defaults = [NSUserDefaults standardUserDefaults];
    return self;
}

-(BOOL)gutterMasterSwitch {
    return [defaults boolForKey:@"GutterMaster"];
}

-(BOOL)gutterLineNumbers {
    return [defaults boolForKey:@"GutterLineNumbers"];
}

-(NSFont*)editorFont {
    if(!_editorFont) {
        _editorFont = [self loadFontWithKey:@"TextEditorFont"];
    }
    
    return _editorFont;
}

-(NSFont*)loadFontWithKey:(NSString*)key {
    NSData *lFontData =[defaults valueForKey:key];
    return [NSKeyedUnarchiver unarchivedObjectOfClass:NSFont.class
                                             fromData:lFontData
                                                error:nil];
}

+(BracezPreferences*)sharedPreferences {
    if(!__sharedPreferences) {
        __sharedPreferences = [[BracezPreferences alloc] init];
    }
    
    return __sharedPreferences;
}

@end
