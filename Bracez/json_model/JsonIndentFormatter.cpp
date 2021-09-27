//
//  JsonIndentFormatter.cpp
//  Bracez
//
//  Created by Eldan Ben Haim on 01/03/2021.
//

#include "JsonIndentFormatter.hpp"
#include "reader.h"

static TokenStreamAndUnderlying tokenStreamAtBeginningOfLine(const std::wstring &text,
                                                             const json::JsonFile &file,
                                                             TextCoordinate aOffset) {
    
    int row, col;
    file.getCoordinateRowCol(aOffset, row, col);
    
    TextCoordinate inputOffset = file.getLineStart(row) + (row > 1 ? 1 : 0);
    
    return TokenStreamAndUnderlying(text, inputOffset.getAddress(), row-1, 0);
}

/**
 * JsonIndentFormatter
 */

JsonIndentFormatter::JsonIndentFormatter(const std::wstring &text,
                                         const json::JsonFile &jsonFile,
                                         TextCoordinate aOffsetStart,
                                         TextLength aLen,
                                         int indentSize)
    : tokStreamAndCo(tokenStreamAtBeginningOfLine(text, jsonFile, aOffsetStart)),
      indentationContext(JsonIndentationContext::approximateWithDocument(jsonFile, aOffsetStart, indentSize))
{
    TextLength textLen = (TextLength)text.length();
    startOffset = aOffsetStart;

    if(startOffset+aLen > TextCoordinate(textLen)) {
        aLen = textLen - startOffset;
    }
        
    endOffset = startOffset + aLen;
}

TextLength JsonIndentFormatter::getIndentedLength() {
    if(!output.length()) {
        reindent();
    }

    return endOffset - startOffset;
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
            
    TextCoordinate lastIndented = startOffset;
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
        
        lastIndented = tok.locEnd;
    }
    endOffset = lastIndented;
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


JsonIndentationContext JsonIndentationContext::approximateWithTokenStream(json::TokenStream &aTokStream, TextCoordinate aOffset, int aIndentSize) {
    JsonIndentationContext ret(0, aIndentSize);
    
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

JsonIndentationContext JsonIndentationContext::approximateWithDocument(const json::JsonFile &file,
                                                                       TextCoordinate aOffset, int aIndentSize) {
    int row, col;
    file.getCoordinateRowCol(aOffset, row, col);
    
    TextCoordinate lineStartCoord = aOffset - (col-1);
    const json::Node *startNode = file.FindNodeContaining(lineStartCoord, nullptr, true);
    
    std::vector<int> indentLevels;
    
    const json::ContainerNode *currentContainer = dynamic_cast<const json::ContainerNode*>(startNode);
    if(!currentContainer) {
        return JsonIndentationContext(0, aIndentSize);
    }
    
    TextCoordinate colStartCoord = json::getContainerStartColumnAddr(currentContainer);
    file.getCoordinateRowCol(colStartCoord, row, col);
    indentLevels.insert(indentLevels.begin(), col-1);
    currentContainer = currentContainer->GetParent();
    
    while(currentContainer) {
        TextCoordinate currentContainerColStartCoord = json::getContainerStartColumnAddr(currentContainer);
        file.getCoordinateRowCol(currentContainerColStartCoord, row, col);
        indentLevels.insert(indentLevels.begin(), col-1);
        currentContainer = currentContainer->GetParent();
    }

    JsonIndentationContext ret(std::move(indentLevels), aIndentSize);
    ret.pushIndentLevel();
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

void JsonIndentationContext::setInitialIndentLevel(int level) {
    indentLevelStack.front() = level;
}

JsonIndentationContext::JsonIndentationContext(const std::vector<int> &&indentLevels, int indentSize)
    : indentSize(indentSize),
      indentLevelStack(indentLevels)
{
    
}

JsonIndentationContext::JsonIndentationContext(int initialIndent, int indentSize)
    : indentSize(indentSize)
{
    indentLevelStack.push_back(initialIndent);
}
