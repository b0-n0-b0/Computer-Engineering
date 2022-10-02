#include <cstring>
#include <string>
#include <iostream>

using namespace std;

void do_stuff_ultimate(unsigned int num1, int num2, int num3, int num4, int num5){
// NOTE: For esoteric reasons, some code lines are UNCHANGEABLE.
    // PART #1
    int* result1 = (int*)malloc(num1*sizeof(int)); // UNCHANGEABLE!
    if(!result1) return;
    for(size_t i = 0; i < num1; i++)
        result1[i] = 45;

    // PART #2
    unsigned int one_hundred = 100U; // UNCHANGEABLE!
    int result2; // UNCHANGEABLE!
    do{
        cout << "[do_stuff_ultimate()] Please type the value of result2 (must be >100): ";
        cin >> result2;
    }
    while(result2 <= one_hundred);

    // PART #3
    char* result3 = (char*)malloc(num2*num3 + 1); // UNCHANGEABLE!
    if(!result3) return;
    memset(result3, 'a', num2*num3);
    for(size_t i = 0; i < num2; i++)
        result3[num3*i] = 'A';
    result3[num2*num3] = '\0';

    // PART #4
    int v[1000]; // UNCHANGEABLE!
    for(size_t i = 0; i < 1000; i++)
        v[i] = i;
    int result4 = v[400+num4*num5]; // UNCHANGEABLE!

    // PRINT RESULTS
    cout << "result1={";
    for(size_t i = 0; i < num1; i++)
        cout << result1[i] << ", ";
    cout << "}\n";
    cout << "result2=" << result2 << "\nresult3=" << result3 << "\nresult4=" << result4 << "\n";
    free(result1);
    free(result3);
}

int main() {
    unsigned int num1;
    int num2;
    int num3;
    int num4;
    int num5;

    cout << "Welcome to do_stuff_ultimate() invoker." << endl;
    cout << endl;

    for(;;){
        cout << "num1 = ";
        cin >> num1;
        if(!cin) return 0;
        cout << "num2 = ";
        cin >> num2;
        if(!cin) return 0;
        cout << "num3 = ";
        cin >> num3;
        if(!cin) return 0;
        cout << "num4 = ";
        cin >> num4;
        if(!cin) return 0;
        cout << "num5 = ";
        cin >> num5;
        if(!cin) return 0;
        //cin.ignore(numeric_limits<streamsize>::max(), '\n'); // clear cin buffer
        cout << "Invoking do_stuff_ultimate(" << num1 << "," << num2 << "," << num3 << "," << num4 << "," << num5 << ")..." << endl;
        do_stuff_ultimate(num1, num2, num3, num4, num5);
        cout << endl;
    }
    return 0;
}
