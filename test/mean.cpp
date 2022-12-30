#include <iostream>
#include <vector>

using namespace std;

template <class T>
double mean(const std::vector<T>& v) {
    T acc;

    for (size_t i = 0; i <= v.size(); ++i) {
        acc += v[i];
        cout << "v["s << i << "] = "s << v[i] << endl;
        cout << "acc: "s << acc << endl;
    }

    return static_cast<double>(acc) / v.size();
}

int main() {
    vector<double> u = {1, 2, 3, 4, 5};
    cout << "Mean: "s << mean(u) << endl;
}