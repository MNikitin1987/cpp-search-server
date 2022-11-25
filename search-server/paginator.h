#pragma once
#include <iostream>
#include <vector>

template <typename Iterator>
class IteratorRange {
public:
    IteratorRange(Iterator Begin, Iterator End, size_t Size) {
        begin_ = Begin;
        end_ = End;
        size_ = Size;
    }
    auto begin() {
        return begin_;
    }
    auto end() {
        return end_;
    }
    auto size() {
        return size_;
    }

private:
    Iterator begin_;
    Iterator end_;
    size_t size_;
};

template <typename Iterator>
std::ostream& std::operator<< (ostream& output, IteratorRange<Iterator> values) {
    for (Iterator value = values.begin(); value < values.end(); advance(value, 1)) {
        output << *value;
    }
    return output;
}

template <typename Iterator>
class Paginator {
        // тело класса
public:
    Paginator(Iterator BeginIt, Iterator EndIt, size_t page_size) {
        for (auto CurrentIt = BeginIt; CurrentIt < EndIt; advance(CurrentIt, page_size)) {
            if (distance(CurrentIt, EndIt) >= page_size) {
                auto temp = IteratorRange(CurrentIt, CurrentIt + page_size, page_size);
                pages_.push_back(temp);
            } else {
                if (distance(CurrentIt, EndIt) > 0) {
                    auto temp = IteratorRange(CurrentIt, CurrentIt + distance(CurrentIt, EndIt), distance(CurrentIt, EndIt));
                    pages_.push_back(temp);
                }
            }
        }
    }
    auto begin() const {
        return pages_.begin();
    }
    auto end() const {
        return pages_.end();
    }
    auto size() const {
        return pages_.size();
    }
    private:
        std::vector<IteratorRange<Iterator>> pages_;
};

template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}