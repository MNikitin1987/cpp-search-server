#include "request_queue.h"

    RequestQueue::RequestQueue(const SearchServer& search_server) 
    :search_server_(search_server), request_no_result_(0), time_current_(0)
    {
    }

    std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentStatus status) {
        return AddFindRequest(
            raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
                return document_status == status;
            });;
    }

    std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query) {
        return AddFindRequest(raw_query, DocumentStatus::ACTUAL);
    }

    int RequestQueue::GetNoResultRequests() const {
        return request_no_result_;
    }

    void RequestQueue::AddRequest(bool result) {
        ++time_current_;
        while (!requests_.empty() && min_in_day_ <= time_current_ - requests_.front().time_of_request) {
            if (requests_.front().empty_result == true) {
                --request_no_result_;
            }
            requests_.pop_front();
        }
        requests_.push_back({time_current_, result});
        if (result == true) {
            ++request_no_result_;
        }
    }