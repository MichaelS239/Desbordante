#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <stack>
#include <unordered_map>
#include <utility>
#include <vector>

#include <boost/dynamic_bitset.hpp>
#include <boost/unordered/unordered_flat_map.hpp>
#include <easylogging++.h>

namespace util {

/**
 * A trie-like structure (prefix tree) for storing bitsets and checking
 * if any stored bitset is a subset of a given bitset.
 */
class NTreeSearch {
    /*public:
        inline static std::size_t count;*/

private:
    // Maps a bit-position to a child node
    // std::unordered_map<std::size_t, std::unique_ptr<NTreeSearch>> children_;
    boost::unordered::unordered_flat_map<std::size_t, std::unique_ptr<NTreeSearch>> children_;
    // std::vector<std::unique_ptr<NTreeSearch>> children_;
    // boost::dynamic_bitset<> children_bitset_;

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
            // children_bitset_.set(next_bit, true);
        }

        child->InsertImpl(bs, bs.find_next(next_bit));
    }

    bool FindSubset(boost::dynamic_bitset<> const& bs, std::size_t next_bit) const {
        //++util::NTreeSearch::count;
        // If the current node stores a bitset, it is a subset by definition
        if (stored_bitset_) {
            return true;
        }

        /*for (std::size_t index = next_bit; index != boost::dynamic_bitset<>::npos;
             index = bs.find_next(index)) {
            auto it = children_.find(index);
            if (it != children_.end() && it->second->FindSubset(bs, bs.find_next(index))) {
                return true;
            }
        }*/

        /*for (std::size_t index = next_bit; index != boost::dynamic_bitset<>::npos;
             index = bs.find_next(index)) {
            if (auto it = children_.find(index); it != children_.end()) {
                if (it->second->FindSubset(bs, bs.find_next(index))) {
                    return true;
                }
            }
        }*/
        auto children_end = children_.end();
        while (next_bit != boost::dynamic_bitset<>::npos) {
            //++util::NTreeSearch::count;
            std::size_t next_index = bs.find_next(next_bit);
            if (auto it = children_.find(next_bit); it != children_end) {
                if (it->second->FindSubset(bs, next_index)) {
                    return true;
                }
            }
            /*if (children_bitset_[next_bit]) {
                if (children_.at(next_bit)->FindSubset(bs, next_index)) {
                    return true;
                }
            }*/
            next_bit = next_index;
        }

        return false;
    }

    bool GetAndRemoveGeneralizations(boost::dynamic_bitset<> const& bs, std::size_t next_bit,
                                     std::vector<boost::dynamic_bitset<>>& result) {
        if (stored_bitset_) {
            result.push_back(stored_bitset_.value());
            stored_bitset_.reset();
        }

        /*for (std::size_t index = next_bit; index != boost::dynamic_bitset<>::npos;
             index = bs.find_next(index)) {
            if (auto it = children_.find(index); it != children_.end()) {
                if (it->second->GetAndRemoveGeneralizations(bs, bs.find_next(index), result)) {
                    children_.erase(index);
                }
            }
        }*/
        auto children_end = children_.end();
        while (next_bit != boost::dynamic_bitset<>::npos) {
            std::size_t next_index = bs.find_next(next_bit);
            if (auto it = children_.find(next_bit); it != children_.end()) {
                if (it->second->GetAndRemoveGeneralizations(bs, bs.find_next(next_bit), result)) {
                    children_.erase(next_bit);
                }
            }
            /*if (children_bitset_[next_bit]) {
                if (children_[next_bit]->GetAndRemoveGeneralizations(bs, next_index, result)) {
                    children_[next_bit] = nullptr;
                    children_bitset_.set(next_bit, false);
                }
            }*/
            next_bit = next_index;
        }

        return children_.empty();
        // return children_bitset_.none();
    }

public:
    struct Iterator {
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = boost::dynamic_bitset<>;
        using pointer = value_type const*;
        using reference = value_type const&;

        Iterator(NTreeSearch* root, bool is_end = false) : traversal_() {
            if (!is_end && (!root->children_.empty() /*!root->children_bitset_.none()*/ ||
                            root->stored_bitset_)) {
                traversal_.emplace(root, root->children_.cbegin());
                // traversal_.emplace(root, root->children_bitset_.find_first());
                FindNext(/*{root, root->children_.begin()}*/);
            }
        }

        reference operator*() const {
            Node cur_node = traversal_.top();
            // LOG(INFO) << "Retrieve: " << cur_node.first->stored_bitset_.value();
            return cur_node.first->stored_bitset_.value();
        }

        Iterator& operator++() {
            Node cur_node = traversal_.top();
            auto cur_it_copy = cur_node.second;
            while (cur_node.second == cur_node.first->children_.cend() /*cur_node.second ==
                           boost::dynamic_bitset<>::npos*/ ||
                   ++cur_it_copy == cur_node.first->children_.cend() /*cur_node.first
                                   ->children_bitset_.find_next(cur_it_copy) ==
                           boost::dynamic_bitset<>::npos*/) {
                cur_it_copy = cur_node.second;
                // LOG(INFO) << "...";
                if (cur_it_copy != cur_node.first->children_.cend() /*cur_it_copy !=
                            boost::dynamic_bitset<>::npos*/
                    && ++cur_it_copy == cur_node.first->children_.cend()
                    /*cur_node.first->children_bitset_.find_next(cur_it_copy) ==
                            boost::dynamic_bitset<>::npos*/
                    && cur_node.first->stored_bitset_) {
                    ++traversal_.top().second;
                    /*traversal_.top().second =
                            cur_node.first->children_bitset_.find_next(traversal_.top().second);*/
                    return *this;
                }
                traversal_.pop();
                if (traversal_.empty()) {
                    return *this;
                }
                cur_node = traversal_.top();
                cur_it_copy = cur_node.second;
            }

            ++traversal_.top().second;
            /*traversal_.top().second =
                    traversal_.top().first->children_bitset_.find_next(traversal_.top().second);*/
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
        using Node = std::pair<
                NTreeSearch*, /*std::size_t*/
                boost::unordered::unordered_flat_map<std::size_t,
                                                     std::unique_ptr<NTreeSearch>>::const_iterator
                /*std::unordered_map<std::size_t, std::unique_ptr<NTreeSearch>>::const_iterator*/>;

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
            // LOG(INFO) << "FindNext";

            while (!traversal_.empty()) {
                // LOG(INFO) << "Go";
                Node cur_node = traversal_.top();
                if (cur_node.first->children_.empty() /*cur_node.first->children_bitset_.none()*/) {
                    // LOG(INFO) << "Found";
                    return;
                }
                // LOG(INFO) << "False";
                NTreeSearch* child = (cur_node.second)->second.get();
                // NTreeSearch* child = cur_node.first->children_[cur_node.second].get();
                //  LOG(INFO) << "Child";
                /*if (child->children_.empty()) {
                    LOG(INFO) << "EMPTY!!!!!!!!";
                }*/
                /*if (child->children_bitset_.none()) {
                    LOG(INFO) << "EMPTY!!!!!!!!";
                }
                if (child->stored_bitset_) {
                    LOG(INFO) << "STORED BITSET!!!!!!";
                }*/
                traversal_.emplace(child, child->children_.cbegin());
                // traversal_.emplace(child, child->children_bitset_.find_first());
                //  LOG(INFO) << "Emplace";
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

    NTreeSearch() : children_(), /*children_bitset_(64UL, false),*/ stored_bitset_() {
        children_.reserve(64UL);
        /*for (std::size_t i = 0; i != 64UL; ++i) {
            children_.emplace_back(nullptr);
        }*/
    }
};

}  // namespace util
