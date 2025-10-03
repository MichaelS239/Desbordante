#include "algorithms/dd/fastdd/trees/node.h"

namespace algos::dd {

std::unique_ptr<Node> Node::CreateInnerNode(std::unique_ptr<Node> first_leaf,
                                            std::unique_ptr<Node> second_leaf,
                                            std::size_t bit) const {
    bool first_bit = first_leaf->bitset_.value()[bit];
    bool second_bit = second_leaf->bitset_.value()[bit];
    while (first_bit == second_bit) {
        ++bit;
        first_bit = first_leaf->bitset_.value()[bit];
        second_bit = second_leaf->bitset_.value()[bit];
    }

    return std::make_unique<Node>(bit, first_bit ? std::move(second_leaf) : std::move(first_leaf),
                                  first_bit ? std::move(first_leaf) : std::move(second_leaf));
}

std::unique_ptr<Node> Node::Add(std::unique_ptr<Node> this_node,
                                boost::dynamic_bitset<> const& bitset, std::size_t bit) {
    if (node_type_ == NodeType::EmptyNode) {
        return std::make_unique<Node>(bitset_.value());
    }
    if (node_type_ == NodeType::LeafNode) {
        if (bitset == bitset_) {
            return this_node;
        }
        return CreateInnerNode(std::move(this_node), std::make_unique<Node>(bitset), bit);
    }

    while (bit < bit_) {
        bool bitset_value = bitset[bit];
        bool union_value = union_.value()[bit];
        if (bitset_value != union_value) {
            std::unique_ptr<Node> left_node =
                    bitset_value ? std::move(this_node) : std::make_unique<Node>(bitset);
            std::unique_ptr<Node> right_node =
                    bitset_value ? std::make_unique<Node>(bitset) : std::move(this_node);
            boost::dynamic_bitset<> union_bitset = boost::operator|(union_.value(), bitset);
            boost::dynamic_bitset<> intersect_bitset = boost::operator&(intersect_.value(), bitset);
            return std::make_unique<Node>(bit, std::move(left_node), std::move(right_node),
                                          union_bitset, intersect_bitset);
        }
        ++bit;
    }
    assert(bit == bit_);

    if (bitset[bit]) {
        std::unique_ptr<Node> new_node =
                right_child_->Add(std::move(right_child_), bitset, bit + 1);
        right_child_ = std::move(new_node);
    } else {
        std::unique_ptr<Node> new_node = left_child_->Add(std::move(right_child_), bitset, bit + 1);
        left_child_ = std::move(new_node);
    }
    union_->operator|=(bitset);
    intersect_->operator&=(bitset);

    return this_node;
}

}  // namespace algos::dd
