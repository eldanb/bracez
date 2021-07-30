//
//  FavoriteNameInputWindowConrtoller.m
//  Bracez
//
//  Created by Eldan Ben Haim on 30/07/2021.
//

#import "FavoriteNameInputWindowConrtoller.h"

@interface FavoriteNameInputWindowConrtoller ()
@property (weak) IBOutlet NSComboBox *nameCombo;
@end

@implementation FavoriteNameInputWindowConrtoller


-(instancetype)initWithInitialName:(NSString*)name
                     existingNames:(NSArray<NSString*>*)names {
    self = [super initWithWindowNibName:@"FavoriteNameInputWindowConrtoller"];
    if(self) {
        self.inputName = name;
        [self loadWindow];
        [self.nameCombo addItemsWithObjectValues:names];
    }
    return self;
}

- (void)windowDidLoad {
    [super windowDidLoad];
    self.nameCombo.stringValue = self.inputName;
}


- (IBAction)onOkClicked:(id)sender {
    self.inputName = self.nameCombo.stringValue;
    [self.window.sheetParent endSheet:self.window
                           returnCode:NSModalResponseOK];
}

- (IBAction)onCancelClicked:(id)sender {
    [self.window.sheetParent endSheet:self.window
                           returnCode:NSModalResponseCancel];
}

@end
