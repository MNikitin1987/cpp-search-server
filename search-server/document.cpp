#include "document.h"

Document::Document(int id, double relevance, int rating)
    : id(id)
    , relevance(relevance)
    , rating(rating) {
}

std::ostream& operator<< (std::ostream& output, const Document& value) {
    output << "{ document_id = " << value.id 
           << ", relevance = " << value.relevance 
           << ", rating = " << value.rating << " }";
    return output;
}