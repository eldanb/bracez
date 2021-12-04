/**********************************************
 
 License: BSD
 Project Webpage: http://cajun-jsonapi.sourceforge.net/
 Author: Terry Caton
 
 ***********************************************/

#include <cassert>
#include <set>
#include <sstream>
#include <codecvt>

// convert wstring to UTF-8 string
static std::string wstring_to_utf8 (const std::wstring& str)
{
    std::wstring_convert< std::codecvt_utf8<wchar_t> > myconv;
    return myconv.to_bytes(str);
}

namespace json
{


//////////////////////
// Reader::InputStream


inline InputStream::InputStream(const wchar_t *aBuffer, TextLength aLength, ParseListener *aListener,
                                TextCoordinate aStartLocation) :
m_Buffer(aBuffer), m_Location(aStartLocation), m_parseListener(aListener), m_Length(aLength) {
    
}



inline wchar_t InputStream::Peek() {
    assert(!EOS());
    return *CurrentPtr();
}

inline const wchar_t *InputStream::CurrentPtr() const {
    return m_Buffer + m_Location;
}

inline bool InputStream::EOS() const {
    return m_Location >= m_Length;
}

inline void InputStream::seek(unsigned long where) {
    m_Location = where;
}

inline bool InputStream::VerifyString(const std::wstring &sExpected)
{
    unsigned long expectedLen = sExpected.length();
    const wchar_t *expected = sExpected.c_str();
    if(expectedLen > m_Length-m_Location) {
        return false;
    }
    
    return !memcmp(expected, CurrentPtr(), sizeof(wchar_t)*expectedLen);
}


inline wchar_t InputStream::Get()
{
    wchar_t c = *CurrentPtr();
    m_Location = m_Location + 1;
    if (c == L'\n') {
        if(m_parseListener)
        {
            m_parseListener->EndOfLine(TextCoordinate(m_Location-1));
        }
    }
    
    return c;
}

inline TokenStream::TokenStream(InputStream& aInputStream,
                                ParseListener *aListener,
                                bool aSkipWhitespace,
                                int startRow,
                                int startCol) :
inputStream(aInputStream),
tokenEaten(true),
listener(aListener),
skipWhitespace(aSkipWhitespace),
currentRow(startRow),
currentCol(startCol)
{
    // Prepare lookup tables
    memset(tokenTypeLookup, Token::TOKEN_UNKNOWN, 256);
    
    
    tokenTypeLookup['\n'] = Token::TOKEN_WHITESPACE;
    tokenTypeLookup['\t'] = Token::TOKEN_WHITESPACE;
    tokenTypeLookup[' '] = Token::TOKEN_WHITESPACE;
    
    tokenTypeLookup['{'] = Token::TOKEN_OBJECT_BEGIN;
    tokenTypeLookup['}'] = Token::TOKEN_OBJECT_END;
    tokenTypeLookup['['] = Token::TOKEN_ARRAY_BEGIN;
    tokenTypeLookup[']'] = Token::TOKEN_ARRAY_END;
    tokenTypeLookup[','] = Token::TOKEN_NEXT_ELEMENT;
    tokenTypeLookup[':'] = Token::TOKEN_MEMBER_ASSIGN;
    
    tokenTypeLookup['-'] = Token::TOKEN_NUMBER;
    for(int i='0'; i<='9'; i++) tokenTypeLookup[i] = Token::TOKEN_NUMBER;
    
    tokenTypeLookup['"'] =  Token::TOKEN_STRING;
    
    
    memset(charClassLookup, Token::CHAR_CLASS_UNKNOWN, 256);
    const char lNumericChars[] = "0123456789";
    for(const char *lChar = lNumericChars; *lChar; lChar++) charClassLookup[*lChar] = Token::CHAR_CLASS_NUMERIC;
    
    
    EatWhiteSpace();              // ignore any leading white space...
}

inline const Token& TokenStream::Peek() {
    pumpTokenIfNeeded();
    
    return currentToken;
}

inline const Token& TokenStream::Get() {
    assert(!EOS());
    pumpTokenIfNeeded();
    tokenEaten = true;
    
    return currentToken;
}

inline bool TokenStream::EOS() const {
    return tokenEaten && inputStream.EOS();
}

inline int TokenStream::Row() {
    return currentRow;
}

inline int TokenStream::Col() {
    return currentCol;
}

inline void TokenStream::pumpTokenIfNeeded()
{
    if(tokenEaten)
    {
        assert(!EOS());
        pumpToken();
        tokenEaten = false;
    }
}

inline void TokenStream::pumpToken()
{
    // Mark token start
    currentToken.locBegin = inputStream.GetLocation();
    
    // Get current token type (good guess...)
    wchar_t sChar = inputStream.Peek();
    currentToken.nType = (Token::Type)tokenTypeLookup[sChar];
    
    switch (currentToken.nType)
    {
        case Token::TOKEN_NUMBER:
            MatchNumber();
            break;
            
        case Token::TOKEN_STRING:
            MatchString();
            break;
            
        case Token::TOKEN_UNKNOWN:
            MatchBareWordToken();
            break;
            
        default:   // Default case is for simple tokens
            currentToken.orgTextStart = inputStream.CurrentPtr();
            inputStream.Get();
            currentToken.orgTextEnd = inputStream.CurrentPtr();
            currentToken.assumeValueFromOrgText();
            updateLineCol(sChar);
    }
    
    currentToken.locEnd = inputStream.GetLocation();
    
    EatWhiteSpace();              // Skip to next real token
}

inline void TokenStream::EatWhiteSpace()
{
    if(skipWhitespace) {
        while (inputStream.EOS() == false &&
               ::isspace(inputStream.Peek()))
            updateLineCol(inputStream.Get());
    }
}

inline void TokenStream::Match4Hex(unsigned int& integer) {
    std::wstringstream ss;
    int i;
    for (i = 0; (i < 4) && (inputStream.EOS() == false) && isxdigit(inputStream.Peek()); ++i)
        ss << inputStream.Get();
    
    if(listener)
        listener->Error(inputStream.GetLocation(), PARSER_ERROR_UNEXPECTED_CHARACTER, "Expected a hex digit");
    
    ss << std::hex;
    ss >> integer;
}



inline void TokenStream::MatchString()
{
    std::wstring tokValue;
    
    
    // Eat starting "\""
    currentToken.orgTextStart = inputStream.CurrentPtr();
    inputStream.Get();
    
    // Initialize value
    currentToken.clearValue();
    currentToken.valueStart = inputStream.CurrentPtr();
    
    while (!inputStream.EOS()  &&
           inputStream.Peek() != L'"')
    {
        wchar_t c = inputStream.Get();
        
        updateLineCol(c);
        
        // escape?
        if (c == L'\\' &&
            inputStream.EOS() == false) // shouldn't have reached the end yet
        {
            // Prepare 'ownbuff' if not ready
            if(tokValue.empty()) {
                tokValue = std::wstring(currentToken.valueStart, inputStream.CurrentPtr()-1);
            }
            
            if(!ProcessStringEscape(tokValue)) {
                std::string sMessage = "Unrecognized escape sequence found in string: \\" + wstring_to_utf8(wstring(&c, 1));
                if(listener)
                    listener->Error(inputStream.GetLocation(), PARSER_ERROR_UNEXPECTED_CHARACTER, sMessage);
            }
        }
        else {
            if(!tokValue.empty()) {
                tokValue.push_back(c);
            }
        }
    }
    
    // Prepare token value: either directly refer underlying or create own buffer
    if(tokValue.empty()) {
        currentToken.valueEnd = inputStream.CurrentPtr();
    } else {
        currentToken.valueBuff = wcsdup(tokValue.c_str());
        currentToken.valueStart = currentToken.valueBuff;
        currentToken.valueEnd = currentToken.valueBuff + wcslen(currentToken.valueBuff);
    }
    
    if(!inputStream.EOS()) {
        // eat the last '"' that we just peeked
        inputStream.Get();
    } else {
        std::string sMessage = "Unterminated string constant.";
        if(listener)
            listener->Error(inputStream.GetLocation(), PARSER_ERROR_UNEXPECTED_EOS, sMessage);
    }
    
    // Store end of token
    currentToken.orgTextEnd = inputStream.CurrentPtr();
    
}

inline void TokenStream::MatchBareWordToken()
{
    currentToken.clearValue();
    
    currentToken.orgTextStart = inputStream.CurrentPtr();
    wchar_t c;
    while(!inputStream.EOS() &&
          !::isspace(c=inputStream.Peek()) &&
          c != L']' && c != L'}' && c != L',' && c != L':'  // Pretty ugly hack
          )
    {
        updateLineCol(c);
        inputStream.Get();
    }
    currentToken.orgTextEnd = inputStream.CurrentPtr();
    currentToken.assumeValueFromOrgText();
    
    if(currentToken.isValueEquals(L"true") || currentToken.isValueEquals(L"false"))
    {
        currentToken.nType = Token::TOKEN_BOOLEAN;
    } else
        if(currentToken.isValueEquals(L"null"))
        {
            currentToken.nType = Token::TOKEN_NULL;
        } else
        {
            
            std::string sErrorMessage = "Unknown token in stream: " + wstring_to_utf8(currentToken.value());
            if(listener)
                listener->Error(currentToken.locBegin, PARSER_ERROR_UNEXPECTED_CHARACTER, sErrorMessage);
        }
}

inline void TokenStream::MatchNumber()
{
    currentToken.clearValue();
    currentToken.orgTextStart = inputStream.CurrentPtr();
    
    enum NumParseState {
        START,
        BODY,
        EXPONENT_START,
        EXPONENT
    } state = START;
    
    
    while (inputStream.EOS() == false)
    {
        char curChar = inputStream.Peek();
        auto curCharClass = charClassLookup[inputStream.Peek()];
        bool validChar = true;
        
        switch(state) {
            case START:
                if(curChar == '-' || curCharClass == Token::CHAR_CLASS_NUMERIC) {
                    state = BODY;
                } else {
                    validChar = false;
                }
                break;
                
            case BODY:
                if(curChar == 'e' || curChar == 'E') {
                    state = EXPONENT_START;
                } else
                    if( curCharClass != Token::CHAR_CLASS_NUMERIC && curChar != '.') {
                        validChar = false;
                    }
                break;
                
            case EXPONENT_START:
                if(curChar == '+' || curChar == '-' || curCharClass == Token::CHAR_CLASS_NUMERIC) {
                    state = EXPONENT_START;
                } else {
                    validChar = false;
                }
                break;
                
            case EXPONENT:
                if(curCharClass != Token::CHAR_CLASS_NUMERIC) {
                    validChar = false;
                }
                break;
        }
        
        if(validChar) {
            updateLineCol(inputStream.Get());
        } else {
            break;
        }
    }
    
    currentToken.orgTextEnd = inputStream.CurrentPtr();
    currentToken.assumeValueFromOrgText();
}

inline void TokenStream::updateLineCol(wchar_t forChar) {
    if(prevNewLine) {
        currentCol = 0;
        currentRow++;
        prevNewLine = false;
    } else {
        currentCol++;
        if(forChar == '\n') {
            prevNewLine = true;
        }
    }
}

///////////////////
// Reader (finally)

inline Reader::Reader(ParseListener *aParseListener) : listener(aParseListener)
{
}

inline unsigned long Reader::Read(ObjectNode*& object, const std::wstring& istr)   { return Read_i(object, istr); }
inline unsigned long Reader::Read(ArrayNode*& array, const std::wstring& istr)     { return Read_i(array, istr); }
inline unsigned long Reader::Read(StringNode*& string, const std::wstring& istr)   { return Read_i(string, istr); }
inline unsigned long Reader::Read(NumberNode*& number, const std::wstring& istr)   { return Read_i(number, istr); }
inline unsigned long Reader::Read(BooleanNode*& boolean, const std::wstring& istr) { return Read_i(boolean, istr); }
inline unsigned long Reader::Read(NullNode*& null, const std::wstring& istr)       { return Read_i(null, istr); }
inline unsigned long Reader::Read(Node*& unknown, const std::wstring& istr, ParseListener *aParseListener, bool allowSuffix)       { return Read_i(unknown, istr, aParseListener, allowSuffix); }


template <typename ElementTypeT>   
unsigned long Reader::Read_i(ElementTypeT& element,
                             const  std::wstring& istr,
                             ParseListener *aParseListener,
                             bool allowSuffix)
{
    Reader reader(aParseListener);
    
    InputStream inputStream(istr.c_str(), istr.size(), aParseListener);
    TokenStream tokenStream(inputStream, aParseListener);
    reader.Parse(element, tokenStream, allowSuffix);
    
    return inputStream.GetLocation();
}

template<typename T>
inline void Reader::Parse(T &element, TokenStream &tokenStream, bool allowSuffix, TextCoordinate aBaseOfs) {
    Parse(element, tokenStream, aBaseOfs);
    
    if (tokenStream.EOS() == false && !allowSuffix)
    {
        const Token& token = tokenStream.Peek();
        std::string sMessage = "Expected End of token stream; found " + wstring_to_utf8(token.value());
        listener->Error(token.locBegin, PARSER_ERROR_EXPECTED_EOS, sMessage);
    }
}

inline void Reader::Parse(Node*& element, TokenStream& tokenStream, TextCoordinate aBaseOfs)
{
    if (tokenStream.EOS()) {
        std::string sMessage = "Unexpected end of token stream";
        listener->Error(TextCoordinate(), PARSER_ERROR_UNEXPECTED_EOS, "Unexpected end of token stream");
        return;
    }
    
    const Token& token = tokenStream.Peek();
    switch (token.nType) {
        case Token::TOKEN_OBJECT_BEGIN:
        {
            // implicit non-const cast will perform conversion for us (if necessary)
            ObjectNode* object;
            Parse(object, tokenStream, aBaseOfs);
            element = object;
            break;
        }
            
        case Token::TOKEN_ARRAY_BEGIN:
        {
            ArrayNode* array;
            Parse(array, tokenStream, aBaseOfs);
            element = array;
            break;
        }
            
        case Token::TOKEN_STRING:
        {
            StringNode* string;
            Parse(string, tokenStream, aBaseOfs);
            element = string;
            break;
        }
            
        case Token::TOKEN_NUMBER:
        {
            NumberNode* number;
            Parse(number, tokenStream, aBaseOfs);
            element = number;
            break;
        }
            
        case Token::TOKEN_BOOLEAN:
        {
            BooleanNode* boolean;
            Parse(boolean, tokenStream, aBaseOfs);
            element = boolean;
            break;
        }
            
        case Token::TOKEN_NULL:
        {
            NullNode* null;
            Parse(null, tokenStream, aBaseOfs);
            element = null;
            break;
        }
            
        default:
        {
            std::string sMessage = "Unexpected token: " + wstring_to_utf8(token.value());
            listener->Error(token.locBegin, PARSER_ERROR_UNEXPECTED_TOKEN, sMessage);
            tokenStream.Get();
        }
    }
}

static inline bool IsObjectMemberTerminator(Token::Type t) {
    return t == Token::TOKEN_OBJECT_END || t == Token::TOKEN_NEXT_ELEMENT;
}

inline void Reader::Parse(ObjectNode*& object, TokenStream& tokenStream, TextCoordinate aBaseOfs)
{
    
    TextCoordinate lBegin(tokenStream.Peek().locBegin);
    
    MatchExpectedToken(Token::TOKEN_OBJECT_BEGIN, tokenStream);
    
    object = new ObjectNode();
    
    for(bool bContinue = (tokenStream.EOS() == false &&
                          tokenStream.Peek().nType != Token::TOKEN_OBJECT_END);
        bContinue;
        bContinue = ParseSeparatorOrTerminator(tokenStream, Token::TOKEN_OBJECT_END))
    {
        if(IsObjectMemberTerminator(tokenStream.Peek().nType)) {
            listener->Error(tokenStream.Peek().locBegin-1, PARSER_ERROR_UNEXPECTED_TOKEN, "Expected object member name");
            continue;
        }
        
        // first the member name. save the token in case we have to throw an exception
        Token tokenName = MatchExpectedToken(Token::TOKEN_STRING, tokenStream);
        
        if(IsObjectMemberTerminator(tokenStream.Peek().nType)) {
            listener->Error(tokenStream.Peek().locBegin-1, PARSER_ERROR_UNEXPECTED_TOKEN, "Expected ':'");
            continue;
        }
        
        // ...then the key/value separator...
        MatchExpectedToken(Token::TOKEN_MEMBER_ASSIGN, tokenStream);
        
        if(IsObjectMemberTerminator(tokenStream.Peek().nType)) {
            listener->Error(tokenStream.Peek().locBegin-1, PARSER_ERROR_UNEXPECTED_TOKEN, "Expected object member value");
            continue;
        }
        
        // ...then the value itself (can be anything).
        Node *nodeVal = NULL;
        Parse(nodeVal, tokenStream, lBegin);
        
        // try adding it to the object (this could throw)
        if(nodeVal) {
            try
            {
                ObjectNode::Member &member = object->domAddMemberNode(tokenName.value(), nodeVal);
                member.nameRange = TextRange(tokenName.locBegin.relativeTo(lBegin),
                                             tokenName.locEnd.relativeTo(lBegin));
            }
            catch (Exception&)
            {
                // must be a duplicate name
                std::string sMessage = "Duplicate object member: " + wstring_to_utf8(tokenName.value());
                listener->Error(tokenName.locBegin, PARSER_ERROR_DUPLICATE_MEMBER, sMessage);
            }
        } else {
            std::string sMessage = "Could not parse member value '" + wstring_to_utf8(tokenName.value()) + "'";
            listener->Error(tokenName.locBegin, PARSER_ERROR_INVALID_MEMBER, sMessage);
        }
    }
    
    if(tokenStream.EOS())
    {
        listener->Error(tokenStream.getInputStream().GetLocation(), PARSER_ERROR_UNEXPECTED_EOS, "Unexpected end of file");
        object->textRange = TextRange(lBegin.relativeTo(aBaseOfs), tokenStream.getInputStream().GetLocation().relativeTo(aBaseOfs));
    } else
    {
        TextCoordinate lEndCoord = MatchExpectedToken(Token::TOKEN_OBJECT_END, tokenStream).locEnd;
        object->textRange = TextRange(lBegin.relativeTo(aBaseOfs), lEndCoord.relativeTo(aBaseOfs));
    }
}

inline bool Reader::ParseSeparatorOrTerminator(TokenStream& tokenStream, Token::Type terminator) {
    if(tokenStream.EOS()) {
        return false;
    }
    
    bool encounteredNext = false;
    bool eosEncountered;
    Token nextToken;
    while(!(eosEncountered = tokenStream.EOS()) &&
          ((nextToken = tokenStream.Peek()).nType == Token::TOKEN_NEXT_ELEMENT)) {
        tokenStream.Get();
        encounteredNext = true;
    }
    
    if(nextToken.nType == terminator) {
        if(encounteredNext) {
            // TODO nicer error
            ReportExpectedToken(Token::TOKEN_NEXT_ELEMENT, nextToken);
        }
        return false;
    } else
        if(eosEncountered) {
            return false;
        }
        else {
            if(!encounteredNext) {
                ReportExpectedToken(Token::TOKEN_NEXT_ELEMENT, nextToken);
            }
        }
    
    return true;
}

inline void Reader::Parse(ArrayNode*& array, TokenStream& tokenStream, TextCoordinate aBaseOfs)
{
    TextCoordinate lBegin(tokenStream.Peek().locBegin);
    
    MatchExpectedToken(Token::TOKEN_ARRAY_BEGIN, tokenStream);
    
    array = new ArrayNode();
    
    for (bool bContinue = (tokenStream.EOS() == false &&
                           tokenStream.Peek().nType != Token::TOKEN_ARRAY_END);
         bContinue;
         bContinue = ParseSeparatorOrTerminator(tokenStream, Token::TOKEN_ARRAY_END))
    {
        Node *elemVal = NULL;
        Parse(elemVal, tokenStream, lBegin);
        
        if(elemVal) {
            array->domAddElementNode(elemVal);
        }
    }
    
    if(tokenStream.EOS()) {
        listener->Error(TextCoordinate(0), PARSER_ERROR_UNEXPECTED_EOS, "Expecting \",\" or \"]\"");
        array->textRange = TextRange(lBegin.relativeTo(aBaseOfs), TextCoordinate::infinity);
    } else {
        TextCoordinate lEnd = MatchExpectedToken(Token::TOKEN_ARRAY_END, tokenStream).locEnd;
        array->textRange = TextRange(lBegin.relativeTo(aBaseOfs), lEnd.relativeTo(aBaseOfs));
    }
}

inline void Reader::Parse(StringNode*& string, TokenStream& tokenStream, TextCoordinate aBaseOfs)
{
    const Token &tok = MatchExpectedToken(Token::TOKEN_STRING, tokenStream);
    TextRange lTokRange(tok.locBegin.relativeTo(aBaseOfs), tok.locEnd.relativeTo(aBaseOfs));
    string = new StringNode(tok.value());
    string->textRange = lTokRange;
}


inline void Reader::Parse(NumberNode*& number, TokenStream& tokenStream, TextCoordinate aBaseOfs)
{
    const Token &tok = MatchExpectedToken(Token::TOKEN_NUMBER, tokenStream);
    TextRange lTokRange(tok.locBegin.relativeTo(aBaseOfs),
                        tok.locEnd.relativeTo(aBaseOfs));
    
    wchar_t *lLastConverted;
    double dValue = wcstod(tok.valueStart, &lLastConverted);
    
    // did we consume all characters in the token?
    if (lLastConverted != tok.valueEnd)
    {
        std::string sMessage = std::string("Unexpected character in NUMBER token");
        listener->Error(tok.locBegin, PARSER_ERROR_MALFORMED_NUMBER, sMessage);
    }
    
    number = new NumberNode(dValue);
    number->textRange = lTokRange;
}


inline void Reader::Parse(BooleanNode*& boolean, TokenStream& tokenStream, TextCoordinate aBaseOfs)
{
    const Token &tok = MatchExpectedToken(Token::TOKEN_BOOLEAN, tokenStream);
    boolean = new BooleanNode(tok.isValueEquals(L"true"));
    TextRange lTokRange(tok.locBegin.relativeTo(aBaseOfs),
                        tok.locEnd.relativeTo(aBaseOfs));
    boolean->textRange = lTokRange;
}


inline void Reader::Parse(NullNode*& aNull, TokenStream& tokenStream, TextCoordinate aBaseOfs)
{
    const Token &tok = MatchExpectedToken(Token::TOKEN_NULL, tokenStream);
    
    aNull = new NullNode();
    TextRange lTokRange(tok.locBegin.relativeTo(aBaseOfs),
                        tok.locEnd.relativeTo(aBaseOfs));
    aNull->textRange = lTokRange;
}


inline void Reader::ReportExpectedToken(Token::Type nExpected, const Token& token) {
    static const char *lTypeNames[] = {
        "'{'",
        "'}'",
        "'['",
        "']'",
        "','",
        "':'",
        "string",
        "number",
        "'true' or 'false'",
        "'null'"
    };
    
    std::string sMessage = "Unexpected token: " + wstring_to_utf8(token.value()) + "; expecting " + lTypeNames[nExpected];
    listener->Error(token.locBegin-1, PARSER_ERROR_UNEXPECTED_TOKEN, sMessage);
}

inline const Token &Reader::MatchExpectedToken(Token::Type nExpected, TokenStream& tokenStream)
{
    static wstring lEmptyStr;
    static Token lEmptyToken;
    
    if (tokenStream.EOS())
    {
        std::string sMessage = "Unexpected end of token stream";
        listener->Error(TextCoordinate(), PARSER_ERROR_UNEXPECTED_EOS, "Unexpected end of token stream");
        return lEmptyToken;
    }
    
    const Token& token = tokenStream.Peek();
    if (token.nType != nExpected)
    {
        ReportExpectedToken(nExpected, token);
    } else
    {
        tokenStream.Get();
    }
    
    return token;
}

} // End namespace
