/*
  ==============================================================================

    CommandParser.h
    Created: 27 Jan 2025 8:14:34pm
    Author:  mpue

  ==============================================================================
*/

#pragma once
#include <iostream>
#include <string>
#include <map>
#include <functional>
#include <vector>
#include <sstream>

class CommandParser {
public:
    using CommandFunction = std::function<std::string(const std::vector<std::string>&)>;

    void registerCommand(const std::string& command, const CommandFunction& func, const std::string& description) {
        commands[command] = func;
        descriptions[command] = description;
    }

    std::string parseAndExecute(const std::string& input) {
        std::vector<std::string> tokens = tokenize(input);

        if (tokens.empty()) {
            return "Error: No command entered.";
        }

        const std::string command = tokens[0];
        tokens.erase(tokens.begin()); // Remove the command from the argument list

        if (commands.find(command) != commands.end()) {
            return "\n"+ commands[command](tokens);
        }
        else {
            return "Error: Command not recognized.";
        }
    }

    std::string displayHelp() const {
        std::ostringstream ss;
        ss << "Available commands:\n";
        for (const auto& [cmd, desc] : descriptions) {
            ss << "- " << cmd << ": " << desc << "\n";
        }
        return ss.str();
    }

private:
    std::map<std::string, CommandFunction> commands;
    std::map<std::string, std::string> descriptions;

    std::vector<std::string> tokenize(const std::string& input) {
        std::istringstream stream(input);
        std::vector<std::string> tokens;
        std::string token;
        while (stream >> token) {
            tokens.push_back(token);
        }
        return tokens;
    }


};
