#include "algorithms/md/hymd/lattice/md_lattice.h"

#include <algorithm>
#include <cassert>
#include <stack>
#include <type_traits>

#include "algorithms/md/hymd/lattice/md_specialization.h"
#include "algorithms/md/hymd/lattice/multi_md_specialization.h"
#include "algorithms/md/hymd/lattice/rhs.h"
#include "algorithms/md/hymd/lattice/spec_generalization_checker.h"
#include "algorithms/md/hymd/lattice/total_generalization_checker.h"
#include "algorithms/md/hymd/lowest_cc_value_id.h"
#include "util/desbordante_assume.h"
#include "util/erase_if_replace.h"

namespace {
using namespace algos::hymd::lattice;
using namespace algos::hymd;
using model::Index;
template <typename MdInfoType>
using MdGenChecker = TotalGeneralizationChecker<MdNode, MdInfoType>;
template <typename MdInfoType>
using MdSpecGenChecker = SpecGeneralizationChecker<MdNode, MdInfoType>;
}  // namespace

namespace algos::hymd::lattice {

bool delete_empty_nodes = true;

// TODO: remove recursion
MdLattice::MdLattice(SingleLevelFunc single_level_func,
                     std::vector<LhsCCVIdsInfo> const& lhs_ccv_id_info, bool prune_nondisjoint,
                     std::size_t max_cardinality, Rhs max_rhs)
    : column_matches_size_(lhs_ccv_id_info.size()),
      md_root_(column_matches_size_, std::move(max_rhs)),
      support_root_(column_matches_size_),
      get_single_level_(std::move(single_level_func)),
      lhs_ccv_id_info_(&lhs_ccv_id_info),
      prune_nondisjoint_(prune_nondisjoint),
      max_cardinality_(max_cardinality) {
    enabled_rhs_indices_.resize(column_matches_size_, true);
}

inline void MdLattice::Specialize(MdLhs const& lhs,
                                  PairComparisonResult const& pair_comparison_result,
                                  Rhss const& rhss) {
    auto get_pair_lhs_ccv_id = [&](Index index, ...) {
        return (*lhs_ccv_id_info_)[index].rhs_to_lhs_map[pair_comparison_result.rhss[index]];
    };
    Specialize(lhs, rhss, get_pair_lhs_ccv_id, get_pair_lhs_ccv_id);
}

inline void MdLattice::Specialize(MdLhs const& lhs, Rhss const& rhss) {
    auto get_lowest = [](...) { return kLowestCCValueId; };
    auto get_lhs_ccv_id = [](Index, ColumnClassifierValueId ccv_id) { return ccv_id; };
    Specialize(lhs, rhss, get_lhs_ccv_id, get_lowest);
}

inline void MdLattice::SpecializeElement(MdLhs const& lhs, auto& rhs, MdLhs::iterator lhs_iter,
                                         model::Index spec_child_index,
                                         ColumnClassifierValueId spec_past,
                                         model::Index lhs_spec_index, auto support_check_method,
                                         auto add_rhs) {
    std::vector<ColumnClassifierValueId> const& lhs_ccv_ids =
            (*lhs_ccv_id_info_)[lhs_spec_index].lhs_to_rhs_map;
    // TODO: enforce this with a special class (basically a vector that guarantees this condition).
    assert(!lhs_ccv_ids.empty());
    ++spec_past;
    if (spec_past == lhs_ccv_ids.size()) return;
    assert(spec_past < lhs_ccv_ids.size());
    LhsSpecialization lhs_spec{lhs, {lhs_iter, {spec_child_index, spec_past}}};
    bool const is_unsupported = (this->*support_check_method)(lhs_spec);
    if (is_unsupported) return;
    add_rhs(rhs, lhs_spec, lhs_spec_index);
}

inline void MdLattice::Specialize(MdLhs const& lhs, Rhss const& rhss, auto get_lhs_ccv_id,
                                  auto get_nonlhs_ccv_id) {
    switch (rhss.size()) {
        case 0:
            break;
        case 1:
            SpecializeSingle(lhs, get_lhs_ccv_id, get_nonlhs_ccv_id, rhss.front());
            break;
        default:
            SpecializeMulti(lhs, get_lhs_ccv_id, get_nonlhs_ccv_id, rhss);
    }
}

inline void MdLattice::SpecializeSingle(MdLhs const& lhs, auto get_lhs_ccv_id,
                                        auto get_nonlhs_ccv_id, MdElement rhs) {
    auto add_rhs_single = [&](auto add_method) {
        return [this, add_method](MdElement rhs, LhsSpecialization lhs_spec,
                                  model::Index lhs_spec_index) {
            auto const& [index, ccv_id] = rhs;
            if (index == lhs_spec_index) {
                if (prune_nondisjoint_) return;
                ColumnClassifierValueId const ccv_id_triviality_bound =
                        (*lhs_ccv_id_info_)[lhs_spec_index]
                                .lhs_to_rhs_map[lhs_spec.specialization_data.new_child.ccv_id];
                if (ccv_id <= ccv_id_triviality_bound) return;
            }
            (this->*add_method)({lhs_spec, rhs});
        };
    };
    DoSpecialize<MdSpecialization>(lhs, get_lhs_ccv_id, get_nonlhs_ccv_id, rhs, add_rhs_single);
}

inline void MdLattice::SpecializeMulti(MdLhs const& lhs, auto get_lhs_ccv_id,
                                       auto get_nonlhs_ccv_id, Rhss const& rhss) {
    auto enabled_rhss = utility::ExclusionList<MdElement>::Create(rhss, enabled_rhs_indices_);
    std::size_t const rhss_size = enabled_rhss.Size();
    auto add_rhs_multi = [&](auto add_method) {
        return [this, add_method, &enabled_rhss, &rhss_size](
                       Rhss const& rhss, LhsSpecialization lhs_spec, model::Index lhs_spec_index) {
            for (model::Index i = 0; i != rhss_size; ++i) {
                MdElement const& rhs = rhss[i];
                if (rhs.index == lhs_spec_index) {
                    if (prune_nondisjoint_) {
                        enabled_rhss.GetEnabled().set(i, false);
                    } else {
                        ColumnClassifierValueId const ccv_id_triviality_bound =
                                (*lhs_ccv_id_info_)[lhs_spec_index].lhs_to_rhs_map
                                        [lhs_spec.specialization_data.new_child.ccv_id];
                        if (rhs.ccv_id <= ccv_id_triviality_bound)
                            enabled_rhss.GetEnabled().set(i, false);
                    }
                    break;
                }
            }
            (this->*add_method)({lhs_spec, enabled_rhss});
            enabled_rhss.Reset();
        };
    };
    DoSpecialize<MultiMdSpecialization>(lhs, get_lhs_ccv_id, get_nonlhs_ccv_id, rhss,
                                        add_rhs_multi);
}

template <typename MdInfoType>
inline void MdLattice::DoSpecialize(MdLhs const& lhs, auto get_lhs_ccv_id, auto get_nonlhs_ccv_id,
                                    auto& rhs, auto add_rhs) {
    Index lhs_spec_index = 0;
    auto lhs_iter = lhs.begin(), lhs_end = lhs.end();
    if (lhs.Cardinality() == max_cardinality_) {
        for (; lhs_iter != lhs_end; ++lhs_iter) {
            auto const& [child_array_index, ccv_id] = *lhs_iter;
            lhs_spec_index += child_array_index;
            SpecializeElement(lhs, rhs, lhs_iter, child_array_index,
                              get_lhs_ccv_id(lhs_spec_index, ccv_id), lhs_spec_index,
                              &MdLattice::IsUnsupportedReplace,
                              add_rhs(&MdLattice::AddIfMinimalReplace<MdInfoType>));
            ++lhs_spec_index;
        }
        return;
    }
    for (; lhs_iter != lhs_end; ++lhs_iter, ++lhs_spec_index) {
        auto const& [child_array_index, ccv_id] = *lhs_iter;
        for (Index spec_child_index = 0; spec_child_index != child_array_index;
             ++spec_child_index, ++lhs_spec_index) {
            SpecializeElement(lhs, rhs, lhs_iter, spec_child_index,
                              get_nonlhs_ccv_id(lhs_spec_index), lhs_spec_index,
                              &MdLattice::IsUnsupportedNonReplace,
                              add_rhs(&MdLattice::AddIfMinimalInsert<MdInfoType>));
        }
        SpecializeElement(lhs, rhs, lhs_iter, child_array_index,
                          get_lhs_ccv_id(lhs_spec_index, ccv_id), lhs_spec_index,
                          &MdLattice::IsUnsupportedReplace,
                          add_rhs(&MdLattice::AddIfMinimalReplace<MdInfoType>));
    }
    for (Index spec_child_index = 0; lhs_spec_index != column_matches_size_;
         ++lhs_spec_index, ++spec_child_index) {
        SpecializeElement(lhs, rhs, lhs_iter, spec_child_index, get_nonlhs_ccv_id(lhs_spec_index),
                          lhs_spec_index, &MdLattice::IsUnsupportedNonReplace,
                          add_rhs(&MdLattice::AddIfMinimalAppend<MdInfoType>));
    }
}

template <typename MdInfoType>
inline void MdLattice::AddNewMinimal(MdNode& cur_node, MdInfoType const& md,
                                     MdLhs::iterator cur_node_iter, auto handle_level_update_tail) {
    assert(cur_node.rhs.IsEmpty());
    assert(cur_node_iter >= md.lhs_specialization.specialization_data.spec_before);
    auto set_rhs = [&](MdNode* node) { node->SetRhs(md.GetRhs()); };
    AddUnchecked(
            &cur_node, md.lhs_specialization.old_lhs, cur_node_iter, set_rhs,
            [&](MdNode* node, model::Index child_array_index, ColumnClassifierValueId next_ccv_id) {
                return node->AddOneUnchecked(child_array_index, next_ccv_id, column_matches_size_);
            });
    if (get_single_level_) UpdateMaxLevel(md.lhs_specialization, handle_level_update_tail);
}

inline void MdLattice::UpdateMaxLevel(LhsSpecialization const& lhs, auto handle_tail) {
    std::size_t level = 0;
    auto const& [spec_child_array_index, spec_ccv_id] = lhs.specialization_data.new_child;
    MdLhs const& old_lhs = lhs.old_lhs;
    MdLhs::iterator spec_iter = lhs.specialization_data.spec_before;
    Index cur_col_match_index = 0;
    MdLhs::iterator lhs_iter = old_lhs.begin();
    auto add_level = [&]() {
        auto const& [index_delta, ccv_id] = *lhs_iter;
        cur_col_match_index += index_delta;
        level += get_single_level_(ccv_id, cur_col_match_index);
        ++cur_col_match_index;
    };
    auto add_until = [&](MdLhs::iterator end_iter) {
        for (; lhs_iter != end_iter; ++lhs_iter) add_level();
    };
    add_until(spec_iter);
    level += get_single_level_(spec_ccv_id, cur_col_match_index + spec_child_array_index);
    MdLhs::iterator const lhs_end = old_lhs.end();
    auto add_until_end = [&]() { add_until(lhs_end); };
    handle_tail(add_until_end, lhs_iter);
    if (level > max_level_) max_level_ = level;
}

template <>
class MdLattice::GeneralizationHelper<Md> {
private:
    MdElement const rhs_;
    MdNode* node_;
    MdGenChecker<Md>& gen_checker_;

public:
    GeneralizationHelper(MdNode& root, auto& gen_checker) noexcept
        : rhs_(gen_checker.GetUnspecialized().rhs), node_(&root), gen_checker_(gen_checker) {}

    bool SetAndCheck(MdNode* node_ptr) noexcept {
        node_ = node_ptr;
        if (!node_) return true;
        if (node_->rhs.IsEmpty()) return false;
        return node_->rhs[rhs_.index] >= rhs_.ccv_id;
    }

    MdNode& CurNode() noexcept {
        return *node_;
    }

    MdNodeChildren& Children() noexcept {
        return node_->children;
    }

    void SetRhsOnCurrent() noexcept {
        node_->rhs.Set(rhs_.index, rhs_.ccv_id);
    }

    auto& GetTotalChecker() noexcept {
        return gen_checker_;
    }
};

template <>
class MdLattice::GeneralizationHelper<MultiMd> {
private:
    MdNode* node_;
    MdGenChecker<MultiMd>& gen_checker_;

public:
    GeneralizationHelper(MdNode& root, auto& gen_checker)
        : node_(&root), gen_checker_(gen_checker) {}

    bool SetAndCheck(MdNode* node_ptr) noexcept {
        node_ = node_ptr;
        if (!node_) return true;
        return gen_checker_.CheckNode(*node_);
    }

    MdNode& CurNode() noexcept {
        return *node_;
    }

    MdNodeChildren& Children() noexcept {
        return node_->children;
    }

    void SetRhsOnCurrent() noexcept {
        node_->SetRhs(gen_checker_.GetUnspecialized().rhss);
    }

    auto& GetTotalChecker() noexcept {
        return gen_checker_;
    }
};

template <typename HelperType>
inline MdNode* MdLattice::TryGetNextNodeChildMap(MdCCVIdChildMap& child_map, HelperType& helper,
                                                 model::Index child_array_index,
                                                 auto new_minimal_action,
                                                 ColumnClassifierValueId const next_lhs_ccv_id,
                                                 MdLhs::iterator iter, auto get_child_map_iter,
                                                 std::size_t gen_check_offset) {
    MdNode& cur_node = helper.CurNode();
    auto it = get_child_map_iter(child_map);
    auto& total_checker = helper.GetTotalChecker();
    for (auto end_it = child_map.end(); it != end_it; ++it) {
        auto const& [generalization_ccv_id, node] = *it;
        if (generalization_ccv_id > next_lhs_ccv_id) break;
        if (generalization_ccv_id == next_lhs_ccv_id) return &it->second;
        if (total_checker.HasGeneralization(node, iter, gen_check_offset)) return nullptr;
    }
    using std::forward_as_tuple;
    MdNode& new_node =
            child_map
                    .emplace_hint(it, std::piecewise_construct, forward_as_tuple(next_lhs_ccv_id),
                                  forward_as_tuple(column_matches_size_,
                                                   cur_node.GetChildArraySize(child_array_index)))
                    ->second;
    new_minimal_action(new_node);
    return nullptr;
}

// NOTE: writing this in AddIfMinimal with gotos may be faster.
template <typename HelperType>
inline MdNode* MdLattice::TryGetNextNode(HelperType& helper, Index const child_array_index,
                                         auto new_minimal_action,
                                         ColumnClassifierValueId const next_lhs_ccv_id,
                                         MdLhs::iterator iter, std::size_t gen_check_offset) {
    MdNode& cur_node = helper.CurNode();
    MdOptionalMap& optional_map = cur_node.children[child_array_index];
    if (!optional_map.HasValue()) [[unlikely]] {
        MdNode& new_node = optional_map
                                   ->try_emplace(next_lhs_ccv_id, column_matches_size_,
                                                 cur_node.GetChildArraySize(child_array_index))
                                   .first->second;
        new_minimal_action(new_node);
        return nullptr;
    }
    return TryGetNextNodeChildMap(
            *optional_map, helper, child_array_index, new_minimal_action, next_lhs_ccv_id, iter,
            [](MdCCVIdChildMap& child_map) { return child_map.begin(); }, gen_check_offset);
}

template <typename MdInfoType>
inline void MdLattice::AddIfMinimal(MdInfoType md, auto handle_tail, auto gen_checker_method) {
    MdSpecGenChecker<MdInfoType> gen_checker{md};
    auto& total_checker = gen_checker.GetTotalChecker();
    auto helper = GeneralizationHelper<typename MdInfoType::Unspecialized>(md_root_, total_checker);
    MdLhs::iterator spec_iter = md.lhs_specialization.specialization_data.spec_before;
    MdLhs const& old_lhs = md.lhs_specialization.old_lhs;
    MdLhs::iterator next_lhs_iter = old_lhs.begin();
    while (next_lhs_iter != spec_iter) {
        auto const& [child_array_index, next_lhs_ccv_id] = *next_lhs_iter;
        ++next_lhs_iter;
        if ((gen_checker.*gen_checker_method)(helper.CurNode(), next_lhs_iter,
                                              child_array_index + 1))
            return;
        assert(helper.Children()[child_array_index].HasValue());
        MdCCVIdChildMap& child_map = *helper.Children()[child_array_index];
        assert(child_map.find(next_lhs_ccv_id) != child_map.end());
        auto it = child_map.begin();
        for (; it->first != next_lhs_ccv_id; ++it) {
            if ((gen_checker.*gen_checker_method)(it->second, next_lhs_iter, 0)) return;
        }
        helper.SetAndCheck(&it->second);
    }
    handle_tail(helper);
}

template <typename MdInfoType, typename HelperType>
inline void MdLattice::WalkToTail(MdInfoType md, HelperType& helper, MdLhs::iterator next_lhs_iter,
                                  auto handle_level_update_tail) {
    auto& total_checker = helper.GetTotalChecker();
    MdLhs::iterator lhs_end = md.lhs_specialization.old_lhs.end();
    while (next_lhs_iter != lhs_end) {
        auto const& [child_array_index, next_lhs_ccv_id] = *next_lhs_iter;
        ++next_lhs_iter;
        auto add_normal = [&](MdNode& node) {
            AddNewMinimal(node, md, next_lhs_iter, handle_level_update_tail);
        };
        if (total_checker.HasGeneralizationInChildren(helper.CurNode(), next_lhs_iter,
                                                      child_array_index + 1))
            return;
        if (helper.SetAndCheck(TryGetNextNode(helper, child_array_index, add_normal,
                                              next_lhs_ccv_id, next_lhs_iter)))
            return;
    }
    // NOTE: Metanome implemented this incorrectly, potentially missing out on recommendations.
    helper.SetRhsOnCurrent();
}

template <typename MdInfoType>
inline void MdLattice::AddIfMinimalAppend(MdInfoType md) {
    MdLhs::iterator spec_iter = md.lhs_specialization.specialization_data.spec_before;
    assert(spec_iter == md.lhs_specialization.old_lhs.end());
    auto const& [spec_child_array_index, spec_ccv_id] =
            md.lhs_specialization.specialization_data.new_child;
    auto do_nothing = [](...) {};
    auto handle_tail = [&](auto& helper) {
        auto add_normal = [&](MdNode& node) { AddNewMinimal(node, md, spec_iter, do_nothing); };
        if (helper.SetAndCheck(TryGetNextNode(helper, spec_child_array_index, add_normal,
                                              spec_ccv_id, spec_iter)))
            return;
        helper.SetRhsOnCurrent();
    };
    AddIfMinimal(md, handle_tail,
                 &MdSpecGenChecker<MdInfoType>::HasGeneralizationInChildrenNonReplace);
}

template <typename MdInfoType>
inline void MdLattice::AddIfMinimalReplace(MdInfoType md) {
    MdLhs::iterator spec_iter = md.lhs_specialization.specialization_data.spec_before;
    assert(spec_iter != md.lhs_specialization.old_lhs.end());
    auto const& [spec_child_array_index, spec_ccv_id] =
            md.lhs_specialization.specialization_data.new_child;
    auto const& [child_array_index, old_ccv_id] = *spec_iter;
    assert(spec_child_array_index == child_array_index);
    assert(old_ccv_id < spec_ccv_id);
    auto skip_one = [](auto add_until_end, MdLhs::iterator& iter) {
        ++iter;
        add_until_end();
    };
    auto handle_tail = [&](auto& helper) {
        assert(helper.Children()[child_array_index].HasValue());
        auto get_higher = [&](MdCCVIdChildMap& child_map) {
            return child_map.upper_bound(old_ccv_id);
        };
        ++spec_iter;
        auto add_normal = [&](MdNode& node) { AddNewMinimal(node, md, spec_iter, skip_one); };
        if (helper.SetAndCheck(TryGetNextNodeChildMap(*helper.Children()[child_array_index], helper,
                                                      spec_child_array_index, add_normal,
                                                      spec_ccv_id, spec_iter, get_higher)))
            return;
        WalkToTail(md, helper, spec_iter, skip_one);
    };
    AddIfMinimal(md, handle_tail,
                 &MdSpecGenChecker<MdInfoType>::HasGeneralizationInChildrenReplace);
}

template <typename MdInfoType>
inline void MdLattice::AddIfMinimalInsert(MdInfoType md) {
    MdLhs::iterator spec_iter = md.lhs_specialization.specialization_data.spec_before;
    assert(spec_iter != md.lhs_specialization.old_lhs.end());
    auto const& [spec_child_array_index, spec_ccv_id] =
            md.lhs_specialization.specialization_data.new_child;
    auto const& [old_child_array_index, next_lhs_ccv_id] = *spec_iter;
    assert(old_child_array_index > spec_child_array_index);
    std::size_t const offset = -(spec_child_array_index + 1);
    std::size_t const fol_spec_child_index = old_child_array_index + offset;
    auto add_all = [](auto add_until_end, ...) { add_until_end(); };
    auto fol_add = [&](MdNode& node) {
        AddNewMinimal(
                *node.AddOneUnchecked(fol_spec_child_index, next_lhs_ccv_id, column_matches_size_),
                md, spec_iter + 1, add_all);
    };
    auto handle_tail = [&](auto& helper) {
        auto& total_checker = helper.GetTotalChecker();
        if (helper.SetAndCheck(TryGetNextNode(helper, spec_child_array_index, fol_add, spec_ccv_id,
                                              spec_iter, offset)))
            return;
        if (total_checker.HasGeneralizationInChildren(helper.CurNode(), spec_iter, offset)) return;
        ++spec_iter;
        auto add_normal = [&](MdNode& node) { AddNewMinimal(node, md, spec_iter, add_all); };
        if (helper.SetAndCheck(TryGetNextNode(helper, fol_spec_child_index, add_normal,
                                              next_lhs_ccv_id, spec_iter)))
            return;
        WalkToTail(md, helper, spec_iter, add_all);
    };
    AddIfMinimal(md, handle_tail,
                 &MdSpecGenChecker<MdInfoType>::HasGeneralizationInChildrenNonReplace);
}

std::size_t MdLattice::MdRefiner::Refine() {
    std::size_t removed = 0;
    Rhs& rhs = node_info_.node->rhs;
    for (auto new_rhs : invalidated_.GetUpdateView()) {
        auto const& [rhs_index, new_ccv_id] = new_rhs;
        DESBORDANTE_ASSUME(rhs.begin[rhs_index] != kLowestCCValueId);
        rhs.Set(rhs_index, kLowestCCValueId);
        bool const trivial = new_ccv_id == kLowestCCValueId;
        if (trivial) {
            ++removed;
            continue;
        }
        bool const not_minimal = lattice_->HasGeneralization({GetLhs(), new_rhs});
        if (not_minimal) {
            ++removed;
            continue;
        }
        DESBORDANTE_ASSUME(rhs.begin[rhs_index] == kLowestCCValueId &&
                           new_ccv_id != kLowestCCValueId);
        rhs.Set(rhs_index, new_ccv_id);
    }
    lattice_->Specialize(GetLhs(), *pair_comparison_result_, invalidated_.GetInvalidated());
    if (rhs.IsEmpty() && node_info_.node->IsEmpty() && delete_empty_nodes) {
        lattice_->TryDeleteEmptyNode<false>(GetLhs());
    }
    return removed;
}

void MdLattice::TryAddRefiner(std::vector<MdRefiner>& found, MdNode& cur_node,
                              PairComparisonResult const& pair_comparison_result,
                              MdLhs const& cur_node_lhs) {
    Rhs& rhs = cur_node.rhs;
    utility::InvalidatedRhss invalidated;
    Index rhs_index = 0;
    Index cur_lhs_index = 0;
    auto try_push_no_match_classifier = [&]() {
        ColumnClassifierValueId pair_ccv_id = pair_comparison_result.rhss[rhs_index];
        ColumnClassifierValueId rhs_ccv_id = rhs[rhs_index];
        if (pair_ccv_id < rhs_ccv_id) {
            invalidated.PushBack({rhs_index, rhs_ccv_id}, pair_ccv_id);
        }
    };
    for (auto const& [child_index, lhs_ccv_id] : cur_node_lhs) {
        cur_lhs_index += child_index;
        for (; rhs_index != cur_lhs_index; ++rhs_index) {
            try_push_no_match_classifier();
        }
        DESBORDANTE_ASSUME(rhs_index < column_matches_size_);
        ColumnClassifierValueId const pair_ccv_id = pair_comparison_result.rhss[rhs_index];
        ColumnClassifierValueId const rhs_ccv_id = rhs[rhs_index];
        if (pair_ccv_id < rhs_ccv_id) {
            MdElement invalid{rhs_index, rhs_ccv_id};
            ColumnClassifierValueId cur_lhs_triviality_bound =
                    (*lhs_ccv_id_info_)[cur_lhs_index].lhs_to_rhs_map[lhs_ccv_id];
            if (cur_lhs_triviality_bound == pair_ccv_id) {
                invalidated.PushBack(invalid, kLowestCCValueId);
            } else {
                DESBORDANTE_ASSUME(pair_ccv_id > cur_lhs_triviality_bound);
                invalidated.PushBack(invalid, pair_ccv_id);
            }
        }
        ++rhs_index;
        ++cur_lhs_index;
    }
    for (; rhs_index != column_matches_size_; ++rhs_index) {
        try_push_no_match_classifier();
    }
    if (invalidated.IsEmpty()) return;
    found.emplace_back(this, &pair_comparison_result, MdLatticeNodeInfo{cur_node_lhs, &cur_node},
                       std::move(invalidated));
}

void MdLattice::CollectRefinersForViolated(MdNode& cur_node, std::vector<MdRefiner>& found,
                                           MdLhs& cur_node_lhs, MdLhs::iterator cur_lhs_iter,
                                           PairComparisonResult const& pair_comparison_result) {
    if (!cur_node.rhs.IsEmpty()) {
        TryAddRefiner(found, cur_node, pair_comparison_result, cur_node_lhs);
    }

    Index child_array_index = 0;
    for (MdLhs::iterator end = pair_comparison_result.maximal_matching_lhs.end();
         cur_lhs_iter != end; ++child_array_index) {
        auto const& [offset, generalization_ccv_id_limit] = *cur_lhs_iter;
        ++cur_lhs_iter;
        child_array_index += offset;
        ColumnClassifierValueId& cur_lhs_ccv_id = cur_node_lhs.AddNext(child_array_index);
        for (auto& [generalization_ccv_id, node] : *cur_node.children[child_array_index]) {
            if (generalization_ccv_id > generalization_ccv_id_limit) break;
            cur_lhs_ccv_id = generalization_ccv_id;
            CollectRefinersForViolated(node, found, cur_node_lhs, cur_lhs_iter,
                                       pair_comparison_result);
        }
        cur_node_lhs.RemoveLast();
    }
}

auto MdLattice::CollectRefinersForViolated(PairComparisonResult const& pair_comparison_result)
        -> std::vector<MdRefiner> {
    std::vector<MdRefiner> found;
    MdLhs current_lhs(pair_comparison_result.maximal_matching_lhs.Cardinality());
    CollectRefinersForViolated(md_root_, found, current_lhs,
                               pair_comparison_result.maximal_matching_lhs.begin(),
                               pair_comparison_result);
    // TODO: traverse support trie simultaneously.
    util::EraseIfReplace(found, [this](MdRefiner& refiner) {
        bool const unsupported = IsUnsupported(refiner.GetLhs());
        if (unsupported) {
            if (delete_empty_nodes) {
                TryDeleteEmptyNode<true>(refiner.GetLhs());
            } else {
                refiner.ZeroRhs();
            }
        }
        return unsupported;
    });
    return found;
}

template <bool MayNotExist>
void MdLattice::TryDeleteEmptyNode(MdLhs const& lhs) {
    std::stack<PathInfo> path_to_node;
    MdNode* cur_node_ptr = &md_root_;
    for (auto const& [offset, ccv_id] : lhs) {
        auto& map = cur_node_ptr->children[offset];
        auto it = map->find(ccv_id);
        if constexpr (MayNotExist) {
            if (it == map->end()) return;
        } else {
            DESBORDANTE_ASSUME(it != map->end());
        }
        path_to_node.emplace(cur_node_ptr, &map, it);
        cur_node_ptr = &it->second;
    }

    while (!path_to_node.empty()) {
        auto& [last_node, map_ptr, it] = path_to_node.top();
        (*map_ptr)->erase(it);
        if (map_ptr->HasValue()) break;
        if (!last_node->rhs.IsEmpty()) break;
        if (!last_node->IsEmpty()) break;
        path_to_node.pop();
    }
}

bool MdLattice::IsUnsupported(MdLhs const& lhs) const {
    return TotalGeneralizationChecker<SupportNode>{lhs}.HasGeneralization(support_root_);
}

void MdLattice::MdVerificationMessenger::MarkUnsupported() {
    // TODO: specializations can be removed from the MD lattice. If not worth it, removing just
    // this node and its children should be cheap. Though, destructors also take time.

    // This matters. Violation search can find a node with a specialized LHS but higher RHS column
    // classifier value ID, leading to extra work (though no influence on correctness, as MDs with
    // unsupported LHSs are filtered out).
    if (!delete_empty_nodes) {
        ZeroRhs();
    }

    lattice_->MarkUnsupported(GetLhs());
    if (delete_empty_nodes) {
        lattice_->TryDeleteEmptyNode<true>(GetLhs());
    }
}

void MdLattice::MdVerificationMessenger::LowerAndSpecialize(
        utility::InvalidatedRhss const& invalidated) {
    Rhs& rhs = GetRhs();
    for (auto [rhs_index, new_ccv_id] : invalidated.GetUpdateView()) {
        DESBORDANTE_ASSUME(rhs[rhs_index] != kLowestCCValueId);
        rhs.Set(rhs_index, new_ccv_id);
    }
    lattice_->Specialize(GetLhs(), invalidated.GetInvalidated());
    if (GetRhs().IsEmpty() && GetNode().IsEmpty() && delete_empty_nodes) {
        lattice_->TryDeleteEmptyNode<false>(GetLhs());
    }
}

void MdLattice::RaiseInterestingnessCCVIds(
        MdNode const& cur_node, MdLhs const& lhs,
        std::vector<ColumnClassifierValueId>& cur_interestingness_ccv_ids,
        MdLhs::iterator cur_lhs_iter, std::vector<Index> const& indices,
        std::vector<ColumnClassifierValueId> const& ccv_id_bounds, std::size_t& max_count) const {
    std::size_t const indices_size = indices.size();
    {
        if (!cur_node.rhs.IsEmpty()) {
            for (Index i = 0; i < indices_size; ++i) {
                ColumnClassifierValueId const cur_node_rhs_ccv_id = cur_node.rhs[indices[i]];
                ColumnClassifierValueId& cur_interestingness_ccv_id =
                        cur_interestingness_ccv_ids[i];
                if (cur_node_rhs_ccv_id > cur_interestingness_ccv_id) {
                    cur_interestingness_ccv_id = cur_node_rhs_ccv_id;
                    if (cur_interestingness_ccv_id == ccv_id_bounds[i]) {
                        max_count++;
                        if (max_count == indices_size) {
                            return;
                        }
                    }
                    // The original paper mentions checking for the case where all decision bounds
                    // are 1.0, but if such a situation occurs for any one RHS, and the
                    // generalization with that RHS happens to be valid on the data, it would make
                    // inference from record pairs give an incorrect result, meaning the algorithm
                    // is incorrect. However, it is possible to stop traversing when the bound's
                    // index in the list of natural decision boundaries (that being column
                    // classifier value ID) is exactly one less than the RHS bound's index, which is
                    // done here.
                }
            }
        }
    }

    Index child_array_index = 0;
    for (MdLhs::iterator end = lhs.end(); cur_lhs_iter != end; ++child_array_index) {
        auto const& [offset, generalization_ccv_id_limit] = *cur_lhs_iter;
        ++cur_lhs_iter;
        child_array_index += offset;
        for (auto const& [generalization_ccv_id, node] : *cur_node.children[child_array_index]) {
            if (generalization_ccv_id > generalization_ccv_id_limit) break;
            RaiseInterestingnessCCVIds(node, lhs, cur_interestingness_ccv_ids, cur_lhs_iter,
                                       indices, ccv_id_bounds, max_count);
            if (max_count == indices_size) return;
        }
    }
}

std::vector<ColumnClassifierValueId> MdLattice::GetInterestingnessCCVIds(
        MdLhs const& lhs, std::vector<Index> const& indices,
        std::vector<ColumnClassifierValueId> const& ccv_id_bounds) const {
    std::vector<ColumnClassifierValueId> interestingness_ccv_ids;
    std::size_t indices_size = indices.size();
    if (prune_nondisjoint_) {
        interestingness_ccv_ids.assign(indices_size, kLowestCCValueId);
    } else {
        interestingness_ccv_ids.reserve(indices_size);
        assert(std::is_sorted(indices.begin(), indices.end()));
        auto fill_interestingness_ccv_ids = [&]() {
            auto index_it = indices.begin(), index_end = indices.end();
            Index cur_index = 0;
            assert(!indices.empty());
            for (auto const& [child_index, lhs_ccv_id] : lhs) {
                cur_index += child_index;
                Index index;
                while ((index = *index_it) < cur_index) {
                    interestingness_ccv_ids.push_back(
                            (*lhs_ccv_id_info_)[index].lhs_to_rhs_map[kLowestCCValueId]);
                    if (++index_it == index_end) return;
                }
                if (cur_index == index) {
                    interestingness_ccv_ids.push_back(
                            (*lhs_ccv_id_info_)[index].lhs_to_rhs_map[lhs_ccv_id]);
                    if (++index_it == index_end) return;
                }
                ++cur_index;
            }
            while (index_it != index_end) {
                interestingness_ccv_ids.push_back(
                        (*lhs_ccv_id_info_)[*index_it].lhs_to_rhs_map[kLowestCCValueId]);
                ++index_it;
            }
        };
        fill_interestingness_ccv_ids();
    }
    std::size_t max_count = 0;
    for (Index i = 0; i < indices_size; ++i) {
        if (interestingness_ccv_ids[i] == ccv_id_bounds[i]) {
            max_count++;
        }
    }
    if (max_count == indices_size) {
        return interestingness_ccv_ids;
    }
    RaiseInterestingnessCCVIds(md_root_, lhs, interestingness_ccv_ids, lhs.begin(), indices,
                               ccv_id_bounds, max_count);
    return interestingness_ccv_ids;
}

bool MdLattice::HasGeneralization(Md const& md) const {
    return MdGenChecker<Md>{md}.HasGeneralization(md_root_);
}

void MdLattice::GetLevel(MdNode& cur_node, std::vector<MdVerificationMessenger>& collected,
                         MdLhs& cur_node_lhs, Index const cur_node_index,
                         std::size_t const level_left) {
    if (level_left == 0) {
        if (!cur_node.rhs.IsEmpty())
            collected.emplace_back(this, MdLatticeNodeInfo{cur_node_lhs, &cur_node});
        return;
    }
    auto collect = [&](MdCCVIdChildMap& child_map, model::Index child_array_index) {
        Index const next_node_index = cur_node_index + child_array_index;
        ColumnClassifierValueId& next_lhs_ccv_id = cur_node_lhs.AddNext(child_array_index);
        for (auto& [ccv_id, node] : child_map) {
            std::size_t const single = get_single_level_(next_node_index, ccv_id);
            if (single > level_left) break;
            next_lhs_ccv_id = ccv_id;
            GetLevel(node, collected, cur_node_lhs, next_node_index + 1, level_left - single);
        }
        cur_node_lhs.RemoveLast();
    };
    cur_node.ForEachNonEmpty(collect);
}

auto MdLattice::GetLevel(std::size_t const level) -> std::vector<MdVerificationMessenger> {
    std::vector<MdVerificationMessenger> collected;
    MdLhs current_lhs(column_matches_size_);
    if (!get_single_level_) {
        GetAll(md_root_, collected, current_lhs, 0);
    } else {
        GetLevel(md_root_, collected, current_lhs, 0, level);
    }
    // TODO: traverse support trie simultaneously.
    util::EraseIfReplace(collected, [this](MdVerificationMessenger& messenger) {
        bool is_unsupported = IsUnsupported(messenger.GetLhs());
        if (is_unsupported) {
            if (delete_empty_nodes) {
                TryDeleteEmptyNode<true>(messenger.GetLhs());
            } else {
                messenger.ZeroRhs();
            }
        }
        return is_unsupported;
    });
    return collected;
}

template <typename NodeInfo>
void MdLattice::GetAll(MdNode& cur_node, std::vector<NodeInfo>& collected, MdLhs& cur_node_lhs,
                       Index const this_node_index) {
    if (!cur_node.rhs.IsEmpty()) {
        if constexpr (std::is_same_v<NodeInfo, MdLatticeNodeInfo>) {
            collected.emplace_back(cur_node_lhs, &cur_node);
        } else {
            static_assert(std::is_same_v<NodeInfo, MdVerificationMessenger>);
            collected.emplace_back(this, MdLatticeNodeInfo{cur_node_lhs, &cur_node});
        }
    }
    auto collect = [&](MdCCVIdChildMap& child_map, model::Index child_array_index) {
        Index const next_node_index = this_node_index + child_array_index;
        ColumnClassifierValueId& next_lhs_ccv_id = cur_node_lhs.AddNext(child_array_index);
        for (auto& [ccv_id, node] : child_map) {
            next_lhs_ccv_id = ccv_id;
            GetAll(node, collected, cur_node_lhs, next_node_index + 1);
        }
        cur_node_lhs.RemoveLast();
    };
    cur_node.ForEachNonEmpty(collect);
}

std::vector<MdLatticeNodeInfo> MdLattice::GetAll() {
    std::vector<MdLatticeNodeInfo> collected;
    MdLhs current_lhs(column_matches_size_);
    GetAll(md_root_, collected, current_lhs, 0);
    assert(std::none_of(
            collected.begin(), collected.end(),
            [this](MdLatticeNodeInfo const& node_info) { return IsUnsupported(node_info.lhs); }));
    return collected;
}

bool MdLattice::IsUnsupportedReplace(LhsSpecialization lhs_spec) const {
    return SpecGeneralizationChecker<SupportNode>{lhs_spec}.HasGeneralizationReplace(support_root_);
}

bool MdLattice::IsUnsupportedNonReplace(LhsSpecialization lhs_spec) const {
    return SpecGeneralizationChecker<SupportNode>{lhs_spec}.HasGeneralizationNonReplace(
            support_root_);
}

void MdLattice::MarkNewLhs(SupportNode& cur_node, MdLhs const& lhs, MdLhs::iterator cur_lhs_iter) {
    AddUnchecked(&cur_node, lhs, cur_lhs_iter, SetUnsupAction());
}

void MdLattice::MarkUnsupported(MdLhs const& lhs) {
    auto mark_new = [this](auto&&... args) { MarkNewLhs(std::forward<decltype(args)>(args)...); };
    CheckedAdd(&support_root_, lhs, lhs, mark_new, SetUnsupAction());
}

}  // namespace algos::hymd::lattice
