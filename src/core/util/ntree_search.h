#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <stack>
#include <unordered_map>
#include <utility>
#include <vector>

#include <boost/dynamic_bitset.hpp>

namespace util {

/**
 * A trie-like structure (prefix tree) for storing bitsets and checking
 * if any stored bitset is a subset of a given bitset.
 */
class NTreeSearch {
private:
    // Maps a bit-position to a child node
    std::unordered_map<std::size_t, std::unique_ptr<NTreeSearch>> children_;

    // Optional to hold a terminal bitset at this node.
    // If present, it represents a complete bitset stored here.
    std::optional<boost::dynamic_bitset<>> stored_bitset_;

    void InsertImpl(boost::dynamic_bitset<> const& bs, std::size_t next_bit) {
        if (next_bit == boost::dynamic_bitset<>::npos) {
            stored_bitset_ = bs;
            return;
        }

        auto& child = children_[next_bit];
        if (!child) {
            child = std::make_unique<NTreeSearch>();
        }

        child->InsertImpl(bs, bs.find_next(next_bit));
    }

    bool FindSubset(boost::dynamic_bitset<> const& bs, std::size_t next_bit) const {
        // If the current node stores a bitset, it is a subset by definition
        if (stored_bitset_) {
            return true;
        }

        while (next_bit != boost::dynamic_bitset<>::npos) {
            if (auto it = children_.find(next_bit); it != children_.end()) {
                if (it->second->FindSubset(bs, bs.find_next(next_bit))) {
                    return true;
                }
            }
            next_bit = bs.find_next(next_bit);
        }

        return false;
    }

    bool GetAndRemoveGeneralizations(boost::dynamic_bitset<> const& bs, std::size_t next_bit,
                                     std::vector<boost::dynamic_bitset<>>& result) {
        if (stored_bitset_) {
            result.push_back(stored_bitset_.value());
            stored_bitset_.reset();
        }

        while (next_bit != boost::dynamic_bitset<>::npos) {
            if (auto it = children_.find(next_bit); it != children_.end()) {
                if (it->second->GetAndRemoveGeneralizations(bs, bs.find_next(next_bit), result)) {
                    children_.erase(next_bit);
                }
            }
            next_bit = bs.find_next(next_bit);
        }

        return !children_.empty();
    }

public:
    struct Iterator {
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = boost::dynamic_bitset<>;
        using pointer = value_type const*;
        using reference = value_type const&;

        Iterator(NTreeSearch* root, bool is_end = false) : traversal_() {
            if (!is_end) {
                traversal_.emplace(root, root->children_.begin());
                FindNext(/*{root, root->children_.begin()}*/);
            }
        }

        reference operator*() const {
            Node cur_node = traversal_.top();
            return cur_node.first->stored_bitset_.value();
        }

        Iterator& operator++() {
            Node cur_node = traversal_.top();
            while (cur_node.second == cur_node.first->children_.end()) {
                traversal_.pop();
                if (traversal_.empty()) {
                    break;
                }
                cur_node = traversal_.top();
            }
            if (!traversal_.empty()) {
                ++traversal_.top().second;
                FindNext();
            }

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
        using Node =
                std::pair<NTreeSearch*,
                          std::unordered_map<std::size_t, std::unique_ptr<NTreeSearch>>::iterator>;

        void FindNext(/*Node cur_node*/) {
            /*if (cur_node.first->stored_bitset_) {
                return true;
            }
            for (auto it = cur_node.second; it != cur_node.first->children_.end(); ++it) {
                traversal_.emplace(cur_node, it);
                NTreeSearch* child = it->second.get();
                if (FindNext({child, child->children_.begin()})) {
                    return true;
                }
                traversal_.pop();
            }*/

            while (!traversal_.empty()) {
                Node cur_node = traversal_.top();
                if (cur_node.first->stored_bitset_) {
                    return;
                }
                NTreeSearch* child = cur_node.second->second.get();
                traversal_.emplace(child, child->children_.begin());
            }
        }

        std::stack<Node> traversal_;
    };

    void Insert(boost::dynamic_bitset<> const& bs) {
        InsertImpl(bs, bs.find_first());
    }

    [[nodiscard]]
    bool ContainsSubset(boost::dynamic_bitset<> const& bs) const {
        return FindSubset(bs, bs.find_first());
    }

    std::vector<boost::dynamic_bitset<>> GetAndRemoveGeneralizations(
            boost::dynamic_bitset<> const& bs) {
        std::vector<boost::dynamic_bitset<>> removed;
        GetAndRemoveGeneralizations(bs, bs.find_first(), removed);
        return removed;
    }

    Iterator begin() {
        return Iterator(this);
    }

    Iterator end() {
        return Iterator(this, true);
    }
};

}  // namespace util
