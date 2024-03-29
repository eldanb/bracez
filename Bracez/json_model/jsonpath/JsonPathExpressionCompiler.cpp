//
//  JsonPathExpressionCompiler.cpp
//  Bracez
//
//  Created by Eldan Ben Haim on 30/01/2021.
//

#include "JsonPathExpressionCompiler.hpp"

#include "JsonPathExpressionNode.hpp"

#include "reader.h"

#include <iostream>
#include <tuple>

/*
 * Utilities
 */
template<typename... Tps>
struct to_tuple {
    void operator()(std::tuple<Tps...> **result, Tps... a) const {
        *result = new std::tuple(a...);
    }
};

template<typename... PS>
constexpr fmap_sequence<to_tuple<typename PS::result_type...>, PS...> const mktuple(PS const&... ps) {
    return all(to_tuple<typename PS::result_type...>(), ps...);
}

using convert_type = std::codecvt_utf8<wchar_t>;
static std::wstring_convert<convert_type, wchar_t> wide_utf8_converter;

/*
 * Stream
 */
struct string_stream_range {
    string_stream_range(string_stream_range const&) = delete;

    string_stream_range(const std::wstring &underlying)
        : first(underlying.begin()), last(underlying.end())
    {
    }
    
    using iterator = std::wstring::const_iterator;

    iterator const first;
    iterator const last;
};

typedef parser_handle<string_stream_range::iterator, string_stream_range, JsonPathExpressionNode *> expression_parser_handle;

/*
 * Tokens
 */
struct is_oneof {
    const char *allowed;
    
    using is_predicate_type = true_type;
    static constexpr int rank = 0;
    
    bool operator() (int const c) const {
        return strchr(allowed, c) != nullptr;
    }
    
    string name() const {
        return "isOneof";
    }
};

constexpr is_oneof is_name_start = {"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_"};
constexpr is_oneof is_name_char = {"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_0123456789"};

auto const name_token = define("name", tokenise(accept(is_name_start) && many(accept(is_name_char))));
auto const wildcard_token = tokenise(accept(is_char('*')));

auto const qualify_token = tokenise(accept(is_char('.')));
auto const navigate_to_parent_token = tokenise(accept_str("^"));
auto const navigate_to_ancestor_token = tokenise(accept_str("^^"));

auto const root_token = tokenise(accept(is_char('$')));
auto const context_token = tokenise(accept(is_char('@')));
auto const cursor_token = tokenise(accept(is_char('^')));
auto const context_name_token = tokenise(accept_str("@@"));

auto const comma_token = tokenise(accept(is_char(',')));
auto const invert_slice_index_token = tokenise(accept(is_char('-')));
auto const slice_index_sep_token = tokenise(accept(is_char(':')));
auto const logical_negate_token = tokenise(accept(is_char('!')));

auto const open_children_filter_paren_token = tokenise(accept_str("?("));
auto const open_filter_paren_token = tokenise(accept_str("??("));
auto const close_filter_paren_token = tokenise(accept(is_char(')')));

auto const open_paren_token = tokenise(accept(is_char('(')));
auto const close_paren_token = tokenise(accept(is_char(')')));

auto const start_subscript_token = tokenise(accept(is_char('[')));
auto const end_subscript_token = tokenise(accept(is_char(']')));

/*
 * Grammar: JSON
 */
class parse_json {
    class IsErrorParseListener : public json::ParseListener {
    public:
        bool error = false;
        
        virtual void EndOfLine(TextCoordinate aWhere) {}
        virtual void Error(TextCoordinate aWhere, int aCode, const string &aText) { error = true; }
    };
    
public:
    using is_parser_type = true_type;
    using is_handle_type = false_type;
    using has_side_effects = false_type;
    using result_type = json::Node*;
    int const rank = 0;

    constexpr explicit parse_json() {}

    template <typename Iterator, typename Range, typename Inherit = default_inherited>
    bool operator() (
        Iterator &i,
        Range const &r,
        json::Node **result = nullptr,
        Inherit* st = nullptr
    ) const {
        if (i == r.last) {
            return false;
        }
        
        IsErrorParseListener pl;
        wstring currentRegion = std::wstring(i, r.last); // TODO consider making a view here
        unsigned long consumed = json::Reader::Read(*result, currentRegion, &pl, true);
        i += consumed;

        if(pl.error) {
            i++;
            return false;
        } else {
            return true;
        }
    }

    string ebnf(unique_defs* defs = nullptr) const {
        return "<json>";
    }
};



auto const number_literal = tokenise(some(accept(is_digit)));

// TODO better string parsing; use JSON parser here?
auto string_literal = all([](std::string* result, json::Node* node) {
    json::StringNode *snode = dynamic_cast<json::StringNode*>(node);
    if(!snode) {
        return false;
    } else {
        *result = wide_utf8_converter.to_bytes(snode->getValue());
        return true;
    }
}, parse_json());



/*
 * Grammar: Pipelines
 */

extern expression_parser_handle expression;

struct construct_context_specifier {
    void operator()(JsonPathExpressionNode **result, std::string tok) const {
        switch(tok[0]) {
            case '$':
                *result = new JsonPathExpressionNodeNavPipeline(JsonPathExpressionNodeNavPipeline::pipelineStartAtRoot);
                break;

            case '@':
                *result = new JsonPathExpressionNodeNavPipeline(JsonPathExpressionNodeNavPipeline::pipelineStartAtContext);
                break;

            case '^':
                *result = new JsonPathExpressionNodeNavPipeline(JsonPathExpressionNodeNavPipeline::pipelineStartAtCursor);
                break;
                
            default:
                throw JsonPathEvalError("Invalid pipeline context specifier");

        }
    }
} constexpr construct_context_specifier;
auto context_specifier = all(construct_context_specifier, root_token || context_token || cursor_token);


struct construct_navigate_recurse {
    void operator()(JsonPathExpressionNodeNavPipelineStep **result, std::string qualify, std::string name) const {
        if(name == "*") {
            *result = new JsonPathExpressionNodeNavRecurse();
        } else {
            *result = new JsonPathExpressionNodeNavRecurse(wide_utf8_converter.from_bytes(name));
        }
    }
} constexpr construct_navigate_recurse;
auto navigate_recurse = all(construct_navigate_recurse,
                            qualify_token,
                            (name_token || wildcard_token));


struct construct_navigate_to_ancestor {
    void operator()(JsonPathExpressionNodeNavPipelineStep **result, std::string qualify) const {
        *result = new JsonPathExpressionNodeNavParent(true);
    }
} constexpr construct_navigate_to_ancestor;
auto navigate_to_ancestor = all(construct_navigate_to_ancestor, navigate_to_ancestor_token);


struct construct_navigate_to_parent {
    void operator()(JsonPathExpressionNodeNavPipelineStep **result, std::string qualify) const {
        *result = new JsonPathExpressionNodeNavParent(false);
    }
} constexpr construct_navigate_to_parent;
auto navigate_to_parent = all(construct_navigate_to_parent, navigate_to_parent_token);

static void construct_node_context_member_name(JsonPathExpressionNode **result, std::string dummy) {
    *result = new JsonPathExpressionNodeContextMemberName();
}
auto context_member_name = define("member_name",
                                all(&construct_node_context_member_name, context_name_token));

struct construct_navigate_non_recurse {
    void operator()(JsonPathExpressionNodeNavPipelineStep **result, std::string name) const {
        if(name == "*") {
            *result = new JsonPathExpressionNodeNavAllChildren();
        } else {
            *result = new JsonPathExpressionNodeNavResolveName(wide_utf8_converter.from_bytes(name));
        }
    }
} constexpr construct_navigate_non_recurse;
auto navigate_non_recurse =
    all(construct_navigate_non_recurse, (name_token || wildcard_token));

auto dot_qualify =
    define("dot_qualify",
           discard(qualify_token) &&
                strict("dot_qualifiation_step",
                    attempt(navigate_recurse) ||
                    attempt(navigate_to_ancestor) ||
                    attempt(navigate_to_parent) ||
                    navigate_non_recurse));




struct construct_index_array {
    void operator()(JsonPathExpressionNodeNavPipelineStep **result, std::string index) const {
        *result = new JsonPathExpressionNodeNavIndexArray(atoi(index.c_str()));
    }
} constexpr construct_index_array;
auto index_array =
    define("array_indexing", all(construct_index_array,
        number_literal));


void construct_get_all_array_items(JsonPathExpressionNodeNavPipelineStep **result, std::string glob) {
    *result = new JsonPathExpressionNodeNavSliceArray(0, false, 0, true);
}

auto get_all_array_items =
    all(&construct_get_all_array_items,
        wildcard_token);


void construct_get_array_item_list(JsonPathExpressionNodeNavPipelineStep **result, std::list<int> numLiteral) {
    *result = new JsonPathExpressionNodeNavIndexArrayByList(numLiteral);
}

void construct_index_list(std::list<int> *result, std::string index) {
    result->push_back(atoi(index.c_str()));
}

auto get_array_item_list =
    define("array_index_list",
           all(&construct_get_array_item_list,
               (sep_by(all(&construct_index_list, number_literal), comma_token))));

void construct_get_array_slice(JsonPathExpressionNodeNavPipelineStep **result,
                    std::tuple<std::string, std::string> *sliceStart,
                    std::string sep,
                    std::tuple<std::string, std::string> *sliceEnd)  {
        
    tuple<std::string, std::string> defaultStart(std::string(""), std::string("0"));
    tuple<std::string, std::string> defaultEnd(std::string("-"), std::string("0"));
    
    if(!sliceStart) {
        sliceStart = &defaultStart;
    }
    if(!sliceEnd) {
        sliceEnd = &defaultEnd;
    }

    *result = new JsonPathExpressionNodeNavSliceArray(
      atoi(get<1>(*sliceStart).c_str()),
      !!get<0>(*sliceStart).size(),
      atoi(get<1>(*sliceEnd).c_str()),
      !!get<0>(*sliceEnd).size());
}

auto get_array_slice =
    define("array_slice",
        all(&construct_get_array_slice,
            option(mktuple(
                           option(invert_slice_index_token),
                           number_literal)),
            slice_index_sep_token,
            option(mktuple(
                option(invert_slice_index_token),
                number_literal))));


auto subscript_by_expression =
    define("subscript_expression",
           all([](JsonPathExpressionNodeNavPipelineStep **result, JsonPathExpressionNode *expression) {
                *result = new JsonPathExpressionNodeIndexByExpression(
                        std::unique_ptr<JsonPathExpressionNode>(expression));
            },
            (discard(open_paren_token) &&
             strict("subscript_expression", reference("expression", &expression) &&
                    discard(close_paren_token)))));

auto subscript_by_string =
    all(construct_navigate_non_recurse, string_literal);

auto subscript_qualify =
    define("subscript",
           discard(start_subscript_token) &&
                strict("subscript_step",
                    (attempt(get_all_array_items) ||
                     attempt(get_array_slice) ||
                     attempt(get_array_item_list) ||
                     attempt(index_array) ||
                     attempt(subscript_by_string) ||
                     attempt(subscript_by_expression)) && discard(end_subscript_token)));

auto filter_by_expression =
    define("filter_expression",
         attempt(discard(start_subscript_token) &&
            all([](JsonPathExpressionNodeNavPipelineStep **result, std::string type, JsonPathExpressionNode *expression) {
                    *result = new JsonPathExpressionNodeFilter(
                                std::unique_ptr<JsonPathExpressionNode>(expression), type == "?(");
                },
                (open_filter_paren_token || open_children_filter_paren_token),
                strict("filter_expression", reference("expression", &expression)) &&
                discard(close_filter_paren_token) &&
                discard(end_subscript_token))));



const auto navigation_step =
    dot_qualify ||
    filter_by_expression ||
    subscript_qualify;
    
struct construct_navigation_pipe {
    void operator()(JsonPathExpressionNode **result, JsonPathExpressionNodeNavPipelineStep *pipeStep) const {
        dynamic_cast<JsonPathExpressionNodeNavPipeline*>(*result)->addStep(std::unique_ptr<JsonPathExpressionNodeNavPipelineStep>(pipeStep));
    }
} constexpr construct_navigation_pipe;

auto navigation_pipe = context_specifier && many(all(construct_navigation_pipe, navigation_step));

/*
 * Grammar: Expressions
 */

template<typename P1, typename P2>
constexpr auto binary_operator(P1 const &opand, P2 const &opor) {
    return all([](JsonPathExpressionNode **result,
                  JsonPathExpressionNode *firstOperand,
                  std::list<binary_operator_and_operand> &operands) {
                    if(operands.size() == 0) {
                        *result = firstOperand;
                    } else {
                        *result = new JsonPathExpressionNodeBinaryOp(std::unique_ptr<JsonPathExpressionNode>(firstOperand), operands);
                    }
               },
               opand,
               many(all([](std::list<binary_operator_and_operand> *result,
                            binary_operator_fn &opor,
                            JsonPathExpressionNode* opand) {
                         result->push_back(binary_operator_and_operand(opor, opand));
                      },
                      opor, strict("operand", opand))));
}

auto operator_parser(const char *operatorSyntax,
                               std::function<expr_operand_value (expr_operand_value, expr_operand_value)> operatorFn) {
    return all([operatorFn](std::function<expr_operand_value (expr_operand_value, expr_operand_value)> *result, std::string opname) {
        *result = operatorFn;
    }, tokenise(accept_str(operatorSyntax)));
}

template<class LT, class RT>
auto scalar_operator_parser(const char *operatorSyntax,
                                      expr_operand_value(*operatorFn)(LT *, RT *)) {
    return operator_parser(operatorSyntax, [operatorFn](expr_operand_value l, expr_operand_value r) {
        if(l.nodeList.size() == 1 && r.nodeList.size() == 1) {
            LT *lt = dynamic_cast<LT*>(l.nodeList.front());
            RT *rt = dynamic_cast<RT*>(r.nodeList.front());
            
            if(lt && rt) {
                return operatorFn(lt, rt);
            }
        }
        
        return expr_operand_value::nullResult();
    });
}

auto exprlist =        sep_by(all([](std::list<std::unique_ptr<JsonPathExpressionNode>> *result,
                                     JsonPathExpressionNode* expr) {
                                    result->push_back(std::unique_ptr<JsonPathExpressionNode>(expr));
                                  },
                                  reference("expression", &expression)),
                              tokenise(accept(is_char(','))));

auto function_call = define("function_call", all([](JsonPathExpressionNode** result,
                                                    std::string fnName,
                                                    std::list<std::unique_ptr<JsonPathExpressionNode>> &args) {
                                                        auto fnIter = JsonPathExpressionFunction::functions.find(fnName);
                                                        if(fnIter == JsonPathExpressionFunction::functions.end()) {
                                                            throw JsonPathEvalError("Unknown function.");
                                                        }
                                                        
                                                        if(fnIter->second->arity() != args.size()) {
                                                            throw JsonPathEvalError("Invalid argument count for function.");
                                                        }
    
                                                        *result = new JsonPathExpressionNodeFunctionInvoke(fnIter->second, std::move(args));
                                                 },
                                                 name_token,
                                                 discard(open_paren_token) &&
                                                      strict("function_call",
                                                             exprlist && discard(close_paren_token))));


                                     
auto json_literal = all([](JsonPathExpressionNode** result, json::Node* node) {
                            *result = new JsonPathExpressionNodeJsonLiteral(std::unique_ptr<json::Node>(node));
                        }, parse_json());

auto expression_terminal =  attempt(context_member_name) ||
                            attempt(navigation_pipe) ||
                            attempt(function_call) ||
                            attempt(discard(open_paren_token) && reference("expression", &expression) && discard(close_paren_token)) ||
                            json_literal;
                            

auto muldiv_expression = binary_operator(expression_terminal,
                                        scalar_operator_parser("*", JsonPathExpressionNodeBinaryOperators::mul) ||
                                        scalar_operator_parser("/", JsonPathExpressionNodeBinaryOperators::div) ||
                                        scalar_operator_parser("%", JsonPathExpressionNodeBinaryOperators::mod)
                                        );

auto addsub_expression = binary_operator(muldiv_expression,
                                         operator_parser("+", JsonPathExpressionNodeBinaryOperators::add) ||
                                         scalar_operator_parser("-", JsonPathExpressionNodeBinaryOperators::subtract)
                                        );

auto negate_expression = attempt(addsub_expression) ||
                        all([](JsonPathExpressionNode **result, JsonPathExpressionNode *operand) {
                            *result = new JsonPathExpressionNodeNegateOp(std::unique_ptr<JsonPathExpressionNode>(operand));
                            }, discard(logical_negate_token) && addsub_expression);

auto rel_expression = binary_operator(negate_expression,
                                        scalar_operator_parser(">=", JsonPathExpressionNodeBinaryOperators::relGte) ||
                                        scalar_operator_parser("<=", JsonPathExpressionNodeBinaryOperators::relLte) ||
                                        scalar_operator_parser(">", JsonPathExpressionNodeBinaryOperators::relGt) ||
                                        scalar_operator_parser("<", JsonPathExpressionNodeBinaryOperators::relLt) ||
                                        scalar_operator_parser("in", JsonPathExpressionNodeBinaryOperators::relIn) ||
                                        scalar_operator_parser("nin", JsonPathExpressionNodeBinaryOperators::relNotIn) ||
                                        scalar_operator_parser("subsetof", JsonPathExpressionNodeBinaryOperators::relSubsetOf) ||
                                        scalar_operator_parser("anyof", JsonPathExpressionNodeBinaryOperators::relAnyOf) ||
                                        scalar_operator_parser("noneof", JsonPathExpressionNodeBinaryOperators::relNoneOf));

auto eqneq_expression = binary_operator(rel_expression,
                                        attempt(operator_parser("==", JsonPathExpressionNodeBinaryOperators::relEq)) ||
                                        operator_parser("!=", JsonPathExpressionNodeBinaryOperators::relNeq)
                                        );
auto and_expression = binary_operator(eqneq_expression, operator_parser("&&", JsonPathExpressionNodeBinaryOperators::logicalAnd));
auto or_expression = binary_operator(and_expression, operator_parser("||", JsonPathExpressionNodeBinaryOperators::logicalOr));

expression_parser_handle expression = or_expression;

auto json_path_compiler =
        first_token &&
        strict("navigation_pipe", navigation_pipe) &&
        strict("end_of_expression", discard(accept(is_eof)));

auto json_path_expr_compiler =
        first_token &&
        strict("expression", expression) &&
        strict("end_of_expression", discard(accept(is_eof)));


JsonPathExpressionNodeEvalResult JsonPathExpression::execute(json::Node *root,
                                                             JsonPathExpressionOptions *options,
                                                             json::Node *initialContext,
                                                             json::Node *cursorNode) {
    JsonPathExpressionNodeEvalContext evalContext;
    evalContext.rootNode = root;
    evalContext.contextNode = initialContext ? initialContext : root;
    evalContext.cursorNode = cursorNode;
    evalContext.options = options;
    return _rootNode->evaluate(evalContext);
}

JsonPathExpression JsonPathExpression::compile(const std::wstring &inputExpression,
                                               CompiledExpressionType expressionType) {
    string_stream_range strrange(inputExpression);
    auto i = strrange.first;
    JsonPathExpressionNode *out = NULL;

    switch(expressionType)
    {
        case CompiledExpressionType::jsonPath:
            json_path_compiler(i, strrange, &out);
            break;
            
        case CompiledExpressionType::jsonPathExpression:
            json_path_expr_compiler(i, strrange, &out);
            break;
    }
    
    return JsonPathExpression(std::unique_ptr<JsonPathExpressionNode>(out));
}

JsonPathExpression JsonPathExpression::compile(const std::string &inputExpression,
                                               CompiledExpressionType expressionType) {
    return compile(wide_utf8_converter.from_bytes(inputExpression), expressionType);
}

bool JsonPathExpression::isCursorDependent() const {
    bool cursorDependent = false;
    enumerateNodes([&cursorDependent](const JsonPathExpressionNode *node) {
        const JsonPathExpressionNodeNavPipeline *pipeline = dynamic_cast<const JsonPathExpressionNodeNavPipeline *>(node);
        if(pipeline && pipeline->getPipelineStart() == JsonPathExpressionNodeNavPipeline::pipelineStartAtCursor) {
            cursorDependent = true;
        }
    });
    
    return cursorDependent;
}

void JsonPathExpression::enumerateNodes(const std::function<void (const JsonPathExpressionNode*)> &enumerator) const {
    std::list<const JsonPathExpressionNode *> nodes;
    nodes.push_back(_rootNode.get());
    while(nodes.size()) {
        enumerator(nodes.front());
        nodes.front()->pushDirectDependentNodes(nodes);
        nodes.pop_front();
    }
}

bool JsonPathExpression::isValidIdentifier(const std::wstring &input) {
    string_stream_range strrange(input);
    auto i = strrange.first;
    return name_token(i, strrange, nullptr, (void*)nullptr) && i == strrange.last;
}

