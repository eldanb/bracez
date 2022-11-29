//
//  json_path_tests.cpp
//  Bracez
//
//  Created by Eldan Ben-Haim on 07/12/2021.
//

#include "json_file.h"
#include "JsonPathExpressionNode.hpp"
#include "JsonPathExpressionCompiler.hpp"

#include <string>

#include "catch2/catch.hpp"

using namespace json;

class JsonPathTestFixture {
public:
    JsonPathTestFixture(const std::wstring &docText) {
        json::Reader::Read(doc, docText);
    }
    
protected:
    json::Node *doc;
};

class BooksJsonPathTestFixture : public JsonPathTestFixture {
public:
    BooksJsonPathTestFixture() : JsonPathTestFixture(L"{"
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
                                                     "}") {}
};

static std::wstring jsonPathResult(json::Node *node, const std::wstring &jsonPath) {
    JsonPathResultNodeList result = JsonPathExpression::compile(jsonPath).execute(node).nodeList;
    if(!result.empty()) {
        std::wstring ret = L"[ ";
        std::for_each(result.begin(), result.end(),
                      [&ret](json::Node* node) {
            std::wstring nodeRet;
            node->calculateJsonTextRepresentation(nodeRet);
            ret += nodeRet;
            ret += L", ";
        });
        
        ret.erase(ret.end()-2);
        ret += L"]";
        
        return ret;
    } else {
        return L"[ ]";
    }
}

TEST_CASE_METHOD(BooksJsonPathTestFixture, "Compare to null", "[test]") {
    REQUIRE(jsonPathResult(doc, L"$..book[?(@.category!=null)].title") == std::wstring(
            L"[ \"Sayings of the Century\", \"Moby Dick\", \"The Lord of the Rings\" ]"));
}


TEST_CASE_METHOD(BooksJsonPathTestFixture, "Wildcard navstep result", "[test]") {
    REQUIRE(jsonPathResult(doc, L"$.store.*") == std::wstring(
            L"[ [ { \"category\": \"reference\", "
            "\"author\": \"Nigel Rees\", "
            "\"title\": \"Sayings of the Century\", "
            "\"price\": 8.9499999999999992895 }, "
            "{ \"category\": \"fiction\", "
            "\"author\": \"Herman Melville\", "
            "\"title\": \"Moby Dick\", "
            "\"isbn\": \"0-553-21311-3\", "
            "\"price\": 8.9900000000000002132 }, "
            "{ \"category\": \"fiction\", "
            "\"author\": \"J.R.R. Tolkien\", "
            "\"title\": \"The Lord of the Rings\", "
            "\"isbn\": \"0-395-19395-8\", "
            "\"price\": 22.989999999999998437 } ], "
            "{ \"color\": \"red\", "
            "\"price\": 19.949999999999999289 } ]"));
}

TEST_CASE_METHOD(BooksJsonPathTestFixture, "Wildcard subscript result", "[test]") {
    REQUIRE(jsonPathResult(doc, L"$.store.book[*]") == std::wstring(
            L"[ { \"category\": \"reference\", "
            "\"author\": \"Nigel Rees\", "
            "\"title\": \"Sayings of the Century\", "
            "\"price\": 8.9499999999999992895 }, "
            "{ \"category\": \"fiction\", "
            "\"author\": \"Herman Melville\", "
            "\"title\": \"Moby Dick\", "
            "\"isbn\": \"0-553-21311-3\", "
            "\"price\": 8.9900000000000002132 }, "
            "{ \"category\": \"fiction\", "
            "\"author\": \"J.R.R. Tolkien\", "
            "\"title\": \"The Lord of the Rings\", "
            "\"isbn\": \"0-395-19395-8\", "
            "\"price\": 22.989999999999998437 } ]"));
    
    REQUIRE(jsonPathResult(doc, L"$..book[*]") == std::wstring(
            L"[ { \"category\": \"reference\", "
            "\"author\": \"Nigel Rees\", "
            "\"title\": \"Sayings of the Century\", "
            "\"price\": 8.9499999999999992895 }, "
            "{ \"category\": \"fiction\", "
            "\"author\": \"Herman Melville\", "
            "\"title\": \"Moby Dick\", "
            "\"isbn\": \"0-553-21311-3\", "
            "\"price\": 8.9900000000000002132 }, "
            "{ \"category\": \"fiction\", "
            "\"author\": \"J.R.R. Tolkien\", "
            "\"title\": \"The Lord of the Rings\", "
            "\"isbn\": \"0-395-19395-8\", "
            "\"price\": 22.989999999999998437 } ]"));
}

TEST_CASE_METHOD(BooksJsonPathTestFixture, "Step from multiple", "[test]") {
    REQUIRE(jsonPathResult(doc, L"$..book[*].title") == std::wstring(
            L"[ \"Sayings of the Century\", \"Moby Dick\", \"The Lord of the Rings\" ]"));
}

TEST_CASE_METHOD(BooksJsonPathTestFixture, "Path result", "[test]") {
    REQUIRE(jsonPathResult(doc, L"$.store.bicycle.color") == std::wstring(
            L"[ \"red\" ]"));
}

TEST_CASE_METHOD(BooksJsonPathTestFixture, "Recursive result", "[test]") {
    CHECK(jsonPathResult(doc, L"$.store..price") == std::wstring(
            L"[ 19.949999999999999289, 8.9499999999999992895, 8.9900000000000002132, 22.989999999999998437 ]"));
    CHECK(jsonPathResult(doc, L"$..price") == std::wstring(
            L"[ 19.949999999999999289, 8.9499999999999992895, 8.9900000000000002132, 22.989999999999998437 ]"));
}

TEST_CASE_METHOD(BooksJsonPathTestFixture, "Subscripting styles", "[test]") {
    CHECK(jsonPathResult(doc, L"$..book[0]") == std::wstring(
            L"[ { \"category\": \"reference\", "
             "\"author\": \"Nigel Rees\", "
             "\"title\": \"Sayings of the Century\", "
             "\"price\": 8.9499999999999992895 } ]"));
    CHECK(jsonPathResult(doc, L"$..book[0,1].title") == std::wstring(
            L"[ \"Sayings of the Century\", \"Moby Dick\" ]"));
    CHECK(jsonPathResult(doc, L"$..book[:2].title") == std::wstring(
            L"[ \"Sayings of the Century\", \"Moby Dick\" ]"));
    CHECK(jsonPathResult(doc, L"$..book[-1:].title") == std::wstring(
            L"[ \"The Lord of the Rings\" ]"));
}

TEST_CASE_METHOD(BooksJsonPathTestFixture, "Filter by expression", "[test]") {
    SECTION("Coalesce to boolean") {
        CHECK(jsonPathResult(doc, L"$..book[?(@.isbn)]") == std::wstring(
               L"[ { \"category\": \"fiction\", "
               "\"author\": \"Herman Melville\", "
               "\"title\": \"Moby Dick\", "
               "\"isbn\": \"0-553-21311-3\", "
               "\"price\": 8.9900000000000002132 }, "
               "{ \"category\": \"fiction\", "
               "\"author\": \"J.R.R. Tolkien\", "
               "\"title\": \"The Lord of the Rings\", "
               "\"isbn\": \"0-395-19395-8\", "
               "\"price\": 22.989999999999998437 } ]"));

        CHECK(jsonPathResult(doc, L"$..book[?(!@.isbn)]") == std::wstring(
                L"[ { \"category\": \"reference\", "
                "\"author\": \"Nigel Rees\", "
                "\"title\": \"Sayings of the Century\", "
                "\"price\": 8.9499999999999992895 } ]"));
    }
    
    SECTION("Compound boolean") {
        CHECK(jsonPathResult(doc, L"$..book[?(@.category == \"fiction\" || @.category == \"reference\")]") == std::wstring(
                 L"[ { \"category\": \"reference\", "
                 "\"author\": \"Nigel Rees\", "
                 "\"title\": \"Sayings of the Century\", "
                 "\"price\": 8.9499999999999992895 }, "
                 "{ \"category\": \"fiction\", "
                 "\"author\": \"Herman Melville\", "
                 "\"title\": \"Moby Dick\", "
                 "\"isbn\": \"0-553-21311-3\", "
                 "\"price\": 8.9900000000000002132 }, "
                 "{ \"category\": \"fiction\", "
                 "\"author\": \"J.R.R. Tolkien\", "
                 "\"title\": \"The Lord of the Rings\", "
                 "\"isbn\": \"0-395-19395-8\", "
                 "\"price\": 22.989999999999998437 } ]"));
    }

    SECTION("Simple expressions") {
        CHECK(jsonPathResult(doc, L"$..book[?(@.author==\"J.R.R. Tolkien\")].title") == std::wstring(
                L"[ \"The Lord of the Rings\" ]"));

        CHECK(jsonPathResult(doc, L"$..book[?(@.price < 10)]") == std::wstring(
                 L"[ { \"category\": \"reference\", "
                 "\"author\": \"Nigel Rees\", "
                 "\"title\": \"Sayings of the Century\", "
                 "\"price\": 8.9499999999999992895 }, "
                 "{ \"category\": \"fiction\", "
                 "\"author\": \"Herman Melville\", "
                 "\"title\": \"Moby Dick\", "
                 "\"isbn\": \"0-553-21311-3\", "
                 "\"price\": 8.9900000000000002132 } ]"));
    }

    SECTION("Access root context in expression") {
        CHECK(jsonPathResult(doc, L"$..book[?(@.price > $.expensive)]") == std::wstring(
                L"[ { "
                    "\"category\": \"fiction\", "
                    "\"author\": \"J.R.R. Tolkien\", "
                    "\"title\": \"The Lord of the Rings\", "
                    "\"isbn\": \"0-395-19395-8\", "
                    "\"price\": 22.989999999999998437 } ]"));
    }

    SECTION("Use 'in' operator") {
        CHECK(jsonPathResult(doc, L"$..book[?(@.price in [22.99, 8.99, 22])]") == std::wstring(
                 L"[ "
                 "{ \"category\": \"fiction\", "
                 "\"author\": \"Herman Melville\", "
                 "\"title\": \"Moby Dick\", "
                 "\"isbn\": \"0-553-21311-3\", "
                 "\"price\": 8.9900000000000002132 }, "
                 "{ \"category\": \"fiction\", "
                 "\"author\": \"J.R.R. Tolkien\", "
                 "\"title\": \"The Lord of the Rings\", "
                 "\"isbn\": \"0-395-19395-8\", "
                 "\"price\": 22.989999999999998437 } ]"));
    }
}

TEST_CASE_METHOD(BooksJsonPathTestFixture, "Subscript by expression", "[test]") {
    CHECK(jsonPathResult(doc, L"$..book[(pow(2, $.expensive-8)/2)]") == std::wstring(
            L"[ "
           "{ \"category\": \"fiction\", "
               "\"author\": \"J.R.R. Tolkien\", "
               "\"title\": \"The Lord of the Rings\", "
               "\"isbn\": \"0-395-19395-8\", "
               "\"price\": 22.989999999999998437 } ]"));
    
}
