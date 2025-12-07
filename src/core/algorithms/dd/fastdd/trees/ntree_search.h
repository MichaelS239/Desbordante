#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <stack>
#include <unordered_map>
#include <utility>
#include <vector>

#include <boost/dynamic_bitset.hpp>

#include "core/model/types/bitset.h"

namespace algos::dd {

/**
 * A trie-like structure (prefix tree) for storing bitsets and checking
 * if any stored bitset is a subset of a given bitset.
 */
class NTreeSearch {
private:
    // Maps a bit-position to a child node
    std::vector<std::unique_ptr<NTreeSearch>> children_;
    model::Bitset<64> children_bitset_;

    // Optional to hold a terminal bitset at this node.
    // If present, it represents a complete bitset stored here.
    std::optional<model::Bitset<64>> stored_bitset_;

    static model::Bitset<64> ToStaticBitset(boost::dynamic_bitset<> const& bs) {
        model::Bitset<64> bitset;
        for (std::size_t index = bs.find_first(); index != boost::dynamic_bitset<>::npos;
             index = bs.find_next(index)) {
            bitset.set(index);
        }

        return bitset;
    }

    void InsertImpl(model::Bitset<64> const& bs, std::size_t cur_bit, std::size_t next_bit) {
        if (next_bit == 64) {
            stored_bitset_ = bs;
            return;
        }

        if (children_bitset_.none()) {
            if (!stored_bitset_) {
                stored_bitset_ = bs;
                return;
            } else {
                std::size_t stored_next_bit = cur_bit == 64 ? stored_bitset_->_Find_first()
                                                            : stored_bitset_->_Find_next(cur_bit);
                if (stored_next_bit != 64) {
                    children_[stored_next_bit] = std::make_unique<NTreeSearch>(stored_bitset_);
                    children_bitset_.set(stored_next_bit, true);
                    stored_bitset_.reset();
                }
            }
        }

        auto& child = children_[next_bit];
        if (!child) {
            child = std::make_unique<NTreeSearch>();
            children_bitset_.set(next_bit, true);
        }

        child->InsertImpl(bs, next_bit, bs._Find_next(next_bit));
    }

    bool FindSubset(/*boost::dynamic_bitset<>*/ model::Bitset<64> const& bs,
                    std::size_t next_bit) const {
        // If the current node stores a bitset, it is a subset by definition
        if (stored_bitset_) {
            return stored_bitset_ == (bs & stored_bitset_.value());
        }

        while (next_bit != 64) {
            std::size_t next_index = bs._Find_next(next_bit);
            if (children_bitset_[next_bit]) {
                if (children_[next_bit]->FindSubset(bs, next_index)) {
                    return true;
                }
            }
            next_bit = next_index;
        }

        return false;
    }

    bool GetAndRemoveGeneralizations(model::Bitset<64> const& bs, std::size_t next_bit,
                                     std::vector<model::Bitset<64>>& result) {
        if (stored_bitset_) {
            if (stored_bitset_ == (bs & stored_bitset_.value())) {
                result.push_back(stored_bitset_.value());
                stored_bitset_.reset();
            } else {
                return false;
            }
        }

        while (next_bit != 64) {
            std::size_t next_index = bs._Find_next(next_bit);
            if (children_bitset_[next_bit]) {
                if (children_[next_bit]->GetAndRemoveGeneralizations(bs, next_index, result)) {
                    children_[next_bit] = nullptr;
                    children_bitset_.set(next_bit, false);
                }
            }
            next_bit = next_index;
        }

        return children_bitset_.none();
    }

public:
    struct Iterator {
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = model::Bitset<64>;
        using pointer = value_type const*;
        using reference = value_type const&;

        Iterator(NTreeSearch* root, bool is_end = false) : traversal_() {
            if (!is_end && (!root->children_bitset_.none() || root->stored_bitset_)) {
                traversal_.emplace(root, root->children_bitset_._Find_first());
                FindNext();
            }
        }

        reference operator*() const {
            Node cur_node = traversal_.top();
            return cur_node.first->stored_bitset_.value();
        }

        Iterator& operator++() {
            Node cur_node = traversal_.top();
            auto cur_it_copy = cur_node.second;
            while (cur_node.second == 64 ||
                   cur_node.first->children_bitset_._Find_next(cur_it_copy) == 64) {
                cur_it_copy = cur_node.second;
                if (cur_it_copy != 64 &&
                    cur_node.first->children_bitset_._Find_next(cur_it_copy) == 64 &&
                    cur_node.first->stored_bitset_) {
                    traversal_.top().second =
                            cur_node.first->children_bitset_._Find_next(traversal_.top().second);
                    return *this;
                }
                traversal_.pop();
                if (traversal_.empty()) {
                    return *this;
                }
                cur_node = traversal_.top();
                cur_it_copy = cur_node.second;
            }

            traversal_.top().second =
                    traversal_.top().first->children_bitset_._Find_next(traversal_.top().second);
            FindNext();

            return *this;
        }

        Iterator operator++(int) {
            Iterator tmp = *this;
            ++(*this);

            return tmp;
        }

        friend bool operator==(Iterator const& a, Iterator const& b) {
            return a.traversal_ == b.traversal_;
        }

        friend bool operator!=(Iterator const& a, Iterator const& b) {
            return !(a == b);
        }

    private:
        using Node = std::pair<NTreeSearch*, std::size_t>;

        void FindNext() {
            while (!traversal_.empty()) {
                Node cur_node = traversal_.top();
                if (cur_node.first->children_bitset_.none()) {
                    return;
                }
                NTreeSearch* child = cur_node.first->children_[cur_node.second].get();
                traversal_.emplace(child, child->children_bitset_._Find_first());
            }
        }

        std::stack<Node> traversal_;
    };

    void Insert(boost::dynamic_bitset<> const& bs) {
        model::Bitset<64> static_bitset = ToStaticBitset(bs);
        InsertImpl(static_bitset, 64UL, static_bitset._Find_first());
    }

    [[nodiscard]]
    bool ContainsSubset(boost::dynamic_bitset<> const& bs) const {
        model::Bitset<64> static_bitset = ToStaticBitset(bs);
        return FindSubset(static_bitset, static_bitset._Find_first());
    }

    std::vector<model::Bitset<64>> GetAndRemoveGeneralizations(boost::dynamic_bitset<> const& bs) {
        std::vector<model::Bitset<64>> removed;
        model::Bitset<64> static_bitset = ToStaticBitset(bs);
        GetAndRemoveGeneralizations(static_bitset, static_bitset._Find_first(), removed);
        return removed;
    }

    Iterator begin() {
        return Iterator(this);
    }

    Iterator end() {
        return Iterator(this, true);
    }

    NTreeSearch() : children_(64UL), children_bitset_(), stored_bitset_() {}

    NTreeSearch(std::optional<model::Bitset<64>> const& bs)
        : children_(64UL), children_bitset_(), stored_bitset_(bs) {}
};

}  // namespace algos::dd
