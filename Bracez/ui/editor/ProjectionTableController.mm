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


@interface ProjectionColumnSortDescriptor : NSSortDescriptor {
    __weak ProjectionTableColumn *_byColumn;
}

-(NSComparisonResult)compareNode:(Node*)left toNode:(Node*)right;
+(ProjectionColumnSortDescriptor*)forColumn:(ProjectionTableColumn*)column
                                  ascending:(BOOL)ascending;

@end

@implementation ProjectionTableColumn

-(instancetype)initWithProjectionFieldDefinition:(ProjectionFieldDefinition*)projectionField {
    self = [super initWithIdentifier:projectionField.fieldTitle];
    
    if(self) {
        self.title = projectionField.fieldTitle;
        self.sortDescriptorPrototype = [ProjectionColumnSortDescriptor forColumn:self ascending:NO];

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

-(NSString*)displayTextForColumnInRow:(Node*)row {
    Node *valueForCol = [self resolveColumnInRow:row];
    return [self formatNodeToText:valueForCol];
}

-(NSString*)formatNodeToText:(Node*)node {
    NSString *nodeValue = [JsonCocoaNode nodeForElement:node withName:@""].nodeValue;

    if(!nodeValue) {
        nodeValue = @"";
    }
    
    return  nodeValue;;
}

-(BOOL)doesMatchFilter:(NSString*)filterText forRow:(Node*)node {
    NSString *text = [self formatNodeToText:[self resolveColumnInRow:node]];
    return [text localizedStandardContainsString:filterText];
}

-(NSComparisonResult)compareForSort:(Node*)left to:(Node*)right {
    return [[self formatNodeToText:[self resolveColumnInRow:left]]
                compare:[self formatNodeToText:[self resolveColumnInRow:right]]];
}

-(NSString *)prototypeViewIdentifier {
    return @"default";
}
@end


@implementation ProjectionColumnSortDescriptor

+(ProjectionColumnSortDescriptor*)forColumn:(ProjectionTableColumn*)column
                                  ascending:(BOOL)ascending {
    ProjectionColumnSortDescriptor *ret = [[ProjectionColumnSortDescriptor alloc] initWithKey:column.title
                                                                                    ascending:ascending];
    
    ret->_byColumn = column;
    
    return ret;
}

-(id)copyWithZone:(NSZone *)zone {
    ProjectionColumnSortDescriptor *ret = [super copyWithZone:zone];
    ret->_byColumn = _byColumn;
    return ret;
}

-(ProjectionColumnSortDescriptor*)reversedSortDescriptor {
    return [ProjectionColumnSortDescriptor forColumn:self->_byColumn
                                           ascending:!self.ascending];
}

-(NSComparisonResult)compareNode:(Node*)left toNode:(Node*)right {
    return self.ascending ?
            [_byColumn compareForSort:left to:right] :
            [_byColumn compareForSort:right to:left];
}
@end


@interface ProjectionTableColumnText : ProjectionTableColumn
@end

@implementation ProjectionTableColumnText

-(NSString*)formatNodeToText:(Node*)node {
    if(dynamic_cast<StringNode*>(node)) {
        return [NSString stringWithWstring:((StringNode*)node)->GetValue()];
    } else
    if(!dynamic_cast<ContainerNode*>(node)) {
        return [super formatNodeToText:node];
    } else
    {
        return @"";
    }
}

@end

@interface ProjectionTableColumnNumber : ProjectionTableColumn {
    NSNumberFormatter *numberFormatter;
}

@end

@implementation ProjectionTableColumnNumber

-(instancetype)initWithProjectionFieldDefinition:(ProjectionFieldDefinition*)projectionField {
    self = [super initWithProjectionFieldDefinition:projectionField];
    numberFormatter = [[NSNumberFormatter alloc] init];
    return self;
}

-(NSNumber*)numberValueForNode:(Node*)node {
    if(dynamic_cast<StringNode*>(node)) {
        return [numberFormatter numberFromString:[NSString stringWithWstring:((StringNode*)node)->GetValue()]];
    } else
    if(dynamic_cast<NumberNode*>(node)) {
        return [NSNumber numberWithDouble:((NumberNode*)node)->GetValue()];
    } else {
        return nil;
    }
}

-(NSString*)formatNodeToText:(Node*)node {
    NSNumber *num = [self numberValueForNode:node];
    return num ? [num description] : @"";
}

-(NSComparisonResult)compareForSort:(Node*)left to:(Node*)right {
    return [[self numberValueForNode:[self resolveColumnInRow:left]]
                compare:[self numberValueForNode:[self resolveColumnInRow:right]]];
}

@end

@interface ProjectionTableColumnDate : ProjectionTableColumn {
    NSDateFormatter *dateFormatter;
    NSDateFormatter *isoDateFormatter;
    
}
@end

@implementation ProjectionTableColumnDate

-(instancetype)initWithProjectionFieldDefinition:(ProjectionFieldDefinition*)projectionField {
    self = [super initWithProjectionFieldDefinition:projectionField];
    dateFormatter = [NSDateFormatter new];
    
    isoDateFormatter = [NSDateFormatter new];
    isoDateFormatter.dateFormat = @"yyyy-MM-dd'T'HH:mm:ss.SSS'Z'";
    isoDateFormatter.locale = [[NSLocale alloc] initWithLocaleIdentifier:@"en_US_POSIX"];

    return self;
}

-(NSDate*)dateValueForNode:(Node*)node {
    if(dynamic_cast<StringNode*>(node)) {
        NSString *stringValue = [NSString stringWithWstring:((StringNode*)node)->GetValue()];
        
        NSDate *dateRet = [dateFormatter dateFromString:stringValue];
        
        if(!dateRet) {
            dateRet = [isoDateFormatter dateFromString:stringValue];
        }
        
        return dateRet;
    } else
    if(dynamic_cast<NumberNode*>(node)) {
        return [NSDate dateWithTimeIntervalSince1970:((NumberNode*)node)->GetValue()];
    } else {
        return nil;
    }
}

-(NSString*)formatNodeToText:(Node*)node {
    NSDate *date = [self dateValueForNode:node];
    return date ? [date description] : @"";
}

-(NSComparisonResult)compareForSort:(Node*)left to:(Node*)right {
    return [[self dateValueForNode:[self resolveColumnInRow:left]]
                compare:[self dateValueForNode:[self resolveColumnInRow:right]]];
}

@end


@interface ProjectionTableController () {
    ProjectionDefinition *definition;
    JsonDocument *jsonDocument;
    NSTableView *_tableView;
    
    NSString *_filterText;
    
    JsonPathExpression dataSourceExpression;
    std::vector<Node*> rowNodes;

    std::vector<Node*> displayedRowNodes;
    
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
            dataSourceExpression = [self->definition compiledRowSelector];
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
        JsonPathResultNodeList dataSourceList = self->dataSourceExpression.execute(jsonDocument.rootNode.proxiedElement);
        rowNodes.reserve(dataSourceList.size());
        std::copy(std::begin(dataSourceList), std::end(dataSourceList), std::back_inserter(rowNodes));
    }
    
    [self updateDisplayedRows];
    
    [_tableView reloadData];
}

-(NSInteger)numberOfRowsInTableView:(NSTableView *)tableView {
    return displayedRowNodes.size();
}

-(NSView *)tableView:(NSTableView *)tableView
  viewForTableColumn:(NSTableColumn *)tableColumn
                 row:(NSInteger)row {
    ProjectionTableColumn* projectionCol = (ProjectionTableColumn*)tableColumn;
    NSTableCellView *viewForCol = [tableView makeViewWithIdentifier:@"projectionCell" owner:tableView];

    viewForCol.textField.stringValue = [projectionCol displayTextForColumnInRow:displayedRowNodes[row]];
    
    return viewForCol;
}

-(void)tableViewSelectionDidChange:(NSNotification *)notification {
    if(self.delegate) {
        NSInteger selectedRow = [(NSTableView*)notification.object selectedRow];
        if(selectedRow >= 0) {
            Node *node = displayedRowNodes[selectedRow];
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
            ProjectionTableColumn *projTableColumn;
            switch(def.formatterType) {
                case ProjectionFieldFormatterText:
                    projTableColumn = [[ProjectionTableColumnText alloc] initWithProjectionFieldDefinition:def];
                    break;

                case ProjectionFieldFormatterNumber:
                    projTableColumn = [[ProjectionTableColumnNumber alloc] initWithProjectionFieldDefinition:def];
                    break;

                case ProjectionFieldFormatterDate:
                    projTableColumn = [[ProjectionTableColumnDate alloc] initWithProjectionFieldDefinition:def];
                    break;

                default:
                    projTableColumn = [[ProjectionTableColumn alloc] initWithProjectionFieldDefinition:def];
                    break;
                    
            }
            [_tableView addTableColumn:projTableColumn];
        }
    }
}

-(void)updateDisplayedRows {
    NSArray<NSSortDescriptor*> *sortDescriptors = self.tableView.sortDescriptors;
    NSArray<NSTableColumn*> *cols = self.tableView.tableColumns;
    
    if(!_filterText.length) {
        displayedRowNodes = rowNodes;
    } else {
        displayedRowNodes.clear();
        displayedRowNodes.reserve(rowNodes.size()/2);
        
        NSString *filterText = _filterText;
        
        std::copy_if(rowNodes.begin(), rowNodes.end(), inserter(displayedRowNodes, displayedRowNodes.end()), [cols, filterText] (Node* node) {
            for(ProjectionTableColumn *ptc in cols) {
                if([ptc doesMatchFilter:filterText forRow:node]) {
                    return true;
                }
            }
            
            return false;
        });
    }
    
    if(sortDescriptors.count) {
        std::sort(displayedRowNodes.begin(),
                  displayedRowNodes.end(), [sortDescriptors](Node *a, Node *b) {
            for(ProjectionColumnSortDescriptor *d in sortDescriptors) {
                NSComparisonResult r = [d compareNode:a toNode:b];
                if(r < 0) {
                    return true;
                } else
                if(r > 0) {
                    return false;
                }
            }
            
            return false;
        });
    }
}

-(void)tableView:(NSTableView *)tableView sortDescriptorsDidChange:(NSArray<NSSortDescriptor *> *)oldDescriptors {
    [self updateDisplayedRows];
    [tableView reloadData];
}

-(void)selectNode:(JsonCocoaNode*)node {
    json::TextRange selectedNodeRange = node.proxiedElement->GetAbsTextRange();
    auto iter = std::find_if(displayedRowNodes.begin(), displayedRowNodes.end(), [selectedNodeRange](json::Node* node) {
        return node->GetAbsTextRange().contains(selectedNodeRange);
    });
    
    if(iter != displayedRowNodes.end()) {
        size_t rowIndex = iter - displayedRowNodes.begin();
        [self.tableView selectRowIndexes:[NSIndexSet indexSetWithIndex:rowIndex]
                    byExtendingSelection:NO];
        [self.tableView scrollRowToVisible:rowIndex];
    }
}

-(void)onSearchChange:(NSSearchField*)sender {
    _filterText = sender.stringValue;
    [self updateDisplayedRows];
    [self.tableView reloadData];
}


@end
