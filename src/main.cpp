#include<iostream>
#include<fstream>
#include<sstream>
#include<optional>
using namespace std;

enum class TypeOfToken {
    _return,
    int_lit,
    semi
};
struct Token {
    TypeOfToken type;
    optional <string> value;//the type either contain a value or nothing
};
int main(int argc, char* argv[]){
    if (argc !=2) {
        cerr<<"Incorrect usage. Correct usage is..."<<endl;
        cerr<<"hyrdo <input.hy>"<<endl;
        return EXIT_FAILURE;
    }

    fstream file(argv[1],ios::in); // read the path file because of ios::in,
    //fstream is capable of reading and writing.

    if (!file) { //if there was no file, return an error.
        cerr << "Failed to open file."<<endl;;
        return EXIT_FAILURE;
    }


    stringstream buffer;
    buffer << file.rdbuf(); //read the entire file
    string contents = buffer.str();
    // gives everything that was stored in buffer as string.
    cout << contents << endl;
    return EXIT_SUCCESS;
}