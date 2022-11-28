#include "remove_duplicates.h"
#include <iostream>
#include <map>
#include <string>
#include <set>

void RemoveDuplicates(SearchServer& search_server) {
    std::set<int> docs_to_delete;
    std::map<std::set<std::string>, int> docs_to_compare;
    
    for (auto i : search_server) {
        std::set<std::string> words;
        for (auto j : search_server.GetWordFrequencies(i)) {
            words.insert(j.first); 
        }
        if (docs_to_compare.find(words) == docs_to_compare.end()) {
            docs_to_compare[words];
        } else {
            docs_to_delete.insert(i);
        }
    }
    
    for (auto k : docs_to_delete) {
        search_server.RemoveDocument(k);
        std::cout << "Found duplicate document id " << k << std::endl;
    }
    
}