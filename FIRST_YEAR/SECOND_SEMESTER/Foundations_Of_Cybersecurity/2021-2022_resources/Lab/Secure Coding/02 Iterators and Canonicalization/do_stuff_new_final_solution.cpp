#include <cstring>
#include <string>
#include <iostream>
#include <fstream>

using namespace std;

void do_stuff_new_final(const string& str1, const string& str2){
// NOTE: For esoteric reasons, some code lines are UNCHANGEABLE.
    // PART #1
    if(str1.empty()) return;
    static char ok_chars[] = "abcdefghijklmnopqrstuvwxyz"
                             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                             "1234567890-.:"; // '-' is allowed in hostnames, '.' for IPv4 addresses, ':' for IPv6 addresses
    if(str1.find_first_not_of(ok_chars) != string::npos) return;
    if(str1[0] == '-') return; // avoid option injection by blacklisting (hostnames cannot start with hyphen)
    string command = "ping -c 1 " + str1; // UNCHANGEABLE!
    int result1 = system(command.c_str()); // UNCHANGEABLE!

    // PART #2
    char* canon_str2 = realpath(str2.c_str(), NULL);
    if(!canon_str2) return;
    if(strncmp(canon_str2, "/home/", strlen("/home/")) != 0) { free(canon_str2); return; } // check that directory is "/home" (in some systems this should be: "/home/<username>")
    ifstream f(canon_str2, ios::in); // only files in the home directory or its subdirs should be opened here!
    free(canon_str2);
    if(!f) { cerr << "Cannot open " << str2 << endl; return; }
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
