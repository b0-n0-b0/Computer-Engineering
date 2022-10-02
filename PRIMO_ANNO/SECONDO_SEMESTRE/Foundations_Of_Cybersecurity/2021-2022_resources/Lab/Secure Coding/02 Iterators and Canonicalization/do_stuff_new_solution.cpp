#include <vector>
#include <iostream>
#include <algorithm>

using namespace std;

void do_stuff_new(const vector<int>& vec){
// NOTE: For esoteric reasons, C++ vectors must be used here, and some code lines are UNCHANGEABLE.
    // PART #1
    int* result1 = (int*)malloc(vec.size()*sizeof(int)); // ignoring here a possible integer wrap bug...
    if(!result1) return;
    copy(vec.begin(), vec.end(), result1);

    // PART #2
    vector<int> result2(vec);

    // PART #3
    vector<int> result3;
    auto tmp1 = (vec.size() < 5) ? vec.end() : (vec.begin() + 5);
    for(auto i = tmp1; i != vec.end(); i++) // UNCHANGEABLE!
        result3.push_back(*i + 10); // UNCHANGEABLE!

    // PART #4
    vector<int> result4(vec); // UNCHANGEABLE!
    size_t tmp_size = result4.size();
    auto tmp2 = (tmp_size < 2) ? result4.end() : (result4.end() - 2);
    for(auto i = result4.begin(); i != tmp2; i++){ // UNCHANGEABLE!
        if(*i % 2 == 0){
            i = result4.insert(i, *i + 20);
            i++;
            tmp2 = (tmp_size < 2) ? result4.end() : (result4.end() - 2);
        }
    }

    // PRINT RESULTS
    // all the following lines are UNCHANGEABLE
    cout << "result1={";
    for(size_t k = 0; k < vec.size(); k++)
        cout << result1[k] << ", ";
    cout << "}\n";
    cout << "result2={";
    for(auto k = result2.begin(); k != result2.end(); k++)
        cout << *k << ", ";
    cout << "}\n";
    cout << "result3={";
    for(auto k = result3.begin(); k != result3.end(); k++)
        cout << *k << ", ";
    cout << "}\n";
    cout << "result4={";
    for(auto k = result4.begin(); k != result4.end(); k++)
        cout << *k << ", ";
    cout << "}\n";
    free(result1);
}

int main() {
    vector<int> vec;

    cout << "Welcome to do_stuff_new() invoker." << endl;
    cout << endl;

    for(;;){
        cout << "(Insert non-number to stop)" << endl;
        vec.clear();
        do{
            int tmp;
            cout << "vec[" << vec.size() << "] = ";
            cin >> tmp;
            if(cin) vec.push_back(tmp);
        }
        while(cin);
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n'); // clear cin buffer
        cout << "Invoking do_stuff_new(vec)..." << endl;
        do_stuff_new(vec);
        cout << endl;
    }
    return 0;
}
