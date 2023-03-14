#include "process_queries.h"

using namespace std;

std::vector<std::vector<Document>> ProcessQueries(
    const SearchServer& search_server,
    const std::vector<std::string>& queries) {
    
    std::vector<std::vector<Document>> res(queries.size());
    
    std::transform(
        execution::par,
        queries.begin(), queries.end(),
        res.begin(),
        [&](const string& query) {
            return search_server.FindTopDocuments(query);
        }
    );
    return res;
}

std::vector<Document> ProcessQueriesJoined(
    const SearchServer& search_server,
    const std::vector<std::string>& queries) {
    
    auto intermediate_result = ProcessQueries(search_server, queries);
    size_t final_size = 0;
    for (const auto& documents : intermediate_result) {
        final_size += documents.size();
    }
    
    std::vector<Document> res;
    res.reserve(final_size);
    
    for (const auto& documents : intermediate_result) {
        res.insert(res.end(), documents.begin(), documents.end());
    }
    return res;
}