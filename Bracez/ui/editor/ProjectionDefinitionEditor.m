//
//  ProjectionDefinitionEditor.m
//  Bracez
//
//  Created by Eldan Ben Haim on 24/07/2021.
//

#import "ProjectionDefinitionEditor.h"

@interface ProjectionDefinitionEditor () {
    ProjectionDefinition *_editedProjection;
}

@end

@implementation ProjectionDefinitionEditor


-(ProjectionDefinition *)editedProjection {
    return _editedProjection;
}


- (IBAction)saveDefinition:(id)sender {
    [self.window.sheetParent endSheet:self.window returnCode:NSModalResponseOK];
}

- (IBAction)cancelDefinitionEdit:(id)sender {
    [self.window.sheetParent endSheet:self.window returnCode:NSModalResponseCancel];
}

-(instancetype)initWithDefinition:(ProjectionDefinition*)editedDefinition {
    self = [super initWithWindowNibName:@"ProjectionDefinitionEditor"];
    if(self) {
        _editedProjection = [editedDefinition copy];
    }
    return self;
}

- (void)windowDidLoad {
    [super windowDidLoad];
    
    // Implement this method to handle any initialization after your window controller's window has been loaded from its nib file.
}

@end
