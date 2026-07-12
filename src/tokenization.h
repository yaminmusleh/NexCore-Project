#pragma once  //Only include this header file once per compilation unit.
//Without it, if the same header gets included multiple times, you can get duplicate definition errors.
#include <vector>
#include <string>
#include <optional>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cctype>
using namespace std;


enum class TypeOfToken {
    exit,
    int_lit,
    semi,
    identifier
};

struct Token {
    TypeOfToken type;
    optional<string> value{};
};

class Tokenization {
    //Defines a class named Tokenization.
public: //Everything below this line is accessible from outside the class to the objects.

    explicit Tokenization(string source)
    : str(std::move(source))
    //This is the constructor. it has the same name as the class. doesn't have a return type.
    //runs automatically when an object is created.
    {
    }

    //need a public method that returns a vector of tokens

    [[nodiscard]] vector<Token> tokenize() {
        //scan characters, build a buffer,  create tokens
        vector<Token> tokens;
        // storing tokens in the string
        string buffer;

        while (auto current = peek()) {
            //while there are characters to look upon

            char c = *current;
            // get current character from peek and since its a vector we make a pointer to point at the value


            if (isalpha(c)) {
                //checks if a single character is alphabetic or not

                buffer.clear(); //clearing buffer before doing a new keyword/identifier

                //it will continue reading as long as it's an alphabetic character
                //like these identifiers values: "value1", "count2", etc.

                while (peek() && isalnum(*peek())) {
                    // if there was a character to read and that character was alpha-numeric, then push it with consume.
                    buffer.push_back(*consume());
                }

                if (buffer == "exit") {
                    tokens.push_back(Token{TypeOfToken::exit, buffer});
                } else {
                    tokens.push_back(Token{TypeOfToken::identifier, buffer});
                }
            } else if (isdigit(c)) {
                buffer.clear();

                while (peek() && isdigit(*peek())) {
                    buffer.push_back(*consume());
                }

                tokens.push_back(Token{TypeOfToken::int_lit, buffer});
            } else if (c == ';') {
                consume(); // move past ';'
                tokens.push_back(Token{TypeOfToken::semi, ";"});
            } else {
                consume(); // skip unknown characters.
            }
        }

        return tokens;
    }

private: //Everything below this line is accessible from inside the class to the objects.


    //need public methods: peak and consume

    //implement peek: peek() lets you look at a character without moving the current position.

    [[nodiscard]] optional<char> peek(int offset = 0) const {
        //looking at the next thing without consuming it
        if (index + offset < str.length()) {
            return str[index + offset];
        }

        return {};
    }

    //implement consume: consume() should return the current character and move forward.
    [[nodiscard]] optional<char> consume() {
        if (index < str.length()) {
            return str[index++]; //returns current then increments the index
        }

        return {};
    }

    //note: consume() and peek() points at the same index of the character but peek() reads it and consume() takes it


    string str; // the file contents
    size_t index = 0; // current position in the string. index tells you where you are while reading.
};


/*
 NOTES:
 1.
& means passing by reference.

When a variable is passed by reference, the function receives a reference
to the original variable instead of making a copy. This means any changes
made to the variable inside the function will also affect the original
variable outside the function.

When a variable is passed by value, the function receives a copy of the
variable. Any changes made inside the function only affect the copy and
will not change the original variable.

 2.
: p_filename(filename) this is initialization for the list, if i dont want move() i should put &filename in the constructor.


: p_filename(move(filename)) Treat this object as something I can steal resources from. used when we don't need copies


 3.
[[nodiscard]] : is placed before a function declaration when you want the compiler to warn you if someone ignores the function's return value.
*/
