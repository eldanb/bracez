//
//  JsonPathExpressionNode.cpp
//  Bracez
//
//  Created by Eldan Ben Haim on 30/01/2021.
//

#include "JsonPathExpressionNode.hpp"
#include "JsonPathExpressionCompiler.hpp"
#include <math.h>
#include <numeric>

using convert_type = std::codecvt_utf8<wchar_t>;
static std::wstring_convert<convert_type, wchar_t> wide_utf8_converter;



bool JsonPathExpressionNodeEvalResult::getTruthValue() const {
    if(!nodeList.size()) {
        return false;
    }
    
    json::Node *node = nodeList.front();
    if(!node) {
        return false;
    }
    
    json::NumberNode *numNode = dynamic_cast<json::NumberNode*>(node);
    if(numNode && !numNode->GetValue()) {
        return false;
    }

    json::StringNode *strNode = dynamic_cast<json::StringNode*>(node);
    if(strNode && !strNode->GetValue().size()) {
        return false;
    }

    json::BooleanNode *boolNode = dynamic_cast<json::BooleanNode*>(node);
    if(boolNode && !boolNode->GetValue()) {
        return false;
    }

    json::NullNode *nullNode = dynamic_cast<json::NullNode*>(node);
    if(nullNode) {
        return false;
    }

    return true;
}

double JsonPathExpressionNodeEvalResult::getNumericValue() const {
    if(!nodeList.size()) {
        throw JsonPathEvalError("Invalid numeric value");
    }
    
    json::Node *node = nodeList.front();
    if(!node) {
        throw JsonPathEvalError("Invalid numeric value");
    }
    
    json::NumberNode *numNode = dynamic_cast<json::NumberNode*>(node);
    if(!numNode) {
        throw JsonPathEvalError("Invalid numeric value");
    }

    return numNode->GetValue();
}

JsonPathExpressionNodeEvalResult JsonPathExpressionNodeEvalResult::booleanResult(bool r) {
    JsonPathExpressionNodeEvalResult ret;
    
    json::BooleanNode *b = new json::BooleanNode(r);
    
    ret.localOwners.push_back(std::shared_ptr<json::Node>(b));
    ret.nodeList.push_back(b);
    
    return ret;
}

JsonPathExpressionNodeEvalResult JsonPathExpressionNodeEvalResult::nullResult() {
    JsonPathExpressionNodeEvalResult ret;
    
    json::NullNode *b = new json::NullNode();
    
    ret.localOwners.push_back(std::shared_ptr<json::Node>(b));
    ret.nodeList.push_back(b);
    
    return ret;
}

JsonPathExpressionNodeEvalResult JsonPathExpressionNodeEvalResult::doubleResult(double result) {
    JsonPathExpressionNodeEvalResult ret;
    
    json::NumberNode *n = new json::NumberNode(result);
    
    ret.localOwners.push_back(std::shared_ptr<json::Node>(n));
    ret.nodeList.push_back(n);
    
    return ret;
}

JsonPathExpressionNodeEvalResult JsonPathExpressionNodeEvalResult::stringResult(const std::wstring &result) {
    JsonPathExpressionNodeEvalResult ret;
    
    json::StringNode *n = new json::StringNode(result);
    
    ret.localOwners.push_back(std::shared_ptr<json::Node>(n));
    ret.nodeList.push_back(n);
    
    return ret;
}

JsonPathExpressionNodeEvalResult JsonPathExpressionNodeEvalResult::nonOwnedNodeResult(json::Node *node) {
    JsonPathExpressionNodeEvalResult ret;
    ret.nodeList.push_back(node);
    return ret;
}

JsonPathExpressionNodeEvalResult JsonPathExpressionNodeEvalResult::ownedNodeResult(json::Node *node) {
    JsonPathExpressionNodeEvalResult ret = JsonPathExpressionNodeEvalResult::nonOwnedNodeResult(node);
    ret.localOwners.push_back(shared_ptr<json::Node>(node));
    return ret;
}

JsonPathExpressionNodeNavPipeline::JsonPathExpressionNodeNavPipeline(bool contextBound)
: _contextBound(contextBound)
{
}
    
void JsonPathExpressionNodeNavPipeline::addStep(std::unique_ptr<JsonPathExpressionNodeNavPipelineStep> &&node) {
    _nodes.push_back(std::move(node));
}
    
void JsonPathExpressionNodeNavPipeline::inspect(std::ostream &out) const {
    out << "Navigate (from " << (_contextBound ? "context" : "root") << "): [ ";
    std::for_each(_nodes.begin(),
                  _nodes.end(),
                  [&out](const std::unique_ptr<JsonPathExpressionNodeNavPipelineStep> &n) {
                        n->inspect(out);
                        out << "; ";
                  });
    out << "]";
}


JsonPathExpressionNodeEvalResult JsonPathExpressionNodeNavPipeline::evaluate(JsonPathExpressionNodeEvalContext &context) {
    JsonPathExpressionNodeEvalResult result;
    if(_contextBound) {
        result.nodeList.push_back(context.contextNode);
    } else {
        result.nodeList.push_back(context.rootNode);
    }
    
    std::for_each(_nodes.begin(),
                  _nodes.end(),
                  [&result, &context](const std::unique_ptr<JsonPathExpressionNodeNavPipelineStep> &n) {
                        n->stepFromNode(context, result);
                  });

    return result;
}


JsonPathExpressionNodeNavRecurse::JsonPathExpressionNodeNavRecurse(const std::wstring &name)
: _name(name), _wildcard(false) {
    
}

JsonPathExpressionNodeNavRecurse::JsonPathExpressionNodeNavRecurse()
: _wildcard(true) {
    
}


void JsonPathExpressionNodeNavRecurse::inspect(std::ostream &out) const {
    out << "Fetch any decsendant named '" << wide_utf8_converter.to_bytes(_name) << "'";
}

void JsonPathExpressionNodeNavRecurse::stepFromNode(JsonPathExpressionNodeEvalContext &context, JsonPathExpressionNodeEvalResult &nodes) {
    JsonPathResultNodeList resultList;
    JsonPathResultNodeList bfsQueue;
    
    while(nodes.nodeList.size()) {
        // Get current node
        json::Node *node = nodes.nodeList.front();
        nodes.nodeList.pop_front();
        
        // Add children to queue (for BFS)
        json::ContainerNode *containerNode = dynamic_cast<json::ContainerNode*>(node);
        if(containerNode) {
            for(int childIdx=0; childIdx<containerNode->GetChildCount(); childIdx++) {
                nodes.nodeList.push_back(containerNode->GetChildAt(childIdx));
            }
        }
        
        if(_wildcard) {
            resultList.push_back(node);
        } else {
            // Resolve name
            json::ObjectNode *obNode = dynamic_cast<json::ObjectNode*>(node);
            int idx = -1;
            if(obNode) {
                idx = obNode->GetIndexOfMemberWithName(_name);
            }
            if(idx != -1) {
                resultList.push_back(obNode->GetChildAt(idx));
            }
        }
    }

    nodes.nodeList = std::move(resultList);
}


JsonPathExpressionNodeNavResolveName::JsonPathExpressionNodeNavResolveName(const std::wstring &name)
: _name(name) {
   
}

void JsonPathExpressionNodeNavResolveName::inspect(std::ostream &out) const {
    out << "Navigate to '" << wide_utf8_converter.to_bytes(_name) << "'";
}

void JsonPathExpressionNodeNavResolveName::stepFromNode(JsonPathExpressionNodeEvalContext &context, JsonPathExpressionNodeEvalResult &nodes) {
    JsonPathResultNodeList resultList;
    if(!(context.options && context.options->fuzzy)) {
        std::for_each(nodes.nodeList.begin(),
                      nodes.nodeList.end(),
                      [this, &resultList](json::Node* node) {
                            json::ObjectNode *obNode = dynamic_cast<json::ObjectNode*>(node);
                            int idx = -1;
                            if(obNode) {
                                idx = obNode->GetIndexOfMemberWithName(_name);
                            }
                            if(idx != -1) {
                                resultList.push_back(obNode->GetChildAt(idx));
                            }
                      });
        
    } else {
        std::for_each(nodes.nodeList.begin(),
                      nodes.nodeList.end(),
                      [this, &resultList](json::Node* node) {
                            json::ObjectNode *obNode = dynamic_cast<json::ObjectNode*>(node);
                            if(obNode) {
                                int childCount = obNode->GetChildCount();
                                for(int idx=0; idx<childCount; idx++) {
                                    if(obNode->GetMemberNameAt(idx).substr(0, _name.length()) == _name) {
                                        resultList.push_back(obNode->GetChildAt(idx));
                                    }
                                }
                            }
                      });
    }
        

    nodes.nodeList = std::move(resultList);
}



JsonPathExpressionNodeNavParent::JsonPathExpressionNodeNavParent(bool allAncestors)
: _allAncestors(allAncestors) {
    
}
    
void JsonPathExpressionNodeNavParent::inspect(std::ostream &out) const {
    out << "Navigate to " << (_allAncestors ? "ancestors" : "parent");
}

void JsonPathExpressionNodeNavParent::stepFromNode(JsonPathExpressionNodeEvalContext &context, JsonPathExpressionNodeEvalResult &nodes) {
    JsonPathResultNodeList resultList;
    std::for_each(nodes.nodeList.begin(),
                  nodes.nodeList.end(),
                  [this, &resultList](json::Node* node) {
                        if(node->GetParent()) {
                            node = node->GetParent();
                            resultList.push_back(node);
                        }
        
                        if(_allAncestors) {
                            while(node->GetParent()) {
                                node = node->GetParent();
                                resultList.push_back(node);
                            }
                        }
                  });
    nodes.nodeList = std::move(resultList);
}


JsonPathExpressionNodeNavAllChildren::JsonPathExpressionNodeNavAllChildren()
{
   
}

void JsonPathExpressionNodeNavAllChildren::inspect(std::ostream &out) const {
    out << "Fetch all children";
}

void JsonPathExpressionNodeNavAllChildren::stepFromNode(JsonPathExpressionNodeEvalContext &context, JsonPathExpressionNodeEvalResult &nodes) {
    JsonPathResultNodeList resultList;
    std::for_each(nodes.nodeList.begin(),
                  nodes.nodeList.end(),
                  [&resultList](json::Node* node) {
                        json::ContainerNode *contNode = dynamic_cast<json::ContainerNode*>(node);
                        if(contNode) {
                            int ccount = contNode->GetChildCount();
                            for(int idx=0; idx<ccount; idx++) {
                                resultList.push_back(contNode->GetChildAt(idx));
                            }
                        }
                  });
    nodes.nodeList = std::move(resultList);
}


JsonPathExpressionNodeContextMemberName::JsonPathExpressionNodeContextMemberName() {
    
}
    
void JsonPathExpressionNodeContextMemberName::inspect(std::ostream &out) const {
    out << "Fetch current node name";
}

JsonPathExpressionNodeEvalResult JsonPathExpressionNodeContextMemberName::evaluate(JsonPathExpressionNodeEvalContext &context) {
    if(context.contextNode) {
        int childIndex = context.contextNode->GetParent()->GetIndexOfChild(context.contextNode);
        if(json::ObjectNode *onode = dynamic_cast<json::ObjectNode*>(context.contextNode->GetParent())) {
            return JsonPathExpressionNodeEvalResult::stringResult(onode->GetMemberNameAt(childIndex));
        } else
        {
            return JsonPathExpressionNodeEvalResult::doubleResult(childIndex);
        }
    }
    
    return JsonPathExpressionNodeEvalResult::nullResult();
}


JsonPathExpressionNodeNavIndexArray::JsonPathExpressionNodeNavIndexArray(int index)
: _index(index) {
   
}

void JsonPathExpressionNodeNavIndexArray::inspect(std::ostream &out) const {
    out << "Access index " << _index;
}

void JsonPathExpressionNodeNavIndexArray::stepFromNode(JsonPathExpressionNodeEvalContext &context,
                                                       JsonPathExpressionNodeEvalResult &nodes) {
    JsonPathResultNodeList resultList;
    std::for_each(nodes.nodeList.begin(),
                  nodes.nodeList.end(),
                  [this, &resultList](json::Node* node) {
                        json::ArrayNode *arrNode = dynamic_cast<json::ArrayNode*>(node);
                        if(arrNode && _index < arrNode->GetChildCount()) {
                            resultList.push_back(arrNode->GetChildAt(_index));
                        }
                  });
    nodes.nodeList = std::move(resultList);
}

JsonPathExpressionNodeNavSliceArray::JsonPathExpressionNodeNavSliceArray(int startIndex, bool startInverted, int endIndex, bool endInverted)
    : _startIndex(startIndex), _startInverted(startInverted), _endIndex(endIndex), _endInverted(endInverted) {
}
    
void JsonPathExpressionNodeNavSliceArray::inspect(std::ostream &out) const {
    out << "Slice array from " << (_startInverted ? "-" : "") << _startIndex
        << " to " << (_endInverted ? "-": "") << _endIndex;
}

void JsonPathExpressionNodeNavSliceArray::stepFromNode(JsonPathExpressionNodeEvalContext &context,
                                                       JsonPathExpressionNodeEvalResult &nodes) {
    JsonPathResultNodeList resultList;
    std::for_each(nodes.nodeList.begin(),
                  nodes.nodeList.end(),
                  [this, &resultList](json::Node* node) {
                        json::ArrayNode *arrNode = dynamic_cast<json::ArrayNode*>(node);
                        if(arrNode) {
                            int childCount = arrNode->GetChildCount();
                            
                            int start = _startIndex;
                            int end = _endIndex;
                            
                            if(_startInverted) start = childCount - start;
                            if(_endInverted) end = childCount - end;
                            
                            for(int idx=start; idx < end && idx < childCount; idx++) {
                                resultList.push_back(arrNode->GetChildAt(idx));
                            }
                        }
                  });
    nodes.nodeList = std::move(resultList);
}


JsonPathExpressionNodeNavIndexArrayByList::JsonPathExpressionNodeNavIndexArrayByList(const std::list<int> &indexList)
    : _indexList(indexList)
{
   
}

void JsonPathExpressionNodeNavIndexArrayByList::inspect(std::ostream &out) const {
    out << "Access " <<  _indexList.size() << " indices";
}

void JsonPathExpressionNodeNavIndexArrayByList::stepFromNode(JsonPathExpressionNodeEvalContext &context,
                                                             JsonPathExpressionNodeEvalResult &nodes) {
    JsonPathResultNodeList resultList;
    std::for_each(nodes.nodeList.begin(),
                  nodes.nodeList.end(),
                  [this, &resultList](json::Node* node) {
                        json::ArrayNode *arrNode = dynamic_cast<json::ArrayNode*>(node);
                        if(arrNode) {
                            int arrLen = arrNode->GetChildCount();
                            std::for_each(_indexList.begin(),
                                          _indexList.end(),
                                          [arrNode, arrLen, &resultList](int index) {
                                            if(index < arrLen) {
                                                resultList.push_back(arrNode->GetChildAt(index));
                                            }
                                           });
                        }
                  });
    nodes.nodeList = std::move(resultList);
}


JsonPathExpressionNodeFilter::JsonPathExpressionNodeFilter(std::unique_ptr<JsonPathExpressionNode> &&filter, bool filterChildren)
    : _filter(std::move(filter)), _filterChildren(filterChildren)
{
   
}


void JsonPathExpressionNodeFilter::stepFromNode(JsonPathExpressionNodeEvalContext &context,
                                                        JsonPathExpressionNodeEvalResult &nodes) {
    JsonPathResultNodeList resultList;
    std::for_each(nodes.nodeList.begin(),
                  nodes.nodeList.end(),
                  [this, &context, &resultList](json::Node* node) {
                            if(_filterChildren) {
                                json::ContainerNode *contNode = dynamic_cast<json::ContainerNode*>(node);
                                if(contNode) {
                                    int ccount = contNode->GetChildCount();
                                    for(int idx=0; idx<ccount; idx++) {
                                        json::Node *oldContext = context.contextNode;
                                        context.contextNode = contNode->GetChildAt(idx);
                                        JsonPathExpressionNodeEvalResult filterRes = _filter->evaluate(context);
                                        
                                        if(filterRes.getTruthValue()) {
                                            resultList.push_back(context.contextNode);
                                        }
                                        
                                        context.contextNode = oldContext;
                                    }
                                }
                            } else {
                                json::Node *oldContext = context.contextNode;
                                context.contextNode = node;
                                JsonPathExpressionNodeEvalResult filterRes = _filter->evaluate(context);
                                    
                                if(filterRes.getTruthValue()) {
                                    resultList.push_back(context.contextNode);
                                }
                                    
                                context.contextNode = oldContext;
                            }
                        }
                  );
    nodes.nodeList = std::move(resultList);
}

void JsonPathExpressionNodeFilter::inspect(std::ostream &out) const {
    out << "Filter " << (_filterChildren ? "children " : "") << "by [ ";
    _filter->inspect(out);
    out << " ]";
}
    


JsonPathExpressionNodeIndexByExpression::JsonPathExpressionNodeIndexByExpression(std::unique_ptr<JsonPathExpressionNode> &&filter)
    : _filter(std::move(filter))
{
   
}

void JsonPathExpressionNodeIndexByExpression::stepFromNode(JsonPathExpressionNodeEvalContext &context,
                                                           JsonPathExpressionNodeEvalResult &nodes) {
    json::Node *oldContext = context.contextNode;

    JsonPathResultNodeList resultList;
    std::for_each(nodes.nodeList.begin(),
                  nodes.nodeList.end(),
                  [this, &context, &resultList](json::Node* node) {
                        json::ArrayNode *arrNode = dynamic_cast<json::ArrayNode*>(node);
                        if(arrNode) {
                            context.contextNode = arrNode;
                            int filterRes = (int)_filter->evaluate(context).getNumericValue();
                            if(filterRes < arrNode->GetChildCount() && filterRes >= 0) {
                                resultList.push_back(arrNode->GetChildAt(filterRes));
                            }
                        }
                  });
    
    nodes.nodeList = std::move(resultList);
    context.contextNode = oldContext;
}


void JsonPathExpressionNodeIndexByExpression::inspect(std::ostream &out) const {
    out << "Index by [ ";
    _filter->inspect(out);
    out << " ]";
}
   
JsonPathExpressionNodeBinaryOp::JsonPathExpressionNodeBinaryOp(std::unique_ptr<JsonPathExpressionNode> &&leftMost, std::list<binary_operator_and_operand> &operands)
    : _leftMost(std::move(leftMost)), _operandList(std::move(operands))
{
   
}

void JsonPathExpressionNodeBinaryOp::inspect(std::ostream &out) const {
    out << "Binary operator chain [ ";
    
    _leftMost->inspect(out); out << "; ";
    std::for_each(_operandList.begin(), _operandList.end(),
                  [&out](const binary_operator_and_operand &n) {
        std::get<1>(n)->inspect(out);
        out << "; ";
    });
    out << "]";
}

JsonPathExpressionNodeEvalResult JsonPathExpressionNodeBinaryOp::evaluate(JsonPathExpressionNodeEvalContext &context) {
    JsonPathExpressionNodeEvalResult curLeftResult = _leftMost->evaluate(context);
    std::for_each(_operandList.begin(),
                  _operandList.end(),
                  [&curLeftResult, &context](const binary_operator_and_operand &oporAndOpand) {
                    JsonPathExpressionNodeEvalResult curRightResult = std::get<1>(oporAndOpand)->evaluate(context);
                    curLeftResult = std::get<0>(oporAndOpand)(curLeftResult, curRightResult);
                  });
    return curLeftResult;
}

JsonPathExpressionNodeNegateOp::JsonPathExpressionNodeNegateOp(std::unique_ptr<JsonPathExpressionNode> &&operand)
    : _operand(std::move(operand))
{
   
}

void JsonPathExpressionNodeNegateOp::inspect(std::ostream &out) const {
    out << "Negate [ ";
    
    _operand->inspect(out); out << "; ";
    out << "]";
}

JsonPathExpressionNodeEvalResult JsonPathExpressionNodeNegateOp::evaluate(JsonPathExpressionNodeEvalContext &context) {
    return JsonPathExpressionNodeEvalResult::booleanResult(!_operand->evaluate(context).getTruthValue());
}

JsonPathExpressionNodeJsonLiteral::JsonPathExpressionNodeJsonLiteral(std::unique_ptr<json::Node> literal)
    : _literal(std::move(literal))
{
   
}


void JsonPathExpressionNodeJsonLiteral::inspect(std::ostream &out) const {
    std::wstring jsonLiteralAsString;
    _literal->CalculateJsonTextRepresentation(jsonLiteralAsString);
    
    using convert_type = std::codecvt_utf8<wchar_t>;
    std::wstring_convert<convert_type, wchar_t> converter;
    
    out << "JSON literal [ ";
    out << converter.to_bytes(jsonLiteralAsString);
    out << "]";
}

JsonPathExpressionNodeEvalResult JsonPathExpressionNodeJsonLiteral::evaluate(JsonPathExpressionNodeEvalContext &context) {
    JsonPathExpressionNodeEvalResult ret;
    ret.nodeList.push_back(_literal.get());
    ret.localOwners.push_back(_literal);
    return ret;
}


JsonPathExpressionNodeFunctionInvoke::JsonPathExpressionNodeFunctionInvoke(JsonPathExpressionFunction *fn,  std::list<std::unique_ptr<JsonPathExpressionNode>> &&args)
 : _fn(fn), _args(std::move(args))
{
    
}
    
void JsonPathExpressionNodeFunctionInvoke::inspect(std::ostream &out) const {
    
}

JsonPathExpressionNodeEvalResult JsonPathExpressionNodeFunctionInvoke::evaluate(JsonPathExpressionNodeEvalContext &context) {
    std::list<JsonPathExpressionNodeEvalResult> evaledArgs;
    
    std::transform(_args.begin(), _args.end(), back_inserter(evaledArgs), [&context](std::unique_ptr<JsonPathExpressionNode>& arg) {
        return arg->evaluate(context);
    });
    
    return _fn->invoke(evaledArgs);
}

namespace JsonPathExpressionNodeBinaryOperators {

    expr_operand_value logicalOr(expr_operand_value opLeft,  expr_operand_value opRight) {
        if(opLeft.getTruthValue()) {
            return opLeft;
        } else {
            return opRight;
        }
    }

    expr_operand_value logicalAnd(expr_operand_value opLeft,  expr_operand_value opRight) {
        if(opLeft.getTruthValue()) {
            return opRight;
        } else {
            return opLeft;
        }
    }

    bool isEqualInternal(expr_operand_value opLeft,  expr_operand_value opRight) {
        JsonPathResultNodeList &leftNodes = opLeft.nodeList;
        JsonPathResultNodeList &rightNodes = opRight.nodeList;
        
        if(leftNodes.size() != rightNodes.size()) {
            return false;
        }
        
        for(auto liter = leftNodes.begin(),
                 riter = rightNodes.begin();
            liter != leftNodes.end() && riter != rightNodes.end();
            liter++, riter++) {
            if(!(*liter)->ValueEquals(*riter)) {
                return false;
            }
        }
        
        
        return true;
    }

    expr_operand_value relEq(expr_operand_value opLeft,  expr_operand_value opRight) {
        return expr_operand_value::booleanResult(isEqualInternal(opLeft, opRight));
    }

    expr_operand_value relNeq(expr_operand_value opLeft,  expr_operand_value opRight) {
        return expr_operand_value::booleanResult(!isEqualInternal(opLeft, opRight));
    }

    expr_operand_value relGte(json::Node *left, json::Node *right) {
        return expr_operand_value::booleanResult(right->ValueLt(left) || right->ValueEquals(left));
    }

    expr_operand_value relLte(json::Node *left, json::Node *right) {
        return expr_operand_value::booleanResult(left->ValueLt(right) || left->ValueEquals(right));
    }

    expr_operand_value relGt(json::Node *left, json::Node *right) {
        return expr_operand_value::booleanResult(right->ValueLt(left));
    }

    expr_operand_value relLt(json::Node *left, json::Node *right) {
        return expr_operand_value::booleanResult(left->ValueLt(right));
    }

    inline bool relInInternal(json::Node *left, json::ArrayNode *right) {
        return find_if(right->Begin(), right->End(), [left](unique_ptr<json::Node> &n) {
            return n->ValueEquals(left);
        }) != right->End();
    }

    expr_operand_value relIn(json::Node *left, json::ArrayNode *right) {
        return expr_operand_value::booleanResult(relInInternal(left, right));
    }

    expr_operand_value relNotIn(json::Node *left, json::ArrayNode *right) {
        return expr_operand_value::booleanResult(!relInInternal(left, right));
    }
    
    expr_operand_value relSubsetOf(json::ArrayNode *left, json::ArrayNode *right) {
        bool notSubset = std::find_if_not(left->Begin(), left->End(), [right](unique_ptr<json::Node> &node) {
            return relInInternal(node.get(), right);
        }) != left->End();
        
        return expr_operand_value::booleanResult(!notSubset);
    }
    
    inline bool relAnyOfInternal(json::ArrayNode *left, json::ArrayNode *right) {
        return std::find_if(left->Begin(), left->End(), [right](unique_ptr<json::Node> &node ) {
            return relInInternal(node.get(), right);
        }) != left->End();

    }
    expr_operand_value relAnyOf(json::ArrayNode *left, json::ArrayNode *right) {
        return expr_operand_value::booleanResult(relAnyOfInternal(left, right));
    }
    
    expr_operand_value relNoneOf(json::ArrayNode *left, json::ArrayNode *right) {
        return expr_operand_value::booleanResult(!relAnyOfInternal(left, right));
    }

    expr_operand_value div(json::NumberNode *opLeft,  json::NumberNode *opRight) {
        return expr_operand_value::doubleResult(opLeft->GetValue() / opRight->GetValue());
    }

    expr_operand_value mod(json::NumberNode *opLeft,  json::NumberNode *opRight) {
        return expr_operand_value::doubleResult((long)opLeft->GetValue() % (long)opRight->GetValue());
    }


    expr_operand_value mul(json::NumberNode *opLeft,  json::NumberNode *opRight) {
        return expr_operand_value::doubleResult(opLeft->GetValue() * opRight->GetValue());
    }
    
    expr_operand_value add(expr_operand_value opLeft,  expr_operand_value opRight) {
        if(opLeft.nodeList.size() == 1 && opRight.nodeList.size() == 1) {
            json::NumberNode *numLeft = dynamic_cast<json::NumberNode*>(opLeft.nodeList.front());
            json::NumberNode *numRight = dynamic_cast<json::NumberNode*>(opRight.nodeList.front());
            if(numLeft && numRight) {
                return expr_operand_value::doubleResult(numLeft->GetValue() + numRight->GetValue());
            } else
            if(json::StringNode *strLeft = dynamic_cast<json::StringNode*>(opLeft.nodeList.front())) {
                return expr_operand_value::stringResult(strLeft->GetValue() + opRight.nodeList.front()->ToString());
            } else
            if(json::ArrayNode *arrLeft = dynamic_cast<json::ArrayNode*>(opLeft.nodeList.front())) {
                json::ArrayNode *arrResult = arrLeft->clone();
                
                if(json::ArrayNode *arrRight = dynamic_cast<json::ArrayNode*>(opRight.nodeList.front())) {
                    for(auto iter = arrRight->Begin(); iter != arrRight->End(); iter++) {
                        arrResult->DomAddElementNode((*iter)->clone());
                    }
                } else {
                    arrResult->DomAddElementNode(arrRight->clone());
                }
                
                expr_operand_value ret;
                ret.nodeList.push_back(arrResult);
                ret.localOwners.push_back(shared_ptr<json::Node>(arrResult));
            }
        }
         
        return expr_operand_value::nullResult();
    }
    
    expr_operand_value subtract(json::NumberNode *opLeft,  json::NumberNode *opRight) {
        return expr_operand_value::doubleResult(opLeft->GetValue() - opRight->GetValue());
    }
}


template<class T>
JsonPathExpressionNodeEvalResult convertToJsonPathResult(const T &&from) {
    throw JsonPathEvalError("Cannot convert return type");
}

template<>
JsonPathExpressionNodeEvalResult convertToJsonPathResult<JsonPathExpressionNodeEvalResult>(const JsonPathExpressionNodeEvalResult &&from) {
    return from;
}

template<>
JsonPathExpressionNodeEvalResult convertToJsonPathResult<double>(const double &&from) {
    return JsonPathExpressionNodeEvalResult::doubleResult(from);
}

template<class ReturnType, class ... FuncArgs>
class JsonPathExpressionFunctionAdapter: public JsonPathExpressionFunction {
public:
    typedef std::function<ReturnType (FuncArgs...)> FunctionType;
    
    JsonPathExpressionFunctionAdapter(const FunctionType &f): _function(f) {}
    JsonPathExpressionFunctionAdapter(JsonPathExpressionFunctionAdapter<ReturnType, FuncArgs...> &&o): _function(o.f) {}
        
    virtual JsonPathExpressionNodeEvalResult invoke(const std::list<JsonPathExpressionNodeEvalResult> &args) {
        auto iter = args.begin();
        return convertToJsonPathResult(
                call_builder<sizeof...(FuncArgs),
                            decltype(iter),
                            FunctionType>::call(_function, iter, args.end()));
    }
    
    virtual int arity() {
        return sizeof...(FuncArgs);
    }
    
private:
    template<size_t AC, class Iter, class Func, class ... ArgTypes>
    class call_builder {
    public:
        inline static typename Func::result_type call(const Func &f, Iter &s, const Iter &e, const ArgTypes& ... args) {
            return call_builder<AC-1, Iter, Func, ArgTypes..., typename Iter::value_type>::call(f, s, e, args..., *(s++));
        }
    };

    template<class Iter, class Func, class ... ArgTypes>
    class call_builder<0, Iter, Func, ArgTypes...> {
    public:
        inline static typename Func::result_type call(const Func &f, Iter &s, const Iter &e, const ArgTypes& ... args) {
            if(s != e) {
                throw JsonPathEvalError("Improper number of arguments passed to function.");
            }
            
            return f((FuncArgs)(args)...);
        }
    };
    
    FunctionType _function;
};


template<class R, class ... A>
JsonPathExpressionFunction* AdaptToJsonPathExpressionFunction(const std::function<R (A...)> &f)  {
    return new JsonPathExpressionFunctionAdapter<R, A...>(f);
}

template<class R, class ... A>
JsonPathExpressionFunction* AdaptToJsonPathExpressionFunction(R (*f)(A...))  {
    return new JsonPathExpressionFunctionAdapter<R, A...>(f);
}

namespace json_path_functions {
    double length(const JsonPathExpressionNodeEvalResult &input) {
        if(input.nodeList.size() == 1) {
            if(const json::ContainerNode *container = dynamic_cast<json::ContainerNode*>(input.nodeList.front())) {
                return container->GetChildCount();
            } else
            if(const json::StringNode *stringNode = dynamic_cast<json::StringNode*>(input.nodeList.front())) {
                return stringNode->GetValue().size();
            }
        }
        
        return -1;
    }

    JsonPathExpressionNodeEvalResult min(const json::ArrayNode *input) {
        if(!input) {
            return JsonPathExpressionNodeEvalResult::nullResult();
        }
        
        auto retElement = std::min_element(input->Begin(), input->End(), [](const std::unique_ptr<json::Node> &a, const std::unique_ptr<json::Node> &b) {
            return a->ValueLt(b.get());
        });
        
        return JsonPathExpressionNodeEvalResult::ownedNodeResult((*retElement)->clone());
    }

    JsonPathExpressionNodeEvalResult max(const json::ArrayNode *input) {
        if(!input) {
            return JsonPathExpressionNodeEvalResult::nullResult();
        }

        auto retElement = std::max_element(input->Begin(), input->End(), [](const std::unique_ptr<json::Node> &a, const std::unique_ptr<json::Node> &b) {
            return a->ValueLt(b.get());
        });
        
        return JsonPathExpressionNodeEvalResult::ownedNodeResult((*retElement)->clone());
    }

    inline double internalSum(const json::ArrayNode *input) {
        return std::reduce(input->Begin(), input->End(), (double)0, [](double s, const std::unique_ptr<json::Node> &n) {
            if(json::NumberNode *num = dynamic_cast<json::NumberNode*>(n.get())) {
                return s + num->GetValue();
            } else {
                return s;
            }
        });
    }

    JsonPathExpressionNodeEvalResult sum(const json::ArrayNode *input) {
        if(!input) {
            return JsonPathExpressionNodeEvalResult::nullResult();
        }
        
        return JsonPathExpressionNodeEvalResult::doubleResult(internalSum(input));
    }

    JsonPathExpressionNodeEvalResult avg(const json::ArrayNode *input) {
        if(!input) {
            return JsonPathExpressionNodeEvalResult::nullResult();
        }
        
        return JsonPathExpressionNodeEvalResult::doubleResult(internalSum(input) / (double)input->GetChildCount());
    }

    JsonPathExpressionNodeEvalResult toarray(const JsonPathExpressionNodeEvalResult &input) {
        std::unique_ptr<json::ArrayNode> result = make_unique<json::ArrayNode>();
        std::for_each(input.nodeList.begin(), input.nodeList.end(), [&result](json::Node* node) {
            result->DomAddElementNode(node->clone());
        });
        
        return JsonPathExpressionNodeEvalResult::ownedNodeResult(result.release());
    }
}

std::map<std::string, JsonPathExpressionFunction*> JsonPathExpressionFunction::functions {
    { "cos", AdaptToJsonPathExpressionFunction<double,double>(::cos) },
    { "sin", AdaptToJsonPathExpressionFunction<double,double>(::sin) },
    { "tan", AdaptToJsonPathExpressionFunction<double,double>(::tan) },
    { "acos", AdaptToJsonPathExpressionFunction<double,double>(::acos) },
    { "asin", AdaptToJsonPathExpressionFunction<double,double>(::asin) },
    { "atan", AdaptToJsonPathExpressionFunction<double,double>(::atan) },
    { "log", AdaptToJsonPathExpressionFunction<double,double>(::log) },
    { "exp", AdaptToJsonPathExpressionFunction<double,double>(::exp) },
    { "pow", AdaptToJsonPathExpressionFunction<double,double,double>(::pow) },
    { "toarray", AdaptToJsonPathExpressionFunction(json_path_functions::toarray) },
    { "length", AdaptToJsonPathExpressionFunction(json_path_functions::length) },
    { "min", AdaptToJsonPathExpressionFunction(json_path_functions::min) },
    { "max", AdaptToJsonPathExpressionFunction(json_path_functions::max) },
    { "sum", AdaptToJsonPathExpressionFunction(json_path_functions::sum) },
    { "avg", AdaptToJsonPathExpressionFunction(json_path_functions::avg) },
};
