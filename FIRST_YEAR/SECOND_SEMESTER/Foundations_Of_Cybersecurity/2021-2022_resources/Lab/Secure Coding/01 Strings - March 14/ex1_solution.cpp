#include <cstring>
#include <string>
#include <iostream>
#include <limits>

using namespace std;

void do_stuff(const char* str, size_t offset){
// NOTE: For esoteric reasons, C++ strings cannot be used here, and some code lines are UNCHANGEABLE.
    // PART #1
    char result1[100]; // UNCHANGEABLE!
    strncpy(result1, str, 99);
    result1[99] = '\0';

      // PART #2
    char result2[50]; // UNCHANGEABLE!
    if (offset > 49) offset = 49;
    memset(result2, 'a', offset);
    printf("[do_stuff()] Please type the second part of result2: ");
    if(fgets(result2 + offset, 50 - offset, stdin) == NULL) return; // sure that: 50-offset>0 -> OK
    char* p;
    if(p=strchr(result2, '\n')){//check exist newline
        *p = '\0';
     } 
    else {
        scanf("%*[^\n]");//clear up to newline
        scanf("%*c");//clear newline
    }
    
    // PART #3
    char result3[60]; // UNCHANGEABLE!
    strcpy(result3, "abcdefghij");
    printf("[do_stuff()] Please type the second part of result3: ");
    scanf("%49s", result3 + 10); // for esoteric reasons, nothing except scanf() can be used here
    scanf("%*[^\n]");//clear up to newline
    scanf("%*c");//clear newline
   
   // PART #4
    char result4[40]; // UNCHANGEABLE!
    strcpy(result4, "ABCDEFGHIJ");
    sprintf(result4 + 10, "--%.25s--", str); // for esoteric reasons, nothing except sprintf() can be used here

    // PRINT RESULTS
    printf("result1=%s\nresult2=%s\nresult3=%s\nresult4=%s\n", result1, result2, result3, result4); // UNCHANGEABLE!
}

int main() {
    string str;
    size_t offset;

    cout << "Welcome to do_stuff() invoker." << endl;
    cout << endl;

    for(;;){
        cout << "str = ";
        cin >> str;
        if(!cin) return 0;
        cout << "offset = ";
        cin >> offset;
        if(!cin) return 0;
        cin.ignore(numeric_limits<streamsize>::max(), '\n'); // clear cin buffer
        cout << "Invoking do_stuff(" << str << "," << offset << ")..." << endl;
        do_stuff(str.c_str(), offset);
        cout << endl;
    }
    return 0;
}
