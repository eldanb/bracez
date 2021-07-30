//
//  ProjectionTableDataSource.m
//  Bracez
//
//  Created by Eldan Ben Haim on 21/07/2021.
//

#import "ProjectionTableDataSource.h"
#import "JsonPathExpressionCompiler.hpp"
#import "NSString+WStringUtils.h"

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


-(NSView *)tableView:(NSTableView *)tableView viewForTableColumn:(NSTableColumn *)tableColumn row:(NSInteger)row {
    ProjectionTableColumn* projectionCol = (ProjectionTableColumn*)tableColumn;
    Node *valueForCol = [projectionCol resolveColumnInRow:rowNodes[row]];
    
    // TODO: col should return a view rendered
    NSTextField *viewForCol = [[NSTextField alloc] initWithFrame:NSMakeRect(0,0,0,0)];
    
    NSString *nodeValue = [JsonCocoaNode nodeForElement:valueForCol withName:@""].nodeValue;
    if(!nodeValue) {
        nodeValue = @"";
    }
    
    viewForCol.stringValue = nodeValue;
    
    return viewForCol;
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
