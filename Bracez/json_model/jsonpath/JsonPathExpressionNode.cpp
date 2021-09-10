//
//  JsonPathExpressionNode.cpp
//  Bracez
//
//  Created by Eldan Ben Haim on 30/01/2021.
//

#include "JsonPathExpressionNode.hpp"
#include "JsonPathExpressionCompiler.hpp"

using convert_type = std::codecvt_utf8<wchar_t>;
static std::wstring_convert<convert_type, wchar_t> wide_utf8_converter;


class JsonPathEvalError : public std::exception {
public:
    JsonPathEvalError(const char *what): _what(what) {
        
    }
    
    const char *what() const noexcept {
        return _what;
    }
    
private:
    const char* _what;
} ;


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


JsonPathExpressionNodeFilterChildren::JsonPathExpressionNodeFilterChildren(std::unique_ptr<JsonPathExpressionNode> &&filter)
    : _filter(std::move(filter))
{
   
}


void JsonPathExpressionNodeFilterChildren::stepFromNode(JsonPathExpressionNodeEvalContext &context,
                                                        JsonPathExpressionNodeEvalResult &nodes) {
    JsonPathResultNodeList resultList;
    std::for_each(nodes.nodeList.begin(),
                  nodes.nodeList.end(),
                  [this, &context, &resultList](json::Node* node) {
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
                  });
    nodes.nodeList = std::move(resultList);
}

void JsonPathExpressionNodeFilterChildren::inspect(std::ostream &out) const {
    out << "Filter by [ ";
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
                            double filterRes = _filter->evaluate(context).getNumericValue();
                            if(filterRes < arrNode->GetChildCount()) {
                                resultList.push_back(arrNode->GetChildAt((int)filterRes));
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

    template<class F>
    inline expr_operand_value operate_on_scalars(expr_operand_value opLeft,  expr_operand_value opRight, const F &f) {
        if(opLeft.nodeList.size() == 1 && opRight.nodeList.size() == 1) {
            return f(opLeft.nodeList.front(), opRight.nodeList.front());
        } else {
            return expr_operand_value::nullResult();
        }
    }

    template<class F>
    inline expr_operand_value operate_on_numbers(expr_operand_value opLeft,  expr_operand_value opRight, const F &f) {
        if(opLeft.nodeList.size() == 1 && opRight.nodeList.size() == 1) {
            json::NumberNode *numLeft = dynamic_cast<json::NumberNode*>(opLeft.nodeList.front());
            json::NumberNode *numRight = dynamic_cast<json::NumberNode*>(opRight.nodeList.front());
            if(numLeft && numRight) {
                return expr_operand_value::doubleResult(f(numLeft->GetValue(), numRight->GetValue()));
            }
        }

        return expr_operand_value::nullResult();
    }


    expr_operand_value relGte(expr_operand_value opLeft,  expr_operand_value opRight) {
        return operate_on_scalars(opLeft, opRight, [](json::Node *left, json::Node *right) {
            return expr_operand_value::booleanResult(right->ValueLt(left) || right->ValueEquals(left));
        });
    }

    expr_operand_value relLte(expr_operand_value opLeft,  expr_operand_value opRight) {
        return operate_on_scalars(opLeft, opRight, [](json::Node *left, json::Node *right) {
            return expr_operand_value::booleanResult(left->ValueLt(right) || left->ValueEquals(right));
        });
    }

    expr_operand_value relGt(expr_operand_value opLeft,  expr_operand_value opRight) {
        return operate_on_scalars(opLeft, opRight, [](json::Node *left, json::Node *right) {
            return expr_operand_value::booleanResult(right->ValueLt(left));
        });
    }

    expr_operand_value relLt(expr_operand_value opLeft,  expr_operand_value opRight) {
        return operate_on_scalars(opLeft, opRight, [](json::Node *left, json::Node *right) {
            return expr_operand_value::booleanResult(left->ValueLt(right));
        });
    }

    expr_operand_value div(expr_operand_value opLeft,  expr_operand_value opRight) {
        return operate_on_numbers(opLeft, opRight, [](double left, double right) {
            return left / right;
        });
    }

    expr_operand_value mod(expr_operand_value opLeft,  expr_operand_value opRight) {
        return operate_on_numbers(opLeft, opRight, [](double left, double right) {
            return (long)left % (long)right;
        });
    }


    expr_operand_value mul(expr_operand_value opLeft,  expr_operand_value opRight) {
        return operate_on_numbers(opLeft, opRight, [](double left, double right) {
            return left * right;
        });
    }
    
    expr_operand_value add(expr_operand_value opLeft,  expr_operand_value opRight) {
        if(opLeft.nodeList.size() == 1 && opRight.nodeList.size() == 1) {
            json::NumberNode *numLeft = dynamic_cast<json::NumberNode*>(opLeft.nodeList.front());
            if(numLeft) {
                json::NumberNode *numRight = dynamic_cast<json::NumberNode*>(opRight.nodeList.front());
                if(numRight) {
                    return expr_operand_value::doubleResult(numLeft->GetValue() + numRight->GetValue());
                }
            }
            
            json::StringNode *strLeft = dynamic_cast<json::StringNode*>(opLeft.nodeList.front());
            if(strLeft) {
                return expr_operand_value::stringResult(strLeft->GetValue() + opRight.nodeList.front()->ToString());
            }
        }
         
        return expr_operand_value::nullResult();
    }
    
    expr_operand_value subtract(expr_operand_value opLeft,  expr_operand_value opRight) {
        return operate_on_numbers(opLeft, opRight, [](double left, double right) {
            return left - right;
        });
    }

}
