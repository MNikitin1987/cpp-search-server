#pragma once

#include <vector>
#include <string_view>
#include <string>
#include <set>

using namespace std;

vector<string_view> SplitIntoWords(string_view text);

template <typename StringContainer>
set<string, less<>> MakeUniqueNonEmptyStrings(const StringContainer& strings) {
    set<string, less<>> non_empty_strings;
    for (const string_view& str : strings) {
        if (!str.empty()) {
            non_empty_strings.emplace(str);
        }
    }
    return non_empty_strings;
}