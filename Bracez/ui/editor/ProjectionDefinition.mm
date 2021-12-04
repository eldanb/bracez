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
#include <iostream>
#include <iomanip>

std::vector<std::wstring> suggestFields(const JsonPathResultNodeList &nodeList);
std::vector<pair<std::wstring, size_t>> suggestRowSelectors(Node *parent, const std::wstring &basePath);

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
    [coder encodeInteger:self.formatterType forKey:@"formatterTypeI"];
}

- (nullable instancetype)initWithCoder:(nonnull NSCoder *)coder {
    self = [super init];
    if(self) {
        self.fieldTitle = [coder decodeObjectOfClass:[NSString class] forKey:@"fieldTitle"];
        self.expression = [coder decodeObjectOfClass:[NSString class] forKey:@"expression"];
        self.formatterType = (ProjectionFieldFormatterType)[coder decodeIntegerForKey:@"formatterTypeI"];
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
            self.formatterType == otherDef.formatterType;
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
    [self willChangeValueForKey:@"fieldDefinitions"];
    [_fieldDefinitions addObject:definition];
    [self didChangeValueForKey:@"fieldDefinitions"];
}

-(void)insertProjection:(ProjectionFieldDefinition*)definition atIndex:(NSUInteger)index {
    [_fieldDefinitions insertObject:definition atIndex:index];
}

-(void)removeProjectionAtIndex:(NSUInteger)index {
    [_fieldDefinitions removeObjectAtIndex:index];
}

+(NSArray<ProjectionDefinition*> *)suggestPojectionsForDocument:(JsonDocument*)document {
    std::vector<pair<std::wstring, size_t>> rowSelectors =
        suggestRowSelectors(document.jsonFile->getDom()->getChildAt(0), L"$");

    if(rowSelectors.size()) {
        NSMutableArray<ProjectionDefinition*> *defs = [NSMutableArray arrayWithCapacity:rowSelectors.size()];
        for(auto iter = rowSelectors.begin(); iter != rowSelectors.end(); iter++) {
            
            ProjectionDefinition *suggestedDef = [[ProjectionDefinition alloc] init];
            suggestedDef.rowSelector = [NSString stringWithWstring:iter->first];
            
            NSArray<ProjectionFieldDefinition*> *suggestedFields = [suggestedDef suggestFieldsBasedOnDocument:document];
            for(ProjectionFieldDefinition *def in suggestedFields) {
                [suggestedDef addProjection:def];
                if(suggestedDef.fieldDefinitions.count >= 5) {
                    break;
                }
            }
                        
            [defs addObject:suggestedDef];
        }
        
        return defs;
    } else {
        return nil;
    }
}


-(NSArray<ProjectionFieldDefinition*> *)suggestFieldsBasedOnDocument:(JsonDocument*)document {
    if(self.rowSelector) {
        JsonPathExpression rowSelector = [self compiledRowSelector];
        auto nodes = rowSelector.execute(document.jsonFile->getDom()->getChildAt(0));
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

-(JsonPathExpression)compiledRowSelector {
    return JsonPathExpression::compile(self.rowSelector.UTF8String);
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
            std::for_each(objNode->begin(), objNode->end(),
                          [&callback, maxDepth, &pathToNode](const json::ObjectNode::Member &member) {
                                    std::wstring navPath = JsonPathExpression::isValidIdentifier(member.name) ?
                                                                std::wstring(L".") + member.name :
                                                                std::wstring(L"[\"") + member.name + std::wstring(L"\"]");
                                    walkPathsForNode(member.node.get(),
                                                     pathToNode + navPath,
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

std::map<std::wstring, double> FIELD_BOOST_VALUES = {
    { L"time", 1.2 },
    { L"timestamp", 1.2 },
    { L"id", 1.2 },
    { L"name", 1.2 },
    { L"title", 1.2 },
    { L"description", 1.2 }
};

double calculateBoostForField(const std::wstring &field) {
    wstringstream parsed(field);
    double ret = 1;
    std::wstring tok;
    while(std::getline(parsed, tok, L'.')) {
        auto iter = FIELD_BOOST_VALUES.find(tok);
        if(iter != FIELD_BOOST_VALUES.end()) {
            ret *= iter->second;
        }
    }
    
    return ret;
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
        walkPathsForNode(sampledNode, L"@", 3, [&valuesForFields, &maxVals](const std::wstring &path, const json::Node *node) {
            std::wstring nodeStr;
            node->calculateJsonTextRepresentation(nodeStr);
            std::vector<std::wstring> &valsForField = valuesForFields[path];
            valsForField.push_back(nodeStr);
            if(valsForField.size() > maxVals) {
                maxVals = valsForField.size();
            }
        });
    });
        
    // Compute scores
    struct field_stats_record {
        std::wstring field;
        int nOccurences;
        double entropy;
        double boost;
        
        double score() const { return pow(nOccurences, 1.5) * entropy * boost; }
        
        std::wstring debugDescription() const {
            wstringstream ret;
            ret << left << std::setw(40) << field
                << std::setw(6) << nOccurences
                << std::setw(6) <<  std::setprecision(2) << entropy
                << std::setw(6) <<  std::setprecision(2) << boost
            << L"[" << std::setw(6) <<  std::setprecision(2) << score() << L"]";
                
            
            return ret.str();
        }
    };
    
    std::vector<field_stats_record> field_stats;
    std::transform(valuesForFields.begin(), valuesForFields.end(), std::back_inserter(field_stats), [](const decltype(valuesForFields)::value_type &e) {
        field_stats_record ret = {
            e.first,
            (int)e.second.size(),
            calculateEntropy(e.second.begin(), e.second.end()),
            calculateBoostForField(e.first)
        };
        
        return ret;
    });
    
    std::sort(field_stats.begin(), field_stats.end(), [](field_stats_record &field_a, field_stats_record &field_b) {
        return field_b.score() < field_a.score();
    });

    std::vector<std::wstring> results;
    std::transform(field_stats.begin(), field_stats.end(), std::back_inserter(results), [](const field_stats_record &e) {
        NSLog(@"%ls\n", (const unichar*)e.debugDescription().c_str());
        return e.field;
    });
    
    return results;
}


std::vector<pair<std::wstring, size_t>> suggestRowSelectors(Node *parent, const std::wstring &basePath) {
    std::vector<pair<std::wstring, size_t>> scoredArrays;
    
    walkPathsForNode(parent, L"$", 5, [&scoredArrays] (const std::wstring &path, const json::Node *node) {
        const json::ArrayNode *array = dynamic_cast<const json::ArrayNode*>(node);
        if(array) {
            scoredArrays.push_back(make_pair(path + L"[*]", array->getChildCount()));
        }
    });
    
    std::sort(scoredArrays.begin(), scoredArrays.end(), [](const pair<std::wstring, size_t>&left, const pair<std::wstring, size_t>&right) {
        return left.second > right.second;
    });
    
    return scoredArrays;
}
