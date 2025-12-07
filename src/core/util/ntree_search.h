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

#include "model/types/bitset.h"

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
    // boost::unordered::unordered_flat_map<std::size_t, std::unique_ptr<NTreeSearch>> children_;
    std::vector<std::unique_ptr<NTreeSearch>> children_;
    // boost::dynamic_bitset<> children_bitset_;
    model::Bitset<64> children_bitset_;

    // Optional to hold a terminal bitset at this node.
    // If present, it represents a complete bitset stored here.
    // std::optional<boost::dynamic_bitset<>> stored_bitset_;

    std::optional<model::Bitset<64>> stored_bitset_;

    static model::Bitset<64> ToStaticBitset(boost::dynamic_bitset<> const& bs) {
        model::Bitset<64> bitset;
        for (std::size_t index = bs.find_first(); index != boost::dynamic_bitset<>::npos;
             index = bs.find_next(index)) {
            bitset.set(index);
        }
        // LOG(INFO) << bitset.to_string();
        return bitset;
    }

    void InsertImpl(/*boost::dynamic_bitset<>*/ model::Bitset<64> const& bs, std::size_t cur_bit,
                    std::size_t next_bit) {
        if (next_bit == 64 /*boost::dynamic_bitset<>::npos*/) {
            // LOG(INFO) << "STORED";
            stored_bitset_ = bs;
            return;
        }
        // LOG(INFO) << next_bit;

        if (/*children_.empty()*/ children_bitset_.none()) {
            if (!stored_bitset_) {
                stored_bitset_ = bs;
                return;
            } else {
                std::size_t stored_next_bit = cur_bit == 64 ? stored_bitset_->_Find_first()
                                                            : stored_bitset_->_Find_next(cur_bit);
                if (stored_next_bit != 64 /*boost::dynamic_bitset<>::npos*/) {
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

        // child->InsertImpl(bs, /*next_bit,*/ bs.find_next(next_bit));
        child->InsertImpl(bs, next_bit, bs._Find_next(next_bit));
    }

    bool FindSubset(/*boost::dynamic_bitset<>*/ model::Bitset<64> const& bs,
                    std::size_t next_bit) const {
        //++util::NTreeSearch::count;
        // If the current node stores a bitset, it is a subset by definition
        if (stored_bitset_) {
            // LOG(INFO) << "GOT!!!";
            return stored_bitset_ == (bs & stored_bitset_.value());
            // return true;
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
        // auto children_end = children_.end();
        while (next_bit != 64 /*boost::dynamic_bitset<>::npos*/) {
            //++util::NTreeSearch::count;
            // std::size_t next_index = bs.find_next(next_bit);
            std::size_t next_index = bs._Find_next(next_bit);
            /*if (auto it = children_.find(next_bit); it != children_end) {
                if (it->second->FindSubset(bs, next_index)) {
                    return true;
                }
            }*/
            if (children_bitset_[next_bit]) {
                if (/*children_.at(next_bit)*/ children_[next_bit]->FindSubset(bs, next_index)) {
                    return true;
                }
            }
            next_bit = next_index;
        }

        return false;
    }

    bool GetAndRemoveGeneralizations(
            /*boost::dynamic_bitset<>*/ model::Bitset<64> const& bs, std::size_t next_bit,
            std::vector</*boost::dynamic_bitset<>*/ model::Bitset<64>>& result) {
        if (stored_bitset_) {
            if (stored_bitset_ == (bs & stored_bitset_.value())) {
                result.push_back(stored_bitset_.value());
                stored_bitset_.reset();
            } else {
                return false;
            }
        }

        /*for (std::size_t index = next_bit; index != boost::dynamic_bitset<>::npos;
             index = bs.find_next(index)) {
            if (auto it = children_.find(index); it != children_.end()) {
                if (it->second->GetAndRemoveGeneralizations(bs, bs.find_next(index), result)) {
                    children_.erase(index);
                }
            }
        }*/
        while (next_bit != 64 /*boost::dynamic_bitset<>::npos*/) {
            // std::size_t next_index = bs.find_next(next_bit);
            std::size_t next_index = bs._Find_next(next_bit);
            /*if (auto it = children_.find(next_bit); it != children_.end()) {
                if (it->second->GetAndRemoveGeneralizations(bs, next_index, result)) {
                    children_.erase(next_bit);
                }
            }*/
            if (children_bitset_[next_bit]) {
                if (children_[next_bit]->GetAndRemoveGeneralizations(bs, next_index, result)) {
                    children_[next_bit] = nullptr;
                    // children_.erase(next_bit);
                    children_bitset_.set(next_bit, false);
                }
            }
            next_bit = next_index;
        }

        // return children_.empty();
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
            if (!is_end && (/*!root->children_.empty()*/ !root->children_bitset_.none() ||
                            root->stored_bitset_)) {
                // traversal_.emplace(root, root->children_.cbegin());
                traversal_.emplace(root, root->children_bitset_._Find_first());
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
            while (/*cur_node.second == cur_node.first->children_.cend()*/ cur_node.second == 64
                   /*boost::dynamic_bitset<>::npos*/
                   ||
                   /*++cur_it_copy == cur_node.first->children_.cend()*/ cur_node.first
                                   ->children_bitset_._Find_next(cur_it_copy) == 64
                   /*boost::dynamic_bitset<>::npos*/) {
                cur_it_copy = cur_node.second;
                // LOG(INFO) << "...";
                if (/*cur_it_copy != cur_node.first->children_.cend()*/ cur_it_copy != 64
                    /*boost::dynamic_bitset<>::npos*/
                    &&
                    /*++cur_it_copy == cur_node.first->children_.cend()*/
                    cur_node.first->children_bitset_._Find_next(cur_it_copy) == 64
                    /*boost::dynamic_bitset<>::npos*/
                    && cur_node.first->stored_bitset_) {
                    //++traversal_.top().second;
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

            //++traversal_.top().second;
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
        using Node = std::pair<
                NTreeSearch*, std::size_t
                /*boost::unordered::unordered_flat_map<std::size_t,
                                                     std::unique_ptr<NTreeSearch>>::const_iterator*/
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
                if (/*cur_node.first->children_.empty()*/ cur_node.first->children_bitset_.none()) {
                    // LOG(INFO) << "Found";
                    return;
                }
                // LOG(INFO) << "False";
                // NTreeSearch* child = (cur_node.second)->second.get();
                NTreeSearch* child = cur_node.first->children_[cur_node.second].get();
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
                // traversal_.emplace(child, child->children_.cbegin());
                traversal_.emplace(child, child->children_bitset_._Find_first());
                //  LOG(INFO) << "Emplace";
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

    std::vector</*boost::dynamic_bitset<>*/ model::Bitset<64>> GetAndRemoveGeneralizations(
            boost::dynamic_bitset<> const& bs) {
        std::vector</*boost::dynamic_bitset<>*/ model::Bitset<64>> removed;
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

    NTreeSearch() : children_(64UL), children_bitset_(/*64UL*/), stored_bitset_() {
        // children_.reserve(64UL);
        /*for (std::size_t i = 0; i != 64UL; ++i) {
            children_.emplace_back(nullptr);
        }*/
    }

    NTreeSearch(std::optional</*boost::dynamic_bitset<>*/ model::Bitset<64>> const& bs)
        : children_(64UL), children_bitset_(/*64UL*/), stored_bitset_(bs) {
        // children_.reserve(64UL);
    }
};

}  // namespace util
