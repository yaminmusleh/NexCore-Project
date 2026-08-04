#pragma once  //Only include this header file once per compilation unit.
//Without it, if the same header gets included multiple times, you can get duplicate definition errors.
#include <vector>
#include <string>
#include <optional>
#include <stdexcept>
#include <cctype>
#include <utility>


enum class TypeOfToken {
    exit,
    int_lit,
    semi,
    identifier,
    open_paren,
    close_paren,
    equals,
    set,
    plus,
    minus,
    star,
    slash,
    open_scope,
    close_scope,
    iff_kw,
    less,
    greater,
    great_or_equal,
    less_or_equal,
    bang_equal, // !=
    condition_equal, // ==
    otherwise_kw, // custom else keyword
    logical_and,
    logical_or,
    logical_not,
};

struct Token {
    TypeOfToken type;
    std::optional<std::string> value{};
};

class Tokenization {
    //Defines a class named Tokenization.
public: //Everything below this line is accessible from outside the class to the objects.

    explicit Tokenization(std::string source)
        : str(std::move(source))
    //This is the constructor. it has the same name as the class. doesn't have a return type.
    //runs automatically when an object is created.
    {
    }

    //need a public method that returns a vector of tokens

    [[nodiscard]] std::vector<Token> tokenize() {
        //scan characters, build a buffer,  create tokens
        std::vector<Token> tokens;
        // storing tokens in the string
        std::string buffer;

        while (auto current = peek()) {
            //while there are characters to look upon

            char c = *current;
            // get current character from peek and since its a vector we make a pointer to point at the value


            if (std::isalpha(c)) {
                //checks if a single character is alphabetic or not

                buffer.clear(); //clearing buffer before doing a new keyword/identifier

                //it will continue reading as long as it's an alphabetic character
                //like these identifiers values: "value1", "count2", etc.

                while (peek() && std::isalnum(*peek())) {
                    // if there was a character to read and that character was alpha-numeric, then push it with consume.
                    buffer.push_back(*consume());
                }

                if (buffer == "exit") {
                    tokens.push_back(Token{TypeOfToken::exit, buffer});
                } else if (buffer == "set") {
                    tokens.push_back(Token{TypeOfToken::set, buffer});
                } else if (buffer == "iff") {
                    tokens.push_back(Token{TypeOfToken::iff_kw, buffer});
                } else if (buffer == "otherwise") {
                    tokens.push_back(Token{TypeOfToken::otherwise_kw, buffer});
                } else {
                    tokens.push_back(Token{TypeOfToken::identifier, buffer});
                }
            } else if (std::isdigit(c)) {
                buffer.clear();

                while (peek() && std::isdigit(*peek())) {
                    buffer.push_back(*consume());
                }

                tokens.push_back(Token{TypeOfToken::int_lit, buffer});
            } else if (c == '(') {
                consume();
                tokens.push_back(Token{TypeOfToken::open_paren});
            } else if (c == ')') {
                consume();
                tokens.push_back(Token{TypeOfToken::close_paren});
            } else if (c == '<') {
                consume();

                if (peek() && *peek() == '=') {
                    consume();
                    tokens.push_back({TypeOfToken::less_or_equal, "<="});
                } else {
                    tokens.push_back({TypeOfToken::less, "<"});
                }
            } else if (c == '>') {
                consume();

                if (peek() && *peek() == '=') {
                    consume();
                    tokens.push_back({TypeOfToken::great_or_equal, ">="});
                } else {
                    tokens.push_back({TypeOfToken::greater, ">"});
                }
            } else if (c == ';') {
                consume(); // move past ';'
                if (peek() && *peek() == ';') {
                    consume(); //second ;
                    while (peek() && *peek() != '\n') {
                        consume();
                        //we don't push tokens here since the comment have no meaning.
                    };
                } else if (peek() && *peek() == '*') {
                    consume();
                    while (true) {
                        if (!peek()) {
                            throw std::runtime_error("Unterminated block comment");
                        }

                        if (*peek() == '*' && peek(1) && *peek(1) == ';') {
                            consume(); // consume *
                            consume(); // consume ;
                            // so we consumed the close *; for the block comment.
                            break;
                        }
                        consume();
                    }
                } else {
                    tokens.push_back(Token{TypeOfToken::semi, ";"});
                }
            } else if (c == '=') {
                consume();

                if (peek() && *peek() == '=') {
                    consume();
                    tokens.push_back({TypeOfToken::condition_equal, "=="});
                } else {
                    tokens.push_back({TypeOfToken::equals, "="});
                }
            } else if (c == '!') {
                consume();

                if (peek() && *peek() == '=') {
                    consume();
                    tokens.push_back({TypeOfToken::bang_equal, "!="});
                }
                else {
                    tokens.push_back(Token{TypeOfToken::logical_not, "!"});
                }
            } else if (c == '+') {
                consume();
                tokens.push_back(Token{TypeOfToken::plus, "+"});
            } else if (c == '-') {
                consume();
                tokens.push_back(Token{TypeOfToken::minus, "-"});
            } else if (c == '*') {
                consume();
                tokens.push_back(Token{TypeOfToken::star, "*"});
            } else if (c == '/') {
                consume();
                tokens.push_back(Token{TypeOfToken::slash, "/"});
            } else if (c == '&') {
                consume();
                tokens.push_back(Token{TypeOfToken::logical_and, "&"});
            } else if (c == '|') {
                consume();
                tokens.push_back(Token{TypeOfToken::logical_or, "|"});
            } else if (c == '{') {
                consume();
                tokens.push_back(Token{TypeOfToken::open_scope, "{"});
            } else if (c == '}') {
                consume();
                tokens.push_back(Token{TypeOfToken::close_scope, "}"});
            } else {
                consume(); // skip unknown characters.
            }
        }

        return tokens;
    }

private: //Everything below this line is accessible from inside the class to the objects.


    //need public methods: peak and consume

    //implement peek: peek() lets you look at a character without moving the current position.

    [[nodiscard]] inline std::optional<char> peek(int offset = 0) const {
        //looking at the next thing without consuming it
        if (index + offset < str.length()) {
            return str[index + offset];
        }

        return {};
    }

    //implement consume: consume() should return the current character and move forward.
    inline std::optional<char> consume() {
        if (index < str.length()) {
            return str[index++]; //returns current then increments the index
        }

        return {};
    }

    //note: consume() and peek() points at the same index of the character but peek() reads it and consume() takes it


    std::string str; // the file contents
    std::size_t index = 0; // current position in the string. index tells you where you are while reading.
};
