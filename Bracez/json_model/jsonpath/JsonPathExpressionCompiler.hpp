//
//  JsonPathExpressionCompiler.hpp
//  Bracez
//
//  Created by Eldan Ben Haim on 30/01/2021.
//

#ifndef JsonPathExpressionCompiler_hpp
#define JsonPathExpressionCompiler_hpp

#include "JsonPathExpressionNode.hpp"
#include "Parser-Combinators/parser_combinators.hpp"

enum CompiledExpressionType {
    jsonPath,
    jsonPathExpression
};

struct JsonPathExpressionOptions {
    bool fuzzy;
};

class JsonPathExpression {
public:
    JsonPathExpressionNodeEvalResult execute(json::Node *root,
                                             JsonPathExpressionOptions *options = NULL,
                                             json::Node *initialContext = NULL);
    
public:
    static JsonPathExpression compile(const std::wstring &inputExpression,
                                      CompiledExpressionType expressionType = jsonPath);
    static JsonPathExpression compile(const std::string &inputExpression,
                                      CompiledExpressionType expressionType = jsonPath);
    static bool isValidIdentifier(const std::wstring &input);
    
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

#endif /* JsonPathExpressionCompiler_hpp */
