//
//  JsonIndentFormatter.hpp
//  Bracez
//
//  Created by Eldan Ben Haim on 01/03/2021.
//

#ifndef JsonIndentFormatter_hpp
#define JsonIndentFormatter_hpp

#include <string>
#include "json_file.h"
#include "BookmarksList.h"
#include "reader.h"

struct TokenStreamAndUnderlying {
    TokenStreamAndUnderlying(const std::wstring &text,
                             unsigned long startOffset,
                             int startRow, int startCol)
    : inputStream(text.c_str(), text.size(), NULL, TextCoordinate(startOffset)),
    tokenStream(inputStream, NULL, false, startRow, startCol)
    {
    }
    
    json::InputStream inputStream;
    json::TokenStream tokenStream;
};

class JsonIndentationContext {
public:
    int currentIndent();
    
    void pushIndentLevel();
    void popIndentLevel();
    
public:
    static JsonIndentationContext approximateWithTokenStream(json::TokenStream &aTokStream, TextCoordinate aOffset, int aIndentSize);
    
    static JsonIndentationContext approximateWithDocument(const json::JsonFile &file,
                                                          TextCoordinate aOffset, int aIndentSize);
    
private:
    JsonIndentationContext(int initialIndent, int indentSize);
    JsonIndentationContext(const std::vector<int> &&indentLevels, int indentSize);
    
    void setInitialIndentLevel(int level);
    
    std::vector<int> indentLevelStack;
    int indentSize;
};

class JsonIndentFormatter {
public:
    JsonIndentFormatter(const std::wstring &text,
                        const json::JsonFile &jsonFile,
                        TextCoordinate aOffsetStart, TextLength aLen, int indentSize);
    
    const std::wstring &getIndented();
    TextLength getIndentedLength();
    
private:
    void reindent();
    
    void skipInputWhitespace();
    void outputIndent();
    void copyAndEnsureNewLine();
    
    void outputString(const std::wstring &string);
    
private:
    
    TokenStreamAndUnderlying tokStreamAndCo;
    JsonIndentationContext indentationContext;
    
    TextCoordinate startOffset;
    TextCoordinate endOffset;
    
    unsigned int outputCol;
    std::wstring output;
};

#endif /* JsonIndentFormatter_hpp */
