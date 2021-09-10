//
//  ProjectionDefinitionEditor.m
//  Bracez
//
//  Created by Eldan Ben Haim on 24/07/2021.
//

#import "ProjectionDefinitionEditor.h"
#import "ProjectionTableController.h"
#import "JsonPathExpressionCompiler.hpp"
#import "NSString+WStringUtils.h"
#import "BracezPreferences.h"
#import "SyntaxEditingField.h"
#import "JsonPathExpressionCompiler.hpp"

@interface ProjectionDefinitionEditor () <NSTableViewDataSource, SyntaxEditingFieldDelegate> {
    ProjectionDefinition *_editedProjection;
    NSWindow* _overrideSheetParent;
    __weak IBOutlet NSTableView *projectionTable;
    IBOutlet ProjectionTableController *_projectionController;
    JsonDocument *_previewDocument;
    
    __weak IBOutlet NSTableView *fieldListView;
    ProjectionDefinition *_previewedProjection;
    NSTimer *_refreshTimer;
    
    __unsafe_unretained IBOutlet SyntaxEditingField *rowSelectorEditor;
    __unsafe_unretained IBOutlet SyntaxEditingField *fieldExpressionEditor;
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
        [self updatePreviewedProjection];
    }];
    
    [fieldListView registerForDraggedTypes: [NSArray arrayWithObject: @"public.text"]];
    
    BracezPreferences *prefs = [BracezPreferences sharedPreferences];
    rowSelectorEditor.font = prefs.editorFont;
    fieldExpressionEditor.font = prefs.editorFont;
}


-(void)updatePreviewedProjection {
    if(![_previewedProjection isEqualTo:_editedProjection]) {
        _previewedProjection = [_editedProjection copy];
        _projectionController.projectionDefinition = _previewedProjection;
    }
}

-(id<NSPasteboardWriting>)tableView:(NSTableView *)tableView pasteboardWriterForRow:(NSInteger)row {
    NSData *item = [NSKeyedArchiver archivedDataWithRootObject:[_editedProjection.fieldDefinitions objectAtIndex:row]
                                            requiringSecureCoding:YES
                                                            error:nil];
    NSPasteboardItem *pboardItem = [[NSPasteboardItem alloc] init];
    [pboardItem setPropertyList:@{ @"index": @(row), @"item": item } forType:@"public.text"];
     
    return pboardItem;
}

- (NSDragOperation)tableView:(NSTableView *)tableView validateDrop:(id<NSDraggingInfo>)info proposedRow:(NSInteger)row proposedDropOperation:(NSTableViewDropOperation)dropOperation {

    BOOL canDrop = row >= 0;
    if(canDrop) {
        return NSDragOperationMove;
    } else {
        return NSDragOperationNone;
    }
 }

-(BOOL)tableView:(NSTableView *)tableView acceptDrop:(id<NSDraggingInfo>)info row:(NSInteger)row dropOperation:(NSTableViewDropOperation)dropOperation {
    
    if(dropOperation != NSDragOperationNone) {
        NSNumber *orgIndex = [info.draggingPasteboard propertyListForType:@"public.text"][@"index"];
        ProjectionFieldDefinition *item = [NSKeyedUnarchiver unarchivedObjectOfClass:[ProjectionFieldDefinition class]
                                                           fromData:[info.draggingPasteboard propertyListForType:@"public.text"][@"item"]
                                                              error:nil];
        
        [_editedProjection removeProjectionAtIndex:orgIndex.unsignedIntValue];
        if(orgIndex.unsignedIntValue < row) {
            row --;
        }
        [_editedProjection insertProjection:item atIndex:row];
    
        [fieldListView reloadData];
    }
    return YES;
}

- (void) suggestedFieldSelected:(id)sender {
    NSMenuItem *fieldItem = sender;
    ProjectionFieldDefinition *def = fieldItem.representedObject;
    [_editedProjection addProjection:def];
}

- (IBAction)suggestFields:(id)sender {
    NSMenu *suggestionMenu = [[NSMenu alloc] initWithTitle:@"Field suggestions"];
    for(ProjectionFieldDefinition *def in [_editedProjection suggestFieldsBasedOnDocument:_previewDocument]) {
        NSMenuItem *suggestionItem = [[NSMenuItem alloc] initWithTitle:def.expression
                                                                action:@selector(suggestedFieldSelected:)
                                                         keyEquivalent:@""];
        suggestionItem.representedObject = def;
        suggestionItem.target = self;
        [suggestionMenu addItem:suggestionItem];
        
        if(suggestionMenu.itemArray.count > 20) {
            break;
        }
    }
    
    if(!suggestionMenu.itemArray.count) {
        NSMenuItem *placeholder = [[NSMenuItem alloc] initWithTitle:@"No suggestions"
                                                             action:nil
                                                      keyEquivalent:@""];
        placeholder.enabled = NO;
        [suggestionMenu addItem:placeholder];
    }
        
    [NSMenu popUpContextMenu:suggestionMenu
                   withEvent:[NSApplication sharedApplication].currentEvent
                     forView:sender];
}

- (void)syntaxEditingFieldChanged:(nonnull SyntaxEditingField *)sender {
    [self updatePreviewedProjection];
}

-(void)syntaxEditingField:(SyntaxEditingField *)sender checkSyntax:(NSString *)text {
    [sender clearErrorRanges];
    try {
        JsonPathExpression::compile([text cStringWstring]);
    } catch(const parse_error &e) {
        [sender markErrorRange: NSMakeRange(
                                           std::min(e._offset_start, sender.textStorage.length-1),
                                           std::max(e._len, (size_t)1))
              withErrorMessage: [NSString stringWithFormat:@"Invalid JSON path syntax: %s; expected: %s",
                               e._what.c_str(), e._expecting.c_str()]
         ];
        
    } catch(const std::exception &e) {
        [sender markErrorRange: NSMakeRange(0, sender.textStorage.length)
              withErrorMessage: [NSString stringWithFormat:@"Invalid JSON path syntax: %s", e.what()]
         ];
    }
}
@end

