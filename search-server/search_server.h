#pragma once 

#include <string>
#include <string_view>
#include <vector>
#include <stdexcept>
#include <map>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <iterator>
#include <execution>
#include <deque>
#include <future>

#include "string_processing.h"
#include "document.h"
#include "concurrent_map.h"

using namespace std;

constexpr int MAX_RESULT_DOCUMENT_COUNT = 5;
constexpr double DOUBLE_EPSILON = 1e-6;
constexpr size_t PACKS_NUM = 120;

class SearchServer {
public:
    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words)
        : stop_words_(MakeUniqueNonEmptyStrings(stop_words))
    {
        if (!all_of(stop_words_.begin(), stop_words_.end(), IsValidWord)) {
            throw invalid_argument("Some of stop words are invalid"s);
        }
    }

    explicit SearchServer(const string& stop_words_text);
    explicit SearchServer(string_view& stop_words_text);

    void AddDocument(int document_id, const string_view& document, DocumentStatus status, const vector<int>& ratings);


    template <typename DocumentPredicate>
    vector<Document> FindTopDocuments(const string_view& raw_query, DocumentPredicate document_predicate) const {
        return FindTopDocuments(execution::seq, raw_query, document_predicate);
    }

    template <typename DocumentPredicate, typename ExecutionPolicy>
    vector<Document> FindTopDocuments(const ExecutionPolicy& policy,
        const string_view& raw_query, DocumentPredicate document_predicate) const {
        const auto query = ParseQuery(raw_query, true);
        auto matched_documents = FindAllDocuments(policy, query, document_predicate);
        sort(policy,
            matched_documents.begin(), matched_documents.end(),
            [](const Document& lhs, const Document& rhs) {
                return lhs.relevance > rhs.relevance
                    || (abs(lhs.relevance - rhs.relevance) < DOUBLE_EPSILON && lhs.rating > rhs.rating);
            });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

    vector<Document> FindTopDocuments(
        const string_view& raw_query, DocumentStatus status) const;

    vector<Document> FindTopDocuments(const execution::parallel_policy&,
        const string_view& raw_query, DocumentStatus status) const;

    vector<Document> FindTopDocuments(const execution::sequenced_policy&,
        const string_view& raw_query, DocumentStatus status) const;

    vector<Document> FindTopDocuments(
        const string_view& raw_query) const;

    vector<Document> FindTopDocuments(const execution::parallel_policy&,
        const string_view& raw_query) const;

    vector<Document> FindTopDocuments(const execution::sequenced_policy&,
        const string_view& raw_query) const;

    int GetDocumentCount() const;
    set<int>::const_iterator begin() const;
    set<int>::const_iterator end() const;
    const map<string_view, double>& GetWordFrequencies(int document_id) const;

    void RemoveDocument(int document_id);
    void RemoveDocument(const execution::sequenced_policy&, int document_id);
    void RemoveDocument(const execution::parallel_policy&, int document_id);

    tuple<vector<string_view>, DocumentStatus> MatchDocument(const string_view& raw_query, int document_id) const;
    tuple<vector<string_view>, DocumentStatus> MatchDocument(const execution::sequenced_policy&, const string_view& raw_query, int document_id) const;
    tuple<vector<string_view>, DocumentStatus> MatchDocument(const execution::parallel_policy&, const string_view& raw_query, int document_id) const;

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    struct QueryWord {
        string_view data;
        bool is_minus;
        bool is_stop;
    };

    struct Query {
        vector<string_view> plus_words;
        vector<string_view> minus_words;
    };


private:
    deque<string> all_words_;
    const set<string, less<>> stop_words_;
    map<string_view, map<int, double>> word_to_document_freqs_;
    map<int, map<string_view, double>> document_to_word_freqs_;
    map<int, DocumentData> documents_;
    set<int> document_ids_;


private:
    bool IsStopWord(const string_view& word) const;
    static bool IsValidWord(const string_view& word);
    vector<string_view> SplitIntoWordsNoStop(const string_view& text) const;
    static int ComputeAverageRating(const vector<int>& ratings);
    QueryWord ParseQueryWord(string_view text) const;
    Query ParseQuery(const string_view& text, bool purge) const;
    double ComputeWordInverseDocumentFreq(const string_view& word) const;

    template <typename DocumentPredicate>
    vector<Document> FindAllDocuments(const execution::sequenced_policy&,
        const Query& query,
        DocumentPredicate document_predicate) const {
        map<int, double> document_to_relevance;

        for (const string_view& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                const auto& document_data = documents_.at(document_id);
                if (document_predicate(document_id, document_data.status, document_data.rating)) {
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        }

        for (const string_view& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }

            for (const auto& [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }

        vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back(
                { document_id, relevance, documents_.at(document_id).rating });
        }
        return matched_documents;
    }


    template <typename DocumentPredicate>
    vector<Document> FindAllDocuments(const execution::parallel_policy&,
        const Query& query,
        DocumentPredicate document_predicate) const {

        ConcurrentMap<int, double> document_to_relevance_concurrent(PACKS_NUM);


        for_each(
            execution::par,
            query.plus_words.begin(),
            query.plus_words.end(),
            [&](string_view word) {
                if (word_to_document_freqs_.count(word) == 0) {
                    return;
                }

                const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
                for_each(
                    execution::par,
                    word_to_document_freqs_.at(word).begin(),
                    word_to_document_freqs_.at(word).end(),
                    [&](auto& elem) {
                        auto& document_id = elem.first;
                        auto& term_freq = elem.second;
                        const auto& document_data = documents_.at(document_id);
                        if (document_predicate(document_id, document_data.status, document_data.rating)) {
                            document_to_relevance_concurrent[document_id].ref_to_value += term_freq * inverse_document_freq;
                        }
                    });
            });



        for (const string_view& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }

            for_each(
                execution::par,
                word_to_document_freqs_.at(word).begin(),
                word_to_document_freqs_.at(word).end(),
                [&](const auto& elem) {
                    const auto& document_id = elem.first;
                    document_to_relevance_concurrent.erase(document_id);
                });
        }

        const auto document_to_relevance = move(document_to_relevance_concurrent.BuildOrdinaryMap());

        vector<Document> matched_documents(document_to_relevance.size());
        transform(
            execution::par,
            document_to_relevance.begin(),
            document_to_relevance.end(),
            matched_documents.begin(),
            [&](const auto& elem) {
                const auto& document_id = elem.first;
                const auto& relevance = elem.second;
                return Document(document_id, relevance, documents_.at(document_id).rating);
            });

        return matched_documents;
    }
};