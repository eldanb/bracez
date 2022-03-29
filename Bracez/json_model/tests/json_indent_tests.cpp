//
//  json_indent_tests.cpp
//  BracezTests
//
//  Created by Eldan Ben-Haim on 29/03/2022.
//

#include "json_file.h"
#include "reader.h"
#include "catch2/catch.hpp"
#include <fstream>
#include <string>
#include <codecvt>
#include <locale>
#include "JsonIndentFormatter.hpp"

static bool validateJsonValueInvariantAfterIndent(const std::wstring &aJson,
                                                  int aStartOffset = 0, size_t aLen = -1,
                                                  std::wstring *aOutputText = NULL) {
    if(aLen == -1) {
        aLen = aJson.length();
    }
    
    json::JsonFile jsonFile;
    jsonFile.setText(aJson);
    JsonIndentFormatter indentFormatter(aJson, jsonFile, TextCoordinate(aStartOffset), TextLength(aLen), 3);
    
    std::wstring indented = indentFormatter.getIndented();
    
    std::wstring completeIndentedText = aJson.substr(0, aStartOffset) + indented + aJson.substr(aStartOffset+aLen);
    if(aOutputText) {
        *aOutputText = completeIndentedText;
    }
    
    json::JsonFile jsonIndentedFile;
    jsonIndentedFile.setText(completeIndentedText);
    
    std::wstring s1, s2;
    jsonIndentedFile.getDom()->getChildAt(0)->calculateJsonTextRepresentation(s1);
    jsonFile.getDom()->getChildAt(0)->calculateJsonTextRepresentation(s2);
    
    return s1 == s2;
}

TEST_CASE("JSON indent: oneliner") {
    REQUIRE(validateJsonValueInvariantAfterIndent(L"{\"a\":{\"b\":23}}"));
}


TEST_CASE("JSON indent: whitespace handling") {
    std::wstring firstIndent;
    validateJsonValueInvariantAfterIndent(L" \n   {\"a\":{\"b\":23}}  \n   \n  ", 0, -1, &firstIndent);

    REQUIRE(firstIndent[0] == L'{');
    REQUIRE(firstIndent[firstIndent.length()-1] == L'\n');
}

TEST_CASE("JSON indent: indented is invariant") {
    std::wstring firstIndent;
    validateJsonValueInvariantAfterIndent(L"{\"a\":{\"b\":23}}", 0, -1, &firstIndent);
    
    std::wstring secondIndent;
    validateJsonValueInvariantAfterIndent(firstIndent, 0, -1, &secondIndent);
    
    REQUIRE(firstIndent == secondIndent);
}


