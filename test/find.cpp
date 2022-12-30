#include <algorithm>
#include <iostream>
#include <random>
#include <vector>

using namespace std;

int main() {
    vector v = {1, 81, 100, 50, 70, 30, 90, 200};
    auto iter = upper_bound(v.begin(), v.end(), 80);
    if (iter != v.end()) {
        cout << "Found element "s << *iter << endl;
    }
} 