//
//  JsonIndentFormatter.cpp
//  Bracez
//
//  Created by Eldan Ben Haim on 01/03/2021.
//

#include "JsonIndentFormatter.hpp"
#include "reader.h"

static TokenStreamAndUnderlying tokenStreamAtBeginningOfLine(const std::wstring &text,
                                                             LinesAndBookmarks &linesAndBookmarks,
                                                             TextCoordinate aOffset) {
    
    int row, col;
    linesAndBookmarks.getCoordinateRowCol(aOffset, row, col);
    
    TextCoordinate inputOffset = linesAndBookmarks.getLineStart(row) + (row > 1 ? 1 : 0);
    
    return TokenStreamAndUnderlying(text, inputOffset.getAddress(), row-1, 0);
}

/**
 * JsonIndentFormatter
 */

JsonIndentFormatter::JsonIndentFormatter(const std::wstring &text,
                                         LinesAndBookmarks &linesAndBookmarks,
                                         TextCoordinate aOffsetStart,
                                         TextLength aLen,
                                         int indentSize)
    : tokStreamAndCo(tokenStreamAtBeginningOfLine(text, linesAndBookmarks, aOffsetStart)),
      indentationContext(JsonIndentationContext::approximateWithTokenStream(tokStreamAndCo.tokenStream, aOffsetStart, indentSize))
{
    TextLength textLen = (TextLength)text.length();
    startOffset = aOffsetStart;

    if(startOffset+aLen > TextCoordinate(textLen)) {
        aLen = textLen - startOffset;
    }
        
    endOffset = startOffset + aLen;
}

const std::wstring &JsonIndentFormatter::getIndented() {
    if(!output.length()) {
        reindent();
    }
    
    return output;
}

void JsonIndentFormatter::reindent() {
    int peekedInputCol = tokStreamAndCo.tokenStream.Col();
    
    outputCol = peekedInputCol;
    if(peekedInputCol == 0) {
        skipInputWhitespace();
    }
            
    while(!tokStreamAndCo.tokenStream.EOS() &&
            tokStreamAndCo.tokenStream.Peek().locBegin < endOffset) {
        json::Token tok = tokStreamAndCo.tokenStream.Get();
        
        switch(tok.nType) {
            case json::Token::TOKEN_WHITESPACE:
                outputString(tok.sOrgText);
                if(tok.sOrgText == L"\n") {
                    skipInputWhitespace();
                }
                break;
            
            case json::Token::TOKEN_ARRAY_BEGIN:
            case json::Token::TOKEN_OBJECT_BEGIN:
                outputString(tok.sOrgText);
                indentationContext.pushIndentLevel();
                copyAndEnsureNewLine();
                break;
                
            case json::Token::TOKEN_ARRAY_END:
            case json::Token::TOKEN_OBJECT_END:
                indentationContext.popIndentLevel();
                if(outputCol != 0) {
                    outputString(L"\n");
                }
                outputString(tok.sOrgText);
                break;
                
            case json::Token::TOKEN_NEXT_ELEMENT:
                outputString(tok.sOrgText);
                copyAndEnsureNewLine();
                break;
                
            default:
                outputString(tok.sOrgText);
                break;
            
        }
    }
}

void JsonIndentFormatter::skipInputWhitespace() {
    while(!tokStreamAndCo.tokenStream.EOS() && tokStreamAndCo.tokenStream.Peek().nType == json::Token::TOKEN_WHITESPACE) {
        tokStreamAndCo.tokenStream.Get();
    }
}

void JsonIndentFormatter::outputIndent() {
    int indentSize = indentationContext.currentIndent();
    outputCol += indentSize;
    
    while(indentSize--) {
        output.push_back(L' ');
    }
}

void JsonIndentFormatter::copyAndEnsureNewLine() {
    while(!tokStreamAndCo.inputStream.EOS()) {
        json::Token tok = tokStreamAndCo.tokenStream.Peek();

        if(tok.nType == json::Token::TOKEN_WHITESPACE) {
            tokStreamAndCo.tokenStream.Get();
            if(tok.sOrgText == L"\n") {
                outputString(tok.sOrgText);
                skipInputWhitespace();
                return;
            }
        } else {
            break;
        }
    }
    
    outputString(L"\n");
}

void JsonIndentFormatter::outputString(const std::wstring &string) {
    for(auto iter = string.begin(); iter != string.end(); iter ++) {
        if(!outputCol) {
            outputIndent();
        }
        
        output.push_back(*iter);
        
        outputCol++;
        
        if(*iter == L'\n') {
            outputCol = 0;
        }
    }
}

/**
 * JsonIndentationContext
 */


JsonIndentationContext JsonIndentationContext::approximateBySingleLine(const std::wstring &text,
                                                                              LinesAndBookmarks &linesAndBookmarks,
                                                                              TextCoordinate aOffset, int aIndentSize) {
    TokenStreamAndUnderlying tokStream = tokenStreamAtBeginningOfLine(text, linesAndBookmarks, aOffset);
    return JsonIndentationContext::approximateWithTokenStream(tokStream.tokenStream, aOffset, aIndentSize);
}

JsonIndentationContext JsonIndentationContext::approximateWithTokenStream(json::TokenStream &aTokStream, TextCoordinate aOffset, int aIndentSize) {
    JsonIndentationContext ret;
    
    ret.indentSize = aIndentSize;
    
    bool inIndent = true;
    
    while(aTokStream.getInputStream().GetLocation() < aOffset) {
        json::Token tok = aTokStream.Get();
        
        if(inIndent) {
            if(tok.nType == json::Token::TOKEN_WHITESPACE) {
                ret.setInitialIndentLevel(aTokStream.Col());
            } else {
                inIndent = false;
            }
        }
        
        switch(tok.nType) {
            case json::Token::TOKEN_OBJECT_BEGIN:
            case json::Token::TOKEN_ARRAY_BEGIN:
                ret.pushIndentLevel();
                break;
                
            case json::Token::TOKEN_OBJECT_END:
            case json::Token::TOKEN_ARRAY_END:
                ret.popIndentLevel();
                break;
                
            default:
                break;
        }
    }
    
    return ret;
}

void JsonIndentationContext::pushIndentLevel() {
    indentLevelStack.push_back(indentLevelStack.back() + indentSize);
}

void JsonIndentationContext::popIndentLevel() {
    if(indentLevelStack.size() > 1) {
        indentLevelStack.pop_back();
    } else
    {
        indentLevelStack.back() = std::max(indentLevelStack.back()-indentSize, 0);
        
    }
}

int JsonIndentationContext::currentIndent() {
    return indentLevelStack.back();
}

JsonIndentationContext::JsonIndentationContext() {
    indentLevelStack.push_back(0);
}

void JsonIndentationContext::setInitialIndentLevel(int level) {
    indentLevelStack.front() = level;
}
