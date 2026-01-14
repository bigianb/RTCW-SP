
#pragma once

#include <string>
#include <vector>
#include <map>

class ScriptParser
{
public:
    ScriptParser();
    ~ScriptParser();

    class EventActions
    {
    public:
        std::string name;
        std::vector<std::string> parameters;
    };

    class EntityScript
    {
    public:
        std::string name;
        std::map<std::string, std::vector<ScriptParser::EventActions>> events;
    };

    std::vector<EntityScript> parse(const char* scriptText);

private:
    int currentIndex;
    const char* inputText;

    std::string nextToken();
    void skipWhitespace();
    char peekNextChar();
    char readNextChar();
    void unreadChar();
    bool isEndOfInput();

    EntityScript readEntityScript(std::string scriptName);
};
