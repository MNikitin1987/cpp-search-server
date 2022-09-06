// Решите загадку: Сколько чисел от 1 до 1000 содержат как минимум одну цифру 3?
// Напишите ответ здесь:

#include <iostream>
#include <string>

using namespace std;

int main() {
    int count = 0;
    bool flag = false;
    string str;
    
    for (int i = 1; i <= 1000; ++i) {
        str = to_string (i);
        for (char c : str) if (c == '3') flag = true;
        if (flag == 1) {
            count ++;
            flag = false;
        }      
    }
    
    cout << count << endl;
}

// Закомитьте изменения и отправьте их в свой репозиторий.
