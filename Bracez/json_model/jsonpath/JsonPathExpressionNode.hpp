//
//  JsonPathExpressionNode.hpp
//  Bracez
//
//  Created by Eldan Ben Haim on 30/01/2021.
//

#ifndef JsonPathExpressionNode_hpp
#define JsonPathExpressionNode_hpp

#include <list>
#include <memory>
#include <string>
#include <sstream>
#include <map>

#include "reader.h"

struct JsonPathExpressionOptions;

struct JsonPathExpressionNodeEvalContext {
    json::Node *rootNode;
    json::Node *contextNode;
    json::Node *cursorNode;
    JsonPathExpressionOptions *options;
} ;

typedef std::list<json::Node *> JsonPathResultNodeList;

class JsonPathExpressionNodeEvalResult {
public:
    JsonPathResultNodeList nodeList;
    bool getTruthValue() const;
    double getNumericValue() const;
    
    inline operator double () const { return getNumericValue(); }
    inline operator bool () const { return getTruthValue(); }
    
    template<class T>
    inline operator T* () const {
        if(nodeList.size()!=1) {
            return NULL;
        }
        
        return dynamic_cast<T*>(nodeList.front());
    }

public:
    std::list<std::shared_ptr<json::Node>> localOwners;
    
public:
    static JsonPathExpressionNodeEvalResult booleanResult(bool r);
    static JsonPathExpressionNodeEvalResult nullResult();
    static JsonPathExpressionNodeEvalResult doubleResult(double result);
    static JsonPathExpressionNodeEvalResult stringResult(const std::wstring &result);
    static JsonPathExpressionNodeEvalResult nonOwnedNodeResult(json::Node *node);
    static JsonPathExpressionNodeEvalResult ownedNodeResult(json::Node *node);
} ;

class JsonPathExpressionNode;

class JsonPathInspectable {
public:
    std::string inspect() const {
        std::stringstream o;
        inspect(o);
        return o.str();
    }
    
    virtual void inspect(std::ostream &out) const = 0;
    
    virtual void pushDirectDependentNodes(std::list<const JsonPathExpressionNode*> &output) const = 0;
};

class JsonPathExpressionNode: public JsonPathInspectable {
public:
    virtual ~JsonPathExpressionNode() {}

    virtual JsonPathExpressionNodeEvalResult evaluate(JsonPathExpressionNodeEvalContext &context) = 0;
    
} ;

class JsonPathExpressionNodeNavPipelineStep: public JsonPathInspectable {
public:
    virtual ~JsonPathExpressionNodeNavPipelineStep() {}
    virtual void stepFromNode(JsonPathExpressionNodeEvalContext &context, JsonPathExpressionNodeEvalResult &node) {} // TODO PURE
    
    virtual void pushDirectDependentNodes(std::list<const JsonPathExpressionNode*> &output) const {}
};

class JsonPathExpressionNodeContextMemberName: public JsonPathExpressionNode  {
    
public:
    JsonPathExpressionNodeContextMemberName();
    
    virtual void inspect(std::ostream &out) const;
    virtual JsonPathExpressionNodeEvalResult evaluate(JsonPathExpressionNodeEvalContext &context);

    virtual void pushDirectDependentNodes(std::list<const JsonPathExpressionNode*> &output) const {};
    
};

class JsonPathExpressionNodeNavPipeline: public JsonPathExpressionNode  {
public:
    enum PipelineStart {
      pipelineStartAtRoot,
      pipelineStartAtContext,
      pipelineStartAtCursor
    };
public:
    JsonPathExpressionNodeNavPipeline(PipelineStart pipelineStart);
    
    void addStep(std::unique_ptr<JsonPathExpressionNodeNavPipelineStep> &&node);
        
    virtual void inspect(std::ostream &out) const;
    virtual JsonPathExpressionNodeEvalResult evaluate(JsonPathExpressionNodeEvalContext &context);

    virtual void pushDirectDependentNodes(std::list<const JsonPathExpressionNode*> &output) const;
    
    PipelineStart getPipelineStart() const { return _pipelineStart; }
private:
    PipelineStart _pipelineStart;
    std::list<std::unique_ptr<JsonPathExpressionNodeNavPipelineStep>> _nodes;
};

class JsonPathExpressionNodeNavRecurse: public JsonPathExpressionNodeNavPipelineStep {
public:
    JsonPathExpressionNodeNavRecurse();
    JsonPathExpressionNodeNavRecurse(const std::wstring &name);
    
    virtual void inspect(std::ostream &out) const;
    virtual void stepFromNode(JsonPathExpressionNodeEvalContext &context, JsonPathExpressionNodeEvalResult &node);
    
private:
    std::wstring _name;
    bool _wildcard;
};

class JsonPathExpressionNodeNavResolveName: public JsonPathExpressionNodeNavPipelineStep {
public:
    JsonPathExpressionNodeNavResolveName(const std::wstring &name);
    
    virtual void inspect(std::ostream &out) const;
    virtual void stepFromNode(JsonPathExpressionNodeEvalContext &context, JsonPathExpressionNodeEvalResult &node);

private:
    std::wstring _name;
};

class JsonPathExpressionNodeNavParent: public JsonPathExpressionNodeNavPipelineStep {
public:
    JsonPathExpressionNodeNavParent(bool allAncestors);
    
    virtual void inspect(std::ostream &out) const;
    virtual void stepFromNode(JsonPathExpressionNodeEvalContext &context, JsonPathExpressionNodeEvalResult &node);

private:
    bool _allAncestors;
};

class JsonPathExpressionNodeNavAllChildren: public JsonPathExpressionNodeNavPipelineStep {
public:
    JsonPathExpressionNodeNavAllChildren();
    
    virtual void inspect(std::ostream &out) const;
    virtual void stepFromNode(JsonPathExpressionNodeEvalContext &context, JsonPathExpressionNodeEvalResult &node);
};

class JsonPathExpressionNodeNavIndexArray: public JsonPathExpressionNodeNavPipelineStep {
public:
    JsonPathExpressionNodeNavIndexArray(int index);
    
    virtual void inspect(std::ostream &out) const;
    virtual void stepFromNode(JsonPathExpressionNodeEvalContext &context, JsonPathExpressionNodeEvalResult &node);

private:
    int _index;
};


class JsonPathExpressionNodeNavSliceArray: public JsonPathExpressionNodeNavPipelineStep {
public:
    JsonPathExpressionNodeNavSliceArray(int startIndex, bool startInverted, int endIndex, bool endInverted);
    
    virtual void inspect(std::ostream &out) const;
    virtual void stepFromNode(JsonPathExpressionNodeEvalContext &context, JsonPathExpressionNodeEvalResult &node);

private:
    int _startIndex;
    bool _startInverted;
    int _endIndex;
    bool _endInverted;
};



class JsonPathExpressionNodeNavIndexArrayByList: public JsonPathExpressionNodeNavPipelineStep {
public:
    JsonPathExpressionNodeNavIndexArrayByList(const std::list<int> &indexList);
            
    virtual void inspect(std::ostream &out) const;
    virtual void stepFromNode(JsonPathExpressionNodeEvalContext &context, JsonPathExpressionNodeEvalResult &node);

private:
    std::list<int> _indexList;
};



class JsonPathExpressionNodeFilter: public JsonPathExpressionNodeNavPipelineStep {
public:
    JsonPathExpressionNodeFilter(std::unique_ptr<JsonPathExpressionNode> &&filter, bool filterChildren);
    
    virtual void inspect(std::ostream &out) const;
    virtual void stepFromNode(JsonPathExpressionNodeEvalContext &context, JsonPathExpressionNodeEvalResult &node);

    virtual void pushDirectDependentNodes(std::list<const JsonPathExpressionNode*> &output) const;
private:
    bool _filterChildren;
    std::unique_ptr<JsonPathExpressionNode> _filter;
};



class JsonPathExpressionNodeIndexByExpression: public JsonPathExpressionNodeNavPipelineStep {
public:
    JsonPathExpressionNodeIndexByExpression(std::unique_ptr<JsonPathExpressionNode> &&filter);
    
    virtual void inspect(std::ostream &out) const;
    virtual void stepFromNode(JsonPathExpressionNodeEvalContext &context, JsonPathExpressionNodeEvalResult &node);

    virtual void pushDirectDependentNodes(std::list<const JsonPathExpressionNode*> &output) const;
private:
    std::unique_ptr<JsonPathExpressionNode> _filter;
};

using expr_operand_value = JsonPathExpressionNodeEvalResult;
using binary_operator_fn = std::function<expr_operand_value(expr_operand_value, expr_operand_value)>;
using binary_operator_and_operand = std::tuple<binary_operator_fn, std::unique_ptr<JsonPathExpressionNode>>;

namespace JsonPathExpressionNodeBinaryOperators {
    expr_operand_value logicalOr(expr_operand_value opLeft,  expr_operand_value opRight);
    expr_operand_value logicalAnd(expr_operand_value opLeft,  expr_operand_value opRight);

    expr_operand_value relEq(expr_operand_value opLeft,  expr_operand_value opRight);
    expr_operand_value relNeq(expr_operand_value opLeft,  expr_operand_value opRight);

    expr_operand_value relGte(json::Node *opLeft, json::Node *opRight);
    expr_operand_value relLte(json::Node *opLeft, json::Node *opRight);
    expr_operand_value relGt(json::Node *opLeft, json::Node *opRight);
    expr_operand_value relLt(json::Node *opLeft, json::Node *opRight);
    expr_operand_value relIn(json::Node *opLeft, json::ArrayNode *right);
    expr_operand_value relNotIn(json::Node *opLeft, json::ArrayNode *right);
    expr_operand_value relSubsetOf(json::ArrayNode *left, json::ArrayNode *right);
    expr_operand_value relAnyOf(json::ArrayNode *left, json::ArrayNode *right);
    expr_operand_value relNoneOf(json::ArrayNode *left, json::ArrayNode *right);

    expr_operand_value add(expr_operand_value opLeft,  expr_operand_value opRight);
    expr_operand_value div(json::NumberNode *opLeft,  json::NumberNode *opRight);
    expr_operand_value mul(json::NumberNode *opLeft,  json::NumberNode *opRight);
    expr_operand_value mod(json::NumberNode *opLeft,  json::NumberNode *opRight);
    expr_operand_value subtract(json::NumberNode *opLeft,  json::NumberNode *opRight);
}


class JsonPathExpressionNodeBinaryOp: public JsonPathExpressionNode {
public:
    JsonPathExpressionNodeBinaryOp(std::unique_ptr<JsonPathExpressionNode> &&leftMost, std::list<binary_operator_and_operand> &operands);
    
    virtual void inspect(std::ostream &out) const;
    virtual JsonPathExpressionNodeEvalResult evaluate(JsonPathExpressionNodeEvalContext &context);
    virtual void pushDirectDependentNodes(std::list<const JsonPathExpressionNode*> &output) const;
    
private:
    std::unique_ptr<JsonPathExpressionNode> _leftMost;
    std::list<binary_operator_and_operand> _operandList;
};


class JsonPathExpressionNodeNegateOp: public JsonPathExpressionNode {
public:
    JsonPathExpressionNodeNegateOp(std::unique_ptr<JsonPathExpressionNode> &&operand);
    
    virtual void inspect(std::ostream &out) const;
    virtual JsonPathExpressionNodeEvalResult evaluate(JsonPathExpressionNodeEvalContext &context);
    
    virtual void pushDirectDependentNodes(std::list<const JsonPathExpressionNode*> &output) const;
private:
    std::unique_ptr<JsonPathExpressionNode> _operand;
};


class JsonPathExpressionNodeJsonLiteral: public JsonPathExpressionNode {
public:
    JsonPathExpressionNodeJsonLiteral(std::unique_ptr<json::Node> literal);
    
    virtual void inspect(std::ostream &out) const;
    virtual JsonPathExpressionNodeEvalResult evaluate(JsonPathExpressionNodeEvalContext &context);
    virtual void pushDirectDependentNodes(std::list<const JsonPathExpressionNode*> &output) const {}
    
private:
    std::shared_ptr<json::Node> _literal;
};


class JsonPathExpressionFunction {
public:
    virtual JsonPathExpressionNodeEvalResult invoke(const std::list<JsonPathExpressionNodeEvalResult> &args) = 0;
    virtual int arity() = 0;

    static std::map<std::string, JsonPathExpressionFunction*> functions;
};

class JsonPathExpressionNodeFunctionInvoke: public JsonPathExpressionNode {
public:
    JsonPathExpressionNodeFunctionInvoke(JsonPathExpressionFunction *fn, std::list<std::unique_ptr<JsonPathExpressionNode>> &&args);
    
    virtual void inspect(std::ostream &out) const;
    virtual JsonPathExpressionNodeEvalResult evaluate(JsonPathExpressionNodeEvalContext &context);
    virtual void pushDirectDependentNodes(std::list<const JsonPathExpressionNode*> &output) const;
    
private:
    JsonPathExpressionFunction *_fn;
    std::list<std::unique_ptr<JsonPathExpressionNode>> _args;
};

class JsonPathEvalError : public std::runtime_error {
public:
    JsonPathEvalError(const char *what): std::runtime_error(what) {
        
    }
        
private:
    const char* _what;
} ;



#endif /* JsonPathExpressionNode_hpp */
