#include "ScriptParser.h"
#include <iostream>

ScriptParser::ScriptParser()
{
    currentIndex = 0;
    inputText = nullptr;
}

ScriptParser::~ScriptParser()
{
    
}

void ScriptParser::parse(const char* scriptText)
{
    inputText = scriptText;
    currentIndex = 0;

    while (!isEndOfInput()) {
        std::string token = nextToken();
//        std::cout << "Token: " << token << std::endl;
    }
}

void ScriptParser::skipWhitespace()
{
    while (inputText[currentIndex] == ' ' || inputText[currentIndex] == '\n' || inputText[currentIndex] == '\r' || inputText[currentIndex] == '\t') {
        currentIndex++;
    }
}

char ScriptParser::peekNextChar()
{
    return inputText[currentIndex];
}

char ScriptParser::readNextChar()
{
    char ch = inputText[currentIndex];
    if (ch != '\0') {
        currentIndex++;
    }
    return ch;
}

void ScriptParser::unreadChar()
{
    if (currentIndex > 0) {
        currentIndex--;
    }
}

bool ScriptParser::isEndOfInput()
{
    return inputText[currentIndex] == '\0';
}

std::string ScriptParser::nextToken()
{
    while (true) {
        skipWhitespace();
        if (isEndOfInput()) {
            // if we don't trap this here, we will put pack the last character.
            break;
        }
        char ch = readNextChar();
        char nextCh = peekNextChar();
        if (ch == '/' && nextCh == '/') {
            // skip single line comment
            while (ch != '\n' && ch != '\0') {
                ch = readNextChar();
            }
        } else if (ch == '/' && nextCh == '*') {
            // skip multi-line comment
            readNextChar(); // consume '*'
            while (true) {
                ch = readNextChar();
                if (ch == '\0') {
                    break;
                }
                if (ch == '*' && peekNextChar() == '/') {
                    readNextChar(); // consume '/'
                    break;
                }
            }
        } else {
            // not a comment, put back the character
            unreadChar();
            break;
        }
    }

    char ch = readNextChar();
    if (ch == '\0') {
        return "";
    }

    std::string token;
    // Handle quoted strings
    if (ch == '\"') {
        while (true) {
            ch = readNextChar();
            if (ch == '\"' || ch == '\0') {
                break;
            }
            token += ch;
        }
    } else {
        // Handle regular tokens
        while (ch != '\0' && ch != ' ' && ch != '\n' && ch != '\r' && ch != '\t') {
            token += ch;
            ch = readNextChar();
        }
        // Put back the last read character which is not part of the token
        if (ch != '\0') {
            unreadChar();
        }
    }    

    return token;
}
