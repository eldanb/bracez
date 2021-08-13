//
//  ProjectionTableDataSource.m
//  Bracez
//
//  Created by Eldan Ben Haim on 21/07/2021.
//

#import "ProjectionTableDataSource.h"
#import "JsonPathExpressionCompiler.hpp"
#import "NSString+WStringUtils.h"
#import "NodeSelectionController.h"

@interface ProjectionTableColumn : NSTableColumn {
    JsonPathExpression columnExpression;
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
        columnExpression = JsonPathExpression::compile(projectionField.expression.cStringWstring);
    }
    
    return self;
}

-(Node*)resolveColumnInRow:(Node*)row {
    auto resultList = columnExpression.execute(row);
    if(resultList.size()) {
        return resultList.front();
    } else {
        return nullptr;
    }
}

-(NSString *)prototypeViewIdentifier {
    return @"default";
}
@end




@interface ProjectionTableDataSource () {
    ProjectionDefinition *definition;
    JsonDocument *jsonDocument;
    
    JsonPathExpression dataSourceExpression;
    std::vector<Node*> rowNodes;
}
@end

@implementation ProjectionTableDataSource


-(instancetype)initWithDefinition:(ProjectionDefinition*)definition projectedDocument:(JsonDocument*)jsonFile {
    self = [super init];
    
    if(self) {
        self->definition = definition;
        self->jsonDocument = jsonFile;
        
        self->dataSourceExpression = JsonPathExpression::compile(self->definition.rowSelector.cStringWstring);
    }
    
    return self;
}

-(void)reloadData {
    auto dataSourceList = self->dataSourceExpression.execute(jsonDocument.rootNode.proxiedElement);
    rowNodes.clear();
    rowNodes.reserve(dataSourceList.size());
    std::copy(std::begin(dataSourceList), std::end(dataSourceList), std::back_inserter(rowNodes));
}

-(NSInteger)numberOfRowsInTableView:(NSTableView *)tableView {
    return rowNodes.size();
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

-(NSView *)tableView:(NSTableView *)tableView
  viewForTableColumn:(NSTableColumn *)tableColumn
                 row:(NSInteger)row {
    ProjectionTableColumn* projectionCol = (ProjectionTableColumn*)tableColumn;
    Node *valueForCol = [projectionCol resolveColumnInRow:rowNodes[row]];
    
    // TODO: col should return a view rendered
    NSTableCellView *viewForCol = [tableView makeViewWithIdentifier:@"projectionCell" owner:tableView];
    
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
            [self.delegate projectionTableDataSource:self didSelectNode:node];
        }
    }
    
}

-(void)configureTableViewColumns:(NSTableView *)tableView {
    while(tableView.tableColumns.count) {
        [tableView removeTableColumn:tableView.tableColumns.lastObject];
    }
    
    for(ProjectionFieldDefinition *def in definition.fieldDefinitions) {
        ProjectionTableColumn *projTableColumn = [[ProjectionTableColumn alloc] initWithProjectionFieldDefinition:def];
        
        [tableView addTableColumn:projTableColumn];
    }
}

-(ProjectionDefinition *)projectionDefinition {
    return self->definition;
}

@end
