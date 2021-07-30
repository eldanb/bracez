//
//  JsonPathExpressionCompiler.hpp
//  Bracez
//
//  Created by Eldan Ben Haim on 30/01/2021.
//

#ifndef JsonPathExpressionCompiler_hpp
#define JsonPathExpressionCompiler_hpp

#include "JsonPathExpressionNode.hpp"

struct JsonPathExpressionOptions {
    bool fuzzy;
};

class JsonPathExpression {
public:
    JsonPathResultNodeList execute(json::Node *root, JsonPathExpressionOptions *options = NULL);
    
public:
    static JsonPathExpression compile(const std::wstring &inputExpression);
    static JsonPathExpression compile(const std::string &inputExpression);
    
    JsonPathExpression(JsonPathExpression &&expr) : _rootNode(std::move(expr._rootNode)) {}
    JsonPathExpression() {}

    JsonPathExpression &operator=(JsonPathExpression &&expr) {
        _rootNode = std::move(expr._rootNode);
        return *this;
    }

private:
    JsonPathExpression(std::unique_ptr<JsonPathExpressionNode> &&rootNode) : _rootNode(std::move(rootNode)) {}
    
private:
    std::unique_ptr<JsonPathExpressionNode> _rootNode;
};

void testjsonpathexpressionparser();

#endif /* JsonPathExpressionCompiler_hpp */
