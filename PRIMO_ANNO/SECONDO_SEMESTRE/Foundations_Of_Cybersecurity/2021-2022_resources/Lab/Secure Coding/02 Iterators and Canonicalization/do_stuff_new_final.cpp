#include <string>
#include <iostream>
#include <fstream>

using namespace std;

void do_stuff_new_final(const string& str1, const string& str2){
// NOTE: For esoteric reasons, some code lines are UNCHANGEABLE.
    // PART #1
    if(str1.empty()) return;
    string command = "ping -c 1 " + str1; // UNCHANGEABLE!
    int result1 = system(command.c_str()); // UNCHANGEABLE!

    // PART #2
    ifstream f(str2, ios::in); // only files in the home directory or its subdirs should be opened here!
    if(!f) { cerr << "Cannot open " << str2 << endl; return; }
    cout << "Content of " << str2 << ":" << endl;
    string line;
    do{
        getline(f, line);
        cout << line << endl;
    }
    while(!f.eof());
    f.close();

    // PRINT RESULTS
    cout << "result1=" << result1 << "\n";
}

int main() {
    string str1;
    string str2;

    cout << "Welcome to do_stuff_new_final() invoker." << endl;
    cout << endl;

    for(;;){
        cout << "str1 = ";
        getline(cin, str1);
        if(!cin) return 0;
        cout << "str2 = ";
        getline(cin, str2);
        if(!cin) return 0;
        //cin.ignore(numeric_limits<streamsize>::max(), '\n'); // clear cin buffer
        cout << "Invoking do_stuff_new_final(" << str1 << "," << str2 << ")..." << endl;
        do_stuff_new_final(str1, str2);
        cout << endl;
    }
    return 0;
}
