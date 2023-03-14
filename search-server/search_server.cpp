#include "search_server.h"

SearchServer::SearchServer(const string& stop_words)
: SearchServer(SplitIntoWords(string_view(stop_words))) {

}

SearchServer::SearchServer(string_view& stop_words)
    : SearchServer(SplitIntoWords(stop_words)) {

}

void SearchServer::AddDocument(int document_id, const string_view& document,
    DocumentStatus status, const vector<int>& ratings) {
    if ((document_id < 0) || (documents_.count(document_id) > 0)) {
        throw invalid_argument("Invalid document_id"s);
    }

    all_words_.emplace_back(document);

    const auto words = SplitIntoWordsNoStop(all_words_.back());

    const double inv_word_count = 1.0 / words.size();
    for (const string_view& word : words) {
        word_to_document_freqs_[word][document_id] += inv_word_count;
        document_to_word_freqs_[document_id][word] += inv_word_count;
    }
    documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
    document_ids_.insert(document_id);
}

vector<Document> SearchServer::FindTopDocuments(
    const string_view& raw_query, DocumentStatus status) const {
    return FindTopDocuments(execution::seq,
        raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
            return document_status == status;
        });
}

vector<Document> SearchServer::FindTopDocuments(const execution::parallel_policy&,
    const string_view& raw_query, DocumentStatus status) const {
    return FindTopDocuments(execution::par,
        raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
            return document_status == status;
        });
}

vector<Document> SearchServer::FindTopDocuments(const execution::sequenced_policy&,
    const string_view& raw_query, DocumentStatus status) const {
    return FindTopDocuments(execution::seq,
        raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
            return document_status == status;
        });
}

vector<Document> SearchServer::FindTopDocuments(
    const string_view& raw_query) const {
    return FindTopDocuments(execution::seq, raw_query, DocumentStatus::ACTUAL);
}

vector<Document> SearchServer::FindTopDocuments(const execution::parallel_policy&,
    const string_view& raw_query) const {
    return FindTopDocuments(execution::par, raw_query, DocumentStatus::ACTUAL);
}

vector<Document> SearchServer::FindTopDocuments(const execution::sequenced_policy&,
    const string_view& raw_query) const {
    return FindTopDocuments(execution::seq, raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const {
    return static_cast<int>(documents_.size());
}

set<int>::const_iterator SearchServer::begin() const {
    return document_ids_.begin();
}

set<int>::const_iterator SearchServer::end() const {
    return document_ids_.end();
}

const map<string_view, double>& SearchServer::GetWordFrequencies(int document_id) const {
    static const map<string_view, double> empty_map;
    
    if (document_to_word_freqs_.count(document_id) == 0) {
        return empty_map;
    }

    return document_to_word_freqs_.at(document_id);
}

void SearchServer::RemoveDocument(int document_id) {
    if (document_to_word_freqs_.count(document_id) == 0) {
        return;
    }

    for (auto& [word, _] : document_to_word_freqs_.at(document_id)) {
        word_to_document_freqs_.at(word).erase(document_id);
        if (word_to_document_freqs_.at(word).size() == 0) {
            word_to_document_freqs_.erase(word);
        }
    }
    
    document_to_word_freqs_.erase(document_id);
    documents_.erase(document_id);

    document_ids_.erase(document_id);
}

void SearchServer::RemoveDocument(const execution::sequenced_policy&, int document_id){
    SearchServer::RemoveDocument(document_id);
}

void SearchServer::RemoveDocument(const execution::parallel_policy&, int document_id) {
    if (document_to_word_freqs_.count(document_id) == 0) {
        return;
    }

    std::vector<std::string_view> words(document_to_word_freqs_.at(document_id).size());

    std::transform(
        std::execution::par,
        document_to_word_freqs_.at(document_id).begin(),
        document_to_word_freqs_.at(document_id).end(),
        words.begin(),
        [&](const auto& word) {
            return string_view(word.first);
        }
    );

    std::for_each(
        std::execution::par,
        words.begin(), words.end(),
        [&](const auto& word) {
            word_to_document_freqs_.at(static_cast<std::string>(word)).erase(document_id);
        }
    );

    document_to_word_freqs_.erase(document_id);

    documents_.erase(document_id);
    document_ids_.erase(document_id);
}

tuple<vector<string_view>, DocumentStatus> SearchServer::MatchDocument(const string_view& raw_query, int document_id) const {
    
    const Query query = ParseQuery(raw_query, true);

    vector<string_view> matched_words;

    for (const string_view& word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.clear();
            return { matched_words, documents_.at(document_id).status };
        }
    }

    for (const string_view& word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.push_back(word);
        }
    }

    return { matched_words, documents_.at(document_id).status };
}

tuple<vector<string_view>, DocumentStatus> SearchServer::MatchDocument(const execution::sequenced_policy&, const string_view& raw_query, int document_id) const {
    return MatchDocument(raw_query, document_id);
}

tuple<vector<string_view>, DocumentStatus> SearchServer::MatchDocument(const execution::parallel_policy&, const string_view& raw_query, int document_id) const {
    const auto query = ParseQuery(raw_query, false);

    bool minus_detected =
        std::any_of(
            execution::par,
            query.minus_words.begin(),
            query.minus_words.end(),
            [&](const auto word) {
                return
                    word_to_document_freqs_.count(word) != 0 &&
                    word_to_document_freqs_.at(word).count(document_id) != 0;
            });

    if (minus_detected) {
        return { {}, documents_.at(document_id).status };
    }

    vector<string_view> matched_words(query.plus_words.size());

    std::copy_if(
        execution::par,
        query.plus_words.begin(),
        query.plus_words.end(),
        matched_words.begin(),
        [&](const auto& word) {
            return
                word_to_document_freqs_.count(word) != 0 &&
                word_to_document_freqs_.at(word).count(document_id) != 0;
        });

    std::sort(
        execution::par,
        matched_words.begin(),
        matched_words.end());

    matched_words.erase(
        std::unique(matched_words.begin(), matched_words.end()),
        matched_words.end());

    if (*matched_words.begin() == ""s) {
        matched_words.erase(matched_words.begin());
    }
    return { matched_words, documents_.at(document_id).status };
}

bool SearchServer::IsStopWord(const string_view& word) const {
    return stop_words_.count(word) > 0;
}

bool SearchServer::IsValidWord(const string_view& word) {
    return none_of(word.begin(), word.end(),
        [](char c) {
            return c >= '\0' && c < ' ';
        });
}

vector<string_view> SearchServer::SplitIntoWordsNoStop(const string_view& text) const {
    vector<string_view> words;
    for (const string_view& word : SplitIntoWords(text)) {
        if (!IsValidWord(word)) {
            throw invalid_argument("Word is invalid"s);
        }
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const vector<int>& ratings) {
    return accumulate(ratings.begin(), ratings.end(), 0) / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(string_view word) const {
    if (word.empty()) {
        throw invalid_argument("Query word is empty"s);
    }

    bool is_minus = false;
    if (word[0] == '-') {
        is_minus = true;
        word.remove_prefix(1);
    }

    if (word.empty() || word[0] == '-' || !IsValidWord(word)) {
        throw invalid_argument("Query word "s + string{ word } + " is invalid");
    }
    return { word, is_minus, IsStopWord(word) };
}

SearchServer::Query SearchServer::ParseQuery(const string_view& text, bool purge) const {
    Query result;

    vector<string_view> words = SplitIntoWords(text);

    for (const string_view& word : words) {
        const auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {

            if (query_word.is_minus) {
                result.minus_words.push_back(query_word.data);
            }
            else {
                result.plus_words.push_back(query_word.data);
            }
        }
    }

    if (purge) {
        std::sort(result.plus_words.begin(), result.plus_words.end());
        std::sort(result.minus_words.begin(), result.minus_words.end());

        result.minus_words.erase(
            std::unique(result.minus_words.begin(), result.minus_words.end()),
            result.minus_words.end());

        result.plus_words.erase(
            std::unique(result.plus_words.begin(), result.plus_words.end()),
            result.plus_words.end());
    }

    return result;
}

double SearchServer::ComputeWordInverseDocumentFreq(const string_view& word) const {
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}
