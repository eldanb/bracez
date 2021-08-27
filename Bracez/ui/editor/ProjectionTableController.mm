//
//  ProjectionTableDataSource.m
//  Bracez
//
//  Created by Eldan Ben Haim on 21/07/2021.
//

#import "ProjectionTableController.h"
#import "JsonPathExpressionCompiler.hpp"
#import "NSString+WStringUtils.h"
#import "NodeSelectionController.h"

@interface ProjectionTableColumn : NSTableColumn {
    JsonPathExpression columnExpression;
    BOOL _validDef;
}

-(instancetype)initWithProjectionFieldDefinition:(ProjectionFieldDefinition*)projectionField;
-(Node*)resolveColumnInRow:(Node*)row;

@property (readonly) NSString *prototypeViewIdentifier;

@end


@implementation ProjectionTableColumn


-(instancetype)initWithProjectionFieldDefinition:(ProjectionFieldDefinition*)projectionField {
    self = [super initWithIdentifier:projectionField.fieldTitle];
    if(self) {
        self.title = projectionField.fieldTitle;
        try {
            columnExpression = JsonPathExpression::compile(projectionField.expression.cStringWstring);
            _validDef = YES;
        } catch(const std::exception &err) {
            _validDef = NO;
        }
    }
    
    return self;
}

-(Node*)resolveColumnInRow:(Node*)row {
    if(_validDef) {
        auto resultList = columnExpression.execute(row);
        if(resultList.size()) {
            return resultList.front();
        } else {
            return nullptr;
        }
    } else {
        return nullptr;
    }
}

-(NSString *)prototypeViewIdentifier {
    return @"default";
}
@end




@interface ProjectionTableController () {
    ProjectionDefinition *definition;
    JsonDocument *jsonDocument;
    NSTableView *_tableView;
    
    JsonPathExpression dataSourceExpression;
    std::vector<Node*> rowNodes;
    
    BOOL _validDef;
}
@end

@implementation ProjectionTableController


-(instancetype)initWithDefinition:(ProjectionDefinition*)definition projectedDocument:(JsonDocument*)jsonFile {
    self = [super init];
    
    if(self) {
        self->definition = definition;
        self->jsonDocument = jsonFile;
        
    }
    
    return self;
}

-(void)setProjectedDocument:(JsonDocument *)projectedDocument {
    jsonDocument = projectedDocument;
    [self reloadData];
}

-(JsonDocument *)projectedDocument {
    return jsonDocument;
}

-(void)setProjectionDefinition:(ProjectionDefinition *)projectionDefinition {
    definition = projectionDefinition;
    
    _validDef = NO;
    if(definition) {
        try {
            dataSourceExpression = JsonPathExpression::compile(self->definition.rowSelector.cStringWstring);
            _validDef = YES;
        } catch(const std::exception &e) {
            dataSourceExpression = JsonPathExpression();
        }
    }
    
    [self configureTableViewColumns];
    [self reloadData];
}

-(ProjectionDefinition *)projectionDefinition {
    return definition;
}

-(void)reloadData {
    rowNodes.clear();

    if(_validDef && definition) {
        auto dataSourceList = self->dataSourceExpression.execute(jsonDocument.rootNode.proxiedElement);
        rowNodes.reserve(dataSourceList.size());
        std::copy(std::begin(dataSourceList), std::end(dataSourceList), std::back_inserter(rowNodes));
    }
    
    [_tableView reloadData];
}

-(NSInteger)numberOfRowsInTableView:(NSTableView *)tableView {
    return rowNodes.size();
}

-(NSView *)tableView:(NSTableView *)tableView
  viewForTableColumn:(NSTableColumn *)tableColumn
                 row:(NSInteger)row {
    ProjectionTableColumn* projectionCol = (ProjectionTableColumn*)tableColumn;
    NSTableCellView *viewForCol = [tableView makeViewWithIdentifier:@"projectionCell" owner:tableView];

    Node *valueForCol = [projectionCol resolveColumnInRow:rowNodes[row]];
    NSString *nodeValue = [JsonCocoaNode nodeForElement:valueForCol withName:@""].nodeValue;
    
    if(!nodeValue) {
        nodeValue = @"";
    }
    viewForCol.textField.stringValue = nodeValue;
    
    return viewForCol;
}

-(void)tableViewSelectionDidChange:(NSNotification *)notification {
    if(self.delegate) {
        NSInteger selectedRow = [(NSTableView*)notification.object selectedRow];
        if(selectedRow >= 0) {
            Node *node = rowNodes[selectedRow];
            [self.delegate projectionTableController:self didSelectNode:node];
        }
    }
    
}

-(void)setTableView:(NSTableView*)tableView {
    _tableView.delegate = nil;
    _tableView.dataSource = nil;

    _tableView = tableView;
    
    [self configureTableViewColumns];
    
    _tableView.dataSource = self;
    _tableView.delegate = self;
    
    [_tableView reloadData];
}

-(NSTableView*)tableView {
    return _tableView;
}

-(void)configureTableViewColumns {
    while(_tableView.tableColumns.count) {
        [_tableView removeTableColumn:_tableView.tableColumns.lastObject];
    }
    
    if(definition) {
        for(ProjectionFieldDefinition *def in definition.fieldDefinitions) {
            ProjectionTableColumn *projTableColumn = [[ProjectionTableColumn alloc] initWithProjectionFieldDefinition:def];
            
            [_tableView addTableColumn:projTableColumn];
        }
    }
}

-(void)selectNode:(JsonCocoaNode*)node {
    json::TextRange selectedNodeRange = node.proxiedElement->GetAbsTextRange();
    auto iter = std::find_if(rowNodes.begin(), rowNodes.end(), [selectedNodeRange](json::Node* node) {
        return node->GetAbsTextRange().contains(selectedNodeRange);
    });
    
    if(iter != rowNodes.end()) {
        [self.tableView selectRowIndexes:[NSIndexSet indexSetWithIndex:iter - rowNodes.begin()]
                    byExtendingSelection:NO];
    }
}


@end
