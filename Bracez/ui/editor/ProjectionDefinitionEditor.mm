//
//  ProjectionDefinitionEditor.m
//  Bracez
//
//  Created by Eldan Ben Haim on 24/07/2021.
//

#import "ProjectionDefinitionEditor.h"
#import "ProjectionTableController.h"

@interface ProjectionDefinitionEditor () {
    ProjectionDefinition *_editedProjection;
    NSWindow* _overrideSheetParent;
    __weak IBOutlet NSTableView *projectionTable;
    IBOutlet ProjectionTableController *_projectionController;
    JsonDocument *_previewDocument;
    
    ProjectionDefinition *_previewedProjection;
    NSTimer *_refreshTimer;
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

-(instancetype)initWithDefinition:(ProjectionDefinition*)editedDefinition previewDocument:(JsonDocument* __nullable)previewDocument {
    self = [super initWithWindowNibName:@"ProjectionDefinitionEditor"];
    if(self) {
        _editedProjection = [editedDefinition copy];
        _previewDocument = previewDocument;
    }
    return self;
}

-(void)awakeFromNib {
    [super awakeFromNib];
    [projectionTable registerNib:[[NSNib alloc] initWithNibNamed:@"ProjectionTableCell" bundle:nil] forIdentifier:@"projectionCell"];

    _projectionController.projectedDocument = _previewDocument;
    
    _refreshTimer = [NSTimer scheduledTimerWithTimeInterval:0.5
                                                    repeats:YES
                                                      block:^(NSTimer * _Nonnull timer) {
        if(![self->_previewedProjection isEqualTo:self->_editedProjection]) {
            self->_previewedProjection = [self->_editedProjection copy];
            self->_projectionController.projectionDefinition = self->_previewedProjection;
        }
    }];
}

@end
