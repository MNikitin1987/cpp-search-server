#include "remove_duplicates.h"
#include <iostream>
#include <map>
#include <string>
#include <set>

void RemoveDuplicates(SearchServer& search_server) {
    std::set<int> docs_to_delete;
    std::map<int, std::set<std::string>> docs_to_compare;
    
    for (auto i : search_server) {
        for (auto iter = search_server.GetWordFrequencies(i).begin(); iter != search_server.GetWordFrequencies(i).end(); ++iter) {
            auto [key, value] = *iter;
            docs_to_compare[i].insert(key);
        }
    }
    
    for (auto i : search_server) {
        for (auto j : search_server) {
            if (i == j) {
                continue;
            }
            if (docs_to_compare[i] == docs_to_compare[j]) {
                if (i > j) {
                    docs_to_delete.insert(i);
                } else {
                    docs_to_delete.insert(j);
                }
            }
        }
    }
    
    for (auto k : docs_to_delete) {
        search_server.RemoveDocument(k);
        std::cout << "Found duplicate document id " << k << std::endl;
    }
    
}