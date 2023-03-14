#include "string_processing.h"

vector<string_view> SplitIntoWords(string_view text) {

    vector<string_view> words;

    while (true) {
        const auto pos = text.find(' ');

        if (pos != 0) {
            words.push_back(text.substr(0, pos));
        }

        if (pos == text.npos) {
            break;
        }
        else {
            text.remove_prefix(pos + 1);
        }
    }
    return words;
}