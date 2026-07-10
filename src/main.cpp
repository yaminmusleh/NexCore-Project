#include<iostream>
#include<fstream>
#include<sstream>
#include<optional>
#include<vector>
using namespace std;

enum class TypeOfToken {
    _return,
    int_lit,
    semi
};

struct Token {
    TypeOfToken type;
    optional<string> value; //the type either contain a value or nothing
};

vector<Token> tokenize(const string &str) {
    vector<char> buffer{}; //dynamic container
    for (int i = 0; i < str.length(); i++) {
        char c = str.at(i); // this should give us the character
        if (isalpha(c)) {
            // checks if a character is an alphabetic character
            buffer.push_back(c); //used to insert a new element at the very end of a dynamic container
            //It automatically handles the underlying memory management, increasing the total size of the container by exactly one element.
            //basically push_back is like working with a stack.
            i++;
            while (isalnum(str.at(i))) {
                buffer.push_back(str.at(i));
                i++;
            }
            i--;
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        cerr << "Incorrect usage. Correct usage is..." << endl;
        cerr << "hyrdo <input.hy>" << endl;
        return EXIT_FAILURE;
    }

    fstream file(argv[1], ios::in); // read the path file because of ios::in,
    //fstream is capable of reading and writing.

    if (!file) {
        //if there was no file, return an error.
        cerr << "Failed to open file." << endl;;
        return EXIT_FAILURE;
    }


    stringstream buffer;
    buffer << file.rdbuf(); //read the entire file
    string contents = buffer.str();
    // gives everything that was stored in buffer as string.
    tokenize(contents);
    return EXIT_SUCCESS;
}
