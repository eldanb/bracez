//
//  ProjectionDefinition.m
//  Bracez
//
//  Created by Eldan Ben Haim on 21/07/2021.
//

#import "ProjectionDefinition.h"
#include "JsonPathExpressionCompiler.hpp"

#import "NSString+WStringUtils.h"
#import "JsonDocument.h"

#include <random>
#include <algorithm>

std::vector<std::wstring> suggestFields(const JsonPathResultNodeList &nodeList);


@implementation ProjectionFieldDefinition

-(instancetype)init {
    self = [super init];
    if(self) {
        self.fieldTitle = @"Column";
        self.expression = @"$";
    }
    
    return self;
}

- (nonnull id)copyWithZone:(nullable NSZone *)zone {
    ProjectionFieldDefinition *copy = [[[self class] alloc] init];
    copy.fieldTitle = self.fieldTitle;
    copy.expression = self.expression;
    copy.formatterType = self.formatterType;
    
    return copy;
}

+(BOOL)supportsSecureCoding {
    return YES;
}

- (void)encodeWithCoder:(nonnull NSCoder *)coder {
    [coder encodeObject:self.fieldTitle forKey:@"fieldTitle"];
    [coder encodeObject:self.expression forKey:@"expression"];
    [coder encodeObject:self.formatterType forKey:@"formatterType"];
}

- (nullable instancetype)initWithCoder:(nonnull NSCoder *)coder {
    self = [super init];
    if(self) {
        self.fieldTitle = [coder decodeObjectOfClass:[NSString class] forKey:@"fieldTitle"];
        self.expression = [coder decodeObjectOfClass:[NSString class] forKey:@"expression"];
        self.formatterType = [coder decodeObjectOfClass:[NSString class] forKey:@"formatterType"];
    }
    return self;
}

-(BOOL)isEqual:(id)other {
    if(![other isKindOfClass:[ProjectionFieldDefinition class]]) {
        return NO;
    }
    
    ProjectionFieldDefinition *otherDef = other;
    return [self.fieldTitle isEqualTo:otherDef.fieldTitle] &&
           [self.expression isEqualTo:otherDef.expression] &&
           ([self.formatterType isEqualTo:otherDef.formatterType] ||
                (self.formatterType == nil && otherDef.formatterType == nil)); 
}

@end


@interface ProjectionDefinition () {
    NSMutableArray *_fieldDefinitions;
}

@end

@implementation ProjectionDefinition

-(instancetype)init {
    self = [super init];
    _fieldDefinitions = [NSMutableArray arrayWithCapacity:3];
    return self;
}

+(instancetype)newDefinition {
    ProjectionDefinition *ret = [[ProjectionDefinition alloc] init];
    ret.projectionName = @"Untitled Projection";
    ret.rowSelector = @"$[*]";

    ProjectionFieldDefinition *fieldDef = [[ProjectionFieldDefinition alloc] init];
    fieldDef.fieldTitle = @"Result";
    fieldDef.expression = @"$";

    [ret addProjection:fieldDef];
    
    return ret;
}

-(void)addProjection:(ProjectionFieldDefinition*)definition {
    [_fieldDefinitions addObject:definition];
}

-(void)insertProjection:(ProjectionFieldDefinition*)definition atIndex:(NSUInteger)index {
    [_fieldDefinitions insertObject:definition atIndex:index];
}

-(void)removeProjectionAtIndex:(NSUInteger)index {
    [_fieldDefinitions removeObjectAtIndex:index];
}

-(NSArray<ProjectionFieldDefinition*> *)suggestFieldsBasedOnDocument:(JsonDocument*)document {
    if(self.rowSelector) {
        JsonPathExpression rowSelector = JsonPathExpression::compile(self.rowSelector.UTF8String);
        auto nodes = rowSelector.execute(document.jsonFile->getDom()->GetChildAt(0));
        std::vector<std::wstring> suggestions = suggestFields(nodes);
        
        NSMutableArray<ProjectionFieldDefinition*> *ret = [NSMutableArray arrayWithCapacity:suggestions.size()];
        for(auto it = suggestions.begin(); it != suggestions.end(); it++) {
            ProjectionFieldDefinition *fieldDef = [[ProjectionFieldDefinition alloc] init];
            fieldDef.expression = [NSString stringWithWstring:*it];
            fieldDef.fieldTitle = [[fieldDef.expression componentsSeparatedByString:@"."] lastObject];
            
            [ret addObject:fieldDef];
        }
        
        return ret;
    } else {
        return nil;
    }
}

-(NSArray<ProjectionFieldDefinition*> *)fieldDefinitions {
    return _fieldDefinitions;
}

- (nonnull id)copyWithZone:(nullable NSZone *)zone {
    ProjectionDefinition *copy = [[[self class] alloc] init];
    
    copy.projectionName = self.projectionName;
    copy.rowSelector = self.rowSelector;
    
    for(ProjectionFieldDefinition *fieldDef in self.fieldDefinitions) {
        [copy addProjection:[fieldDef copy]];
    }
    
    return copy;
}

- (void)encodeWithCoder:(nonnull NSCoder *)coder {
    [coder encodeObject:self.projectionName forKey:@"projectionName"];
    [coder encodeObject:self.rowSelector forKey:@"rowSelector"];
    [coder encodeObject:_fieldDefinitions forKey:@"fieldDefinitions"];
}

- (nullable instancetype)initWithCoder:(nonnull NSCoder *)coder {
    self = [super init];
    if(self) {
        self.projectionName = [coder decodeObjectOfClass:[NSString class] forKey:@"projectionName"];
        self.rowSelector = [coder decodeObjectOfClass:[NSString class] forKey:@"rowSelector"];
        _fieldDefinitions = [[coder decodeArrayOfObjectsOfClass:[ProjectionFieldDefinition class]
                                                         forKey:@"fieldDefinitions"] mutableCopy];
    }
    return self;
}

+(BOOL)supportsSecureCoding {
    return YES;
}

-(BOOL)isEqual:(id)other {
    if(![other isKindOfClass:[ProjectionDefinition class]]) {
        return NO;
    }
    
    ProjectionDefinition *otherDef = other;
    if(![self.rowSelector isEqualTo:otherDef.rowSelector]) {
        return NO;
    }
    
    if(![self.fieldDefinitions isEqualToArray:otherDef.fieldDefinitions]) {
        return NO;
    }
    
    return YES;
}

@end


static void walkPathsForNode(const json::Node *node, const std::wstring &pathToNode, int maxDepth, const std::function<void(const std::wstring &, const json::Node *)> &callback) {
    const json::ObjectNode *objNode = dynamic_cast<const json::ObjectNode*>(node);
    if(objNode) {
        if(maxDepth > 0) {
            std::for_each(objNode->Begin(), objNode->End(),
                          [&callback, maxDepth, &pathToNode](const json::ObjectNode::Member &member) {
                                    walkPathsForNode(member.node.get(),
                                                     pathToNode + L"." + member.name,
                                                     maxDepth-1, callback);
                            });
        }
    } else {
        callback(pathToNode, node);
    }
}

template<class It>
double calculateEntropy(It start, It end) {
    std::map<typename It::value_type, int> histogram;
    int totalCount = 0;
    
    for(It i = start; i != end; i++) {
        totalCount++;
        histogram[*i] ++;
    }
    
    double entSum = 0;
    for(auto histIterator = histogram.begin(); histIterator != histogram.end(); histIterator++) {
        double p = (histIterator->second / (double)totalCount);
        entSum += p * log(p);
    }
    
    return -entSum;
}


std::vector<std::wstring> suggestFields(const JsonPathResultNodeList &nodeList) {
    // Sample row nodes
    std::vector<json::Node*> sampledNodes;
    std::sample(
                nodeList.begin(), nodeList.end(),
                std::back_inserter(sampledNodes),
                100, std::mt19937{std::random_device{}()});

    // Collect values and underlying fields
    std::map<std::wstring, vector<std::wstring>> valuesForFields;
    long maxVals = 0;
    
    std::for_each(sampledNodes.begin(), sampledNodes.end(), [&valuesForFields, &maxVals](json::Node* sampledNode) {
        walkPathsForNode(sampledNode, L"$", 3, [&valuesForFields, &maxVals](const std::wstring &path, const json::Node *node) {
            std::wstring nodeStr;
            node->CalculateJsonTextRepresentation(nodeStr);
            std::vector<std::wstring> &valsForField = valuesForFields[path];
            valsForField.push_back(nodeStr);
            if(valsForField.size() > maxVals) {
                maxVals = valsForField.size();
            }
        });
    });
        
    // Sort by quantized number of fields
    struct field_stats_record {
        std::wstring field;
        int nOccurences;
        double entropy;
        
        double score() const { return nOccurences * entropy; }
    };
    
    std::vector<field_stats_record> field_stats;
    std::transform(valuesForFields.begin(), valuesForFields.end(), std::back_inserter(field_stats), [](const decltype(valuesForFields)::value_type &e) {
        return field_stats_record { e.first, (int)e.second.size(), calculateEntropy(e.second.begin(), e.second.end()) };
    });
    
    std::sort(field_stats.begin(), field_stats.end(), [](field_stats_record &field_a, field_stats_record &field_b) {
        return field_b.score() < field_a.score();
    });

    std::vector<std::wstring> results;
    std::transform(field_stats.begin(), field_stats.end(), std::back_inserter(results), [](const field_stats_record &e) {
        return e.field;
    });
    
    return results;
}
