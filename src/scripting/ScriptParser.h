
#pragma once

#include <string>

class ScriptParser
{
public:
    ScriptParser();
    ~ScriptParser();

    void parse(const char* scriptText);

private:
    int currentIndex;
    const char* inputText;

    std::string nextToken();
    void skipWhitespace();
    char peekNextChar();
    char readNextChar();
    void unreadChar();
    bool isEndOfInput();
};
