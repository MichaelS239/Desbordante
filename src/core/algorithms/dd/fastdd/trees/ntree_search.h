#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <stack>
#include <unordered_map>
#include <utility>
#include <vector>

#include <boost/dynamic_bitset.hpp>

#include "core/util/dynamic_bitset.h"

namespace algos::dd {

/**
 * A trie-like structure (prefix tree) for storing bitsets and checking
 * if any stored bitset is a subset of a given bitset.
 */
class NTreeSearch {
private:
    // Maps a bit-position to a child node
    std::vector<std::unique_ptr<NTreeSearch>> children_;
    util::DynamicBitset children_bitset_;

    // Optional to hold a terminal bitset at this node.
    // If present, it represents a complete bitset stored here.
    std::optional<util::DynamicBitset> stored_bitset_;

    void InsertImpl(util::DynamicBitset const& bs, std::size_t cur_bit, std::size_t next_bit) {
        if (next_bit == util::DynamicBitset::npos) {
            stored_bitset_ = bs;
            return;
        }

        if (children_bitset_.none()) {
            if (!stored_bitset_) {
                stored_bitset_ = bs;
                return;
            } else {
                std::size_t stored_next_bit = cur_bit == util::DynamicBitset::npos
                                                      ? stored_bitset_->FindFirst()
                                                      : stored_bitset_->FindNext(cur_bit);
                if (stored_next_bit != util::DynamicBitset::npos) {
                    children_[stored_next_bit] =
                            std::make_unique<NTreeSearch>(stored_bitset_->size(), stored_bitset_);
                    children_bitset_.set(stored_next_bit, true);
                    stored_bitset_.reset();
                }
            }
        }

        auto& child = children_[next_bit];
        if (!child) {
            child = std::make_unique<NTreeSearch>(children_bitset_.size());
            children_bitset_.set(next_bit, true);
        }

        child->InsertImpl(bs, next_bit, bs.FindNext(next_bit));
    }

    bool FindSubset(util::DynamicBitset const& bs, std::size_t next_bit) const {
        // If the current node stores a bitset, it is a subset by definition
        if (stored_bitset_) {
            return stored_bitset_ == (bs & stored_bitset_.value());
        }

        while (next_bit != util::DynamicBitset::npos) {
            std::size_t next_index = bs.FindNext(next_bit);
            if (children_bitset_[next_bit]) {
                if (children_[next_bit]->FindSubset(bs, next_index)) {
                    return true;
                }
            }
            next_bit = next_index;
        }

        return false;
    }

    bool GetAndRemoveGeneralizations(util::DynamicBitset const& bs, std::size_t next_bit,
                                     std::vector<util::DynamicBitset>& result) {
        if (stored_bitset_) {
            if (stored_bitset_ == (bs & stored_bitset_.value())) {
                result.push_back(stored_bitset_.value());
                stored_bitset_.reset();
            } else {
                return false;
            }
        }

        while (next_bit != util::DynamicBitset::npos) {
            std::size_t next_index = bs.FindNext(next_bit);
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
        using value_type = util::DynamicBitset;
        using pointer = std::optional<util::DynamicBitset>;
        using reference = value_type const&;

        Iterator(NTreeSearch* root, bool is_end = false) : traversal_() {
            if (!is_end && (!root->children_bitset_.none() || root->stored_bitset_)) {
                traversal_.emplace(root, root->children_bitset_.FindFirst());
                FindNext();
            }
        }

        reference operator*() const {
            Node cur_node = traversal_.top();
            return cur_node.first->stored_bitset_.value();
        }

        pointer operator->() const {
            Node cur_node = traversal_.top();
            return cur_node.first->stored_bitset_;
        }

        Iterator& operator++() {
            Node cur_node = traversal_.top();
            auto cur_it_copy = cur_node.second;
            while (cur_node.second == util::DynamicBitset::npos ||
                   cur_node.first->children_bitset_.FindNext(cur_it_copy) ==
                           util::DynamicBitset::npos) {
                cur_it_copy = cur_node.second;
                if (cur_it_copy != util::DynamicBitset::npos &&
                    cur_node.first->children_bitset_.FindNext(cur_it_copy) ==
                            util::DynamicBitset::npos &&
                    cur_node.first->stored_bitset_) {
                    traversal_.top().second =
                            cur_node.first->children_bitset_.FindNext(traversal_.top().second);
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
                    traversal_.top().first->children_bitset_.FindNext(traversal_.top().second);
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
                traversal_.emplace(child, child->children_bitset_.FindFirst());
            }
        }

        std::stack<Node> traversal_;
    };

    void Insert(boost::dynamic_bitset<> const& bs) {
        util::DynamicBitset bitset(bs);
        InsertImpl(bitset, util::DynamicBitset::npos, bitset.FindFirst());
    }

    [[nodiscard]]
    bool ContainsSubset(boost::dynamic_bitset<> const& bs) const {
        util::DynamicBitset bitset(bs);
        return FindSubset(bitset, bitset.FindFirst());
    }

    std::vector<util::DynamicBitset> GetAndRemoveGeneralizations(
            boost::dynamic_bitset<> const& bs) {
        std::vector<util::DynamicBitset> removed;
        util::DynamicBitset bitset(bs);
        GetAndRemoveGeneralizations(bitset, bitset.FindFirst(), removed);
        return removed;
    }

    Iterator begin() {
        return Iterator(this);
    }

    Iterator end() {
        return Iterator(this, true);
    }

    explicit NTreeSearch(std::size_t bitset_size = 64UL)
        : children_(bitset_size), children_bitset_(bitset_size), stored_bitset_() {}

    NTreeSearch(std::size_t bitset_size, std::optional<util::DynamicBitset> const& bs)
        : children_(bitset_size), children_bitset_(bitset_size), stored_bitset_(bs) {}
};

}  // namespace algos::dd
