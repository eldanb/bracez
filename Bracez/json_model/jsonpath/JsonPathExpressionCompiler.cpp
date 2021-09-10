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
constexpr is_oneof is_name_char = {"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_0123456789"};

auto const name_token = define("name", tokenise(some(accept(is_name_char))));
auto const wildcard_token = tokenise(accept(is_char('*')));

auto const qualify_token = tokenise(accept(is_char('.')));
auto const navigate_to_parent_token = tokenise(accept_str("^"));
auto const navigate_to_ancestor_token = tokenise(accept_str("^^"));

auto const root_token = tokenise(accept(is_char('$')));
auto const context_token = tokenise(accept(is_char('@')));

auto const comma_token = tokenise(accept(is_char(',')));
auto const invert_slice_index_token = tokenise(accept(is_char('-')));
auto const slice_index_sep_token = tokenise(accept(is_char(':')));
auto const logical_negate_token = tokenise(accept(is_char('!')));

auto const open_filter_paren_token = tokenise(accept_str("?("));
auto const close_filter_paren_token = tokenise(accept(is_char(')')));

auto const open_paren_token = tokenise(accept_str("("));
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
        wstringstream currentAsStream(std::wstring(i, r.last)); // TODO consider making a view here
        json::Reader::Read(*result, currentAsStream, &pl, true);
        i += currentAsStream.tellg();

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



// TODO better string parsing; use JSON parser here?
auto const number_literal = tokenise(some(accept(is_digit)));
auto string_literal = all([](std::string* result, json::Node* node) {
    json::StringNode *snode = dynamic_cast<json::StringNode*>(node);
    if(!snode) {
        return false;
    } else {
        *result = wide_utf8_converter.to_bytes(snode->GetValue());
        return true;
    }
}, parse_json());



/*
 * Grammar: Pipelines
 */

extern expression_parser_handle expression;

struct construct_context_specifier {
    void operator()(JsonPathExpressionNode **result, std::string tok) const {
        if(tok == "$") {
            *result = new JsonPathExpressionNodeNavPipeline(false);
        } else {
            *result = new JsonPathExpressionNodeNavPipeline(true);
        }
    }
} constexpr construct_context_specifier;
auto context_specifier = all(construct_context_specifier, root_token || context_token);


struct construct_navigate_recurse {
    void operator()(JsonPathExpressionNodeNavPipelineStep **result, std::string qualify, std::string name) const {
        if(name == "*") {
            *result = new JsonPathExpressionNodeNavRecurse();
        } else {
            *result = new JsonPathExpressionNodeNavRecurse(wide_utf8_converter.from_bytes(name));
        }
    }
} constexpr construct_navigate_recurse;
auto navigate_recurse = all(construct_navigate_recurse, qualify_token, (name_token || wildcard_token));


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
        (attempt(discard(start_subscript_token) &&
         discard(open_filter_paren_token)) &&
         strict("filter_expression",
            all([](JsonPathExpressionNodeNavPipelineStep **result, JsonPathExpressionNode *expression) {
                    *result = new JsonPathExpressionNodeFilterChildren(
                                std::unique_ptr<JsonPathExpressionNode>(expression));
                },
                reference("expression", &expression) &&
                discard(close_filter_paren_token) &&
                discard(end_subscript_token)))));



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

constexpr auto operator_parser(const char *operatorSyntax,
                               expr_operand_value(*operatorFn)(expr_operand_value, expr_operand_value)) {
    return all([operatorFn](binary_operator_fn *result, std::string opname) {
        *result = operatorFn;
    }, tokenise(accept_str(operatorSyntax)));
}

auto json_literal = all([](JsonPathExpressionNode** result, json::Node* node) {
                            *result = new JsonPathExpressionNodeJsonLiteral(std::unique_ptr<json::Node>(node));
                        }, parse_json());

auto expression_terminal = attempt(navigation_pipe) ||
                            attempt(discard(open_paren_token) && reference("expression", &expression) && discard(close_paren_token)) ||
                            json_literal;

auto muldiv_expression = binary_operator(expression_terminal,
                                        attempt(operator_parser("*", JsonPathExpressionNodeBinaryOperators::mul)) ||
                                        attempt(operator_parser("/", JsonPathExpressionNodeBinaryOperators::div)) ||
                                        operator_parser("%", JsonPathExpressionNodeBinaryOperators::mod)
                                        );

auto addsub_expression = binary_operator(muldiv_expression,
                                        attempt(operator_parser("+", JsonPathExpressionNodeBinaryOperators::add)) ||
                                        operator_parser("-", JsonPathExpressionNodeBinaryOperators::subtract)
                                        );

auto negate_expression = attempt(addsub_expression) ||
                        all([](JsonPathExpressionNode **result, JsonPathExpressionNode *operand) {
                            *result = new JsonPathExpressionNodeNegateOp(std::unique_ptr<JsonPathExpressionNode>(operand));
                            }, discard(logical_negate_token) && addsub_expression);

auto rel_expression = binary_operator(negate_expression,
                                        attempt(operator_parser(">=", JsonPathExpressionNodeBinaryOperators::relGte)) ||
                                        attempt(operator_parser("<=", JsonPathExpressionNodeBinaryOperators::relLte)) ||
                                        attempt(operator_parser(">", JsonPathExpressionNodeBinaryOperators::relGt)) ||
                                        operator_parser("<", JsonPathExpressionNodeBinaryOperators::relLt)
                                        );

auto eqneq_expression = binary_operator(rel_expression,
                                        attempt(operator_parser("==", JsonPathExpressionNodeBinaryOperators::relEq)) ||
                                        operator_parser("!=", JsonPathExpressionNodeBinaryOperators::relNeq)
                                        );
auto and_expression = binary_operator(eqneq_expression, operator_parser("&&", JsonPathExpressionNodeBinaryOperators::logicalAnd));
auto or_expression = binary_operator(and_expression, operator_parser("||", JsonPathExpressionNodeBinaryOperators::logicalOr));

expression_parser_handle expression = or_expression;

auto json_path_compiler = first_token && strict("navigation_pipe", navigation_pipe) && strict("end_of_expression", discard(accept(is_eof)));


JsonPathResultNodeList JsonPathExpression::execute(json::Node *root, JsonPathExpressionOptions *options) {
    JsonPathExpressionNodeEvalContext evalContext;
    evalContext.rootNode = root;
    evalContext.contextNode = root;
    evalContext.options = options;
    JsonPathExpressionNodeEvalResult evalResult = _rootNode->evaluate(evalContext);
    return evalResult.nodeList;
}

JsonPathExpression JsonPathExpression::compile(const std::wstring &inputExpression) {
    string_stream_range strrange(inputExpression);
    auto i = strrange.first;
    JsonPathExpressionNode *out = NULL;

    json_path_compiler(i, strrange, &out);
    
    return JsonPathExpression(std::unique_ptr<JsonPathExpressionNode>(out));
}

JsonPathExpression JsonPathExpression::compile(const std::string &inputExpression) {
    return compile(wide_utf8_converter.from_bytes(inputExpression));
}


void assertResult(json::Node* doc, const std::wstring &expression, const JsonPathResultNodeList &expectedResult) {
    JsonPathResultNodeList result = JsonPathExpression::compile(L"$.store.*").execute(doc);
    if(result != expectedResult) {
        throw "Invalid result";
    }
}


void testjsonpathexpressionparser() {
    wstringstream docText(L"{"
                           "  \"store\": {"
                           "    \"book\": ["
                           "       {"
                           "         \"category\": \"reference\","
                           "         \"author\": \"Nigel Rees\","
                           "         \"title\": \"Sayings of the Century\","
                           "         \"price\": 8.95"
                           "       },"
                           "       {"
                           "         \"category\": \"fiction\","
                           "         \"author\": \"Herman Melville\","
                           "         \"title\": \"Moby Dick\","
                           "         \"isbn\": \"0-553-21311-3\","
                           "         \"price\": 8.99"
                           "       },"
                           "       {"
                           "         \"category\": \"fiction\","
                           "         \"author\": \"J.R.R. Tolkien\","
                           "         \"title\": \"The Lord of the Rings\","
                           "         \"isbn\": \"0-395-19395-8\","
                           "         \"price\": 22.99"
                           "       }"
                           "     ],"
                           "     \"bicycle\": {"
                           "       \"color\": \"red\","
                           "       \"price\": 19.95"
                           "     }"
                           "  },"
                           "  \"expensive\": 10"
                           "}");
    json::Node *doc = nullptr;
    json::Reader::Read(doc, docText);
    
    JsonPathResultNodeList result1 = JsonPathExpression::compile(L"$.store.*").execute(doc);
    JsonPathResultNodeList result2 = JsonPathExpression::compile(L"$.store.bicycle.color").execute(doc);
    JsonPathResultNodeList result3 = JsonPathExpression::compile(L"$.store..price").execute(doc);
    JsonPathResultNodeList result4 = JsonPathExpression::compile(L"$..price").execute(doc);
    JsonPathResultNodeList result5 = JsonPathExpression::compile(L"$.store.book[*]").execute(doc);
    JsonPathResultNodeList result6 = JsonPathExpression::compile(L"$..book[*]").execute(doc);
    JsonPathResultNodeList result7 = JsonPathExpression::compile(L"$..book[*].title").execute(doc);
    JsonPathResultNodeList result8 = JsonPathExpression::compile(L"$..book[0]").execute(doc);
    JsonPathResultNodeList result9 = JsonPathExpression::compile(L"$..book[0,1].title").execute(doc);
    JsonPathResultNodeList result10 = JsonPathExpression::compile(L"$..book[:2].title").execute(doc);
    JsonPathResultNodeList result11 = JsonPathExpression::compile(L"$..book[-1:].title").execute(doc);
    JsonPathResultNodeList result12 = JsonPathExpression::compile(L"$..book[?(@.author==\"J.R.R. Tolkien\")].title").execute(doc);
    JsonPathResultNodeList result13 = JsonPathExpression::compile(L"$..book[?(@.isbn)]").execute(doc);
    JsonPathResultNodeList result14 = JsonPathExpression::compile(L"$..book[?(!@.isbn)]").execute(doc);
    JsonPathResultNodeList result15 =
    JsonPathExpression::compile(L"$..book[?(@.category == \"fiction\" || @.category == \"reference\")]").execute(doc);
    JsonPathResultNodeList result16 = JsonPathExpression::compile(L"$..book[?(@.price < 10)]").execute(doc);
    JsonPathResultNodeList result17 = JsonPathExpression::compile(L"$..book[?(@.price > $.expensive)]").execute(doc);
    
    // TODO
    //JsonPathResultNodeList result18 = JsonPathExpression::compile(L"$..book[?(@.author =~ /.*Tolkien/i)]").execute(doc);
    
}
