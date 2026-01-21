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

std::vector<ScriptParser::EntityScript> ScriptParser::parse(const char* scriptText)
{
    std::vector<EntityScript> scripts;
    inputText = scriptText;
    currentIndex = 0;

    while (!isEndOfInput()) {
        std::string token = nextToken();
        if (isEndOfInput()) {
            break;
        }
        if (!token.empty()) {
            scripts.push_back(readEntityScript(token));
        }
    }
    return scripts;
}

ScriptParser::EntityScript ScriptParser::readEntityScript(std::string scriptName)
{
    /*
        Format is as follows:

        {
            eventName
            eventParam1 eventParam2 ...
            {
                actionName param1 param2 ...
                actionName param1 param2 ...
            }
        }
    */
    EntityScript entityScript;
    entityScript.name = scriptName;

    std::string token = nextToken();
    if (token != "{") {
        return entityScript;    // TODO: throw error
    }

    while (true) {
        token = nextToken();
        if (token == "}" || isEndOfInput()) {
            break;
        }

        Event event;

        event.name = token;

        // Read event parameters
        while (true) {
            token = nextToken();
            if (token == "{" || isEndOfInput()) {
                break;
            }
            event.parameters.push_back(token);
        }

        // Read actions
        while (true) {
            token = nextToken();
            if (token == "}") {
                break;
            }
            EventActions action;
            action.name = token;

            // Read action parameters up to the end of a line
            while (true) {
                token = nextToken(false);
                if (token == "\n" || isEndOfInput()) {
                    break;
                }
                action.parameters.push_back(token);
            }
            event.actions.push_back(action);
        }

        entityScript.events.push_back(event);
    }

    return entityScript;
}

void ScriptParser::skipWhitespace(bool skipLinefeed)
{
    if (skipLinefeed) {
        while (inputText[currentIndex] == ' ' || inputText[currentIndex] == '\n' || inputText[currentIndex] == '\r' || inputText[currentIndex] == '\t') {
            currentIndex++;
        }
    } else {
        while (inputText[currentIndex] == ' ' || inputText[currentIndex] == '\r' || inputText[currentIndex] == '\t') {
            currentIndex++;
        }
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

std::string ScriptParser::nextToken(bool skipLinefeed)
{
    while (true) {
        skipWhitespace(skipLinefeed);
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
            if (!skipLinefeed && ch == '\n') {
                unreadChar(); // put back the newline for linefeed handling
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

    if (ch == '\n') {
        return "\n";
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
