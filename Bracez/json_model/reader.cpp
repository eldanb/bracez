//
//  reader.cpp
//  Bracez
//
//  Created by Eldan Ben-Haim on 29/11/2021.
//

#include "reader.h"

namespace json {

bool TokenStream::ProcessStringEscape(std::wstring &tokValue) {
    wchar_t c = inputStream.Get();
    switch (c) {
        case L'/':      tokValue.push_back(L'/');     return true;
        case L'"':      tokValue.push_back(L'"');     return true;
        case L'\\':     tokValue.push_back(L'\\');    return true;
        case L'b':      tokValue.push_back(L'\b');    return true;
        case L'f':      tokValue.push_back(L'\f');    return true;
        case L'n':      tokValue.push_back(L'\n');    return true;
        case L'r':      tokValue.push_back(L'\r');    return true;
        case L't':      tokValue.push_back(L'\t');    return true;
        case L'u':
        {
            unsigned int codepoint = 0, surrogate = 0;
            Match4Hex(codepoint);
            if ((codepoint & 0xFC00) == 0xD800) { // Surrogate?
                if(inputStream.Peek() == L'\\')
                {
                    inputStream.Get();
                    if(inputStream.Peek() == L'u')
                    {
                        inputStream.Get();
                        Match4Hex(surrogate);
                        codepoint = (((codepoint & 0x3F) << 10) |
                                     ((((codepoint >> 6) & 0xF) + 1) << 16) |
                                     (surrogate & 0x3FF));
                    } else
                    {
                        if(listener)
                            listener->Error(inputStream.GetLocation(), PARSER_ERROR_UNEXPECTED_CHARACTER, "Expected \\u after surrogate character.");
                    }
                } else
                {
                    if(listener)
                        listener->Error(inputStream.GetLocation(), PARSER_ERROR_UNEXPECTED_CHARACTER, "Expected \\u after surrogate character.");
                }
            }
            
            tokValue.push_back(codepoint);
            return true;
        }
        default:
            return false;
    }
}

}
