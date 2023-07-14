/*
 * Copyright (c) 2021, Ali Mohammad Pur <mpfard@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/Debug.h>
#include <AK/QuickSort.h>
#include <AK/RedBlackTree.h>
#include <AK/Stack.h>
#include <LibRegex/Regex.h>
#include <LibRegex/RegexBytecodeStreamOptimizer.h>
#include <LibUnicode/CharacterTypes.h>
#if REGEX_DEBUG
#    include <AK/ScopeGuard.h>
#    include <AK/ScopeLogger.h>
#endif

namespace regex {

using Detail::Block;

template<typename Parser>
void Regex<Parser>::run_optimization_passes()
{
    parser_result.bytecode.flatten();

    // Rewrite fork loops as atomic groups
    // e.g. a*b -> (ATOMIC a*)b
    attempt_rewrite_loops_as_atomic_groups(split_basic_blocks(parser_result.bytecode));

    parser_result.bytecode.flatten();
}

template<typename Parser>
typename Regex<Parser>::BasicBlockList Regex<Parser>::split_basic_blocks(ByteCode const& bytecode)
{
    BasicBlockList block_boundaries;
    size_t end_of_last_block = 0;

    auto bytecode_size = bytecode.size();

    MatchState state;
    state.instruction_position = 0;
    auto check_jump = [&]<typename T>(OpCode const& opcode) {
        auto& op = static_cast<T const&>(opcode);
        ssize_t jump_offset = op.size() + op.offset();
        if (jump_offset >= 0) {
            block_boundaries.append({ end_of_last_block, state.instruction_position });
            end_of_last_block = state.instruction_position + opcode.size();
        } else {
            // This op jumps back, see if that's within this "block".
            if (jump_offset + state.instruction_position > end_of_last_block) {
                // Split the block!
                block_boundaries.append({ end_of_last_block, jump_offset + state.instruction_position });
                block_boundaries.append({ jump_offset + state.instruction_position, state.instruction_position });
                end_of_last_block = state.instruction_position + opcode.size();
            } else {
                // Nope, it's just a jump to another block
                block_boundaries.append({ end_of_last_block, state.instruction_position });
                end_of_last_block = state.instruction_position + opcode.size();
            }
        }
    };
    for (;;) {
        auto& opcode = bytecode.get_opcode(state);

        switch (opcode.opcode_id()) {
        case OpCodeId::Jump:
            check_jump.template operator()<OpCode_Jump>(opcode);
            break;
        case OpCodeId::JumpNonEmpty:
            check_jump.template operator()<OpCode_JumpNonEmpty>(opcode);
            break;
        case OpCodeId::ForkJump:
            check_jump.template operator()<OpCode_ForkJump>(opcode);
            break;
        case OpCodeId::ForkStay:
            check_jump.template operator()<OpCode_ForkStay>(opcode);
            break;
        case OpCodeId::FailForks:
            block_boundaries.append({ end_of_last_block, state.instruction_position });
            end_of_last_block = state.instruction_position + opcode.size();
            break;
        case OpCodeId::Repeat: {
            // Repeat produces two blocks, one containing its repeated expr, and one after that.
            auto repeat_start = state.instruction_position - static_cast<OpCode_Repeat const&>(opcode).offset();
            if (repeat_start > end_of_last_block)
                block_boundaries.append({ end_of_last_block, repeat_start });
            block_boundaries.append({ repeat_start, state.instruction_position });
            end_of_last_block = state.instruction_position + opcode.size();
            break;
        }
        default:
            break;
        }

        auto next_ip = state.instruction_position + opcode.size();
        if (next_ip < bytecode_size)
            state.instruction_position = next_ip;
        else
            break;
    }

    if (end_of_last_block < bytecode_size)
        block_boundaries.append({ end_of_last_block, bytecode_size });

    quick_sort(block_boundaries, [](auto& a, auto& b) { return a.start < b.start; });

    return block_boundaries;
}

static bool has_overlap(Vector<CompareTypeAndValuePair> const& lhs, Vector<CompareTypeAndValuePair> const& rhs)
{

    // We have to fully interpret the two sequences to determine if they overlap (that is, keep track of inversion state and what ranges they cover).
    bool inverse { false };
    bool temporary_inverse { false };
    bool reset_temporary_inverse { false };

    auto current_lhs_inversion_state = [&]() -> bool { return temporary_inverse ^ inverse; };

    RedBlackTree<u32, u32> lhs_ranges;
    RedBlackTree<u32, u32> lhs_negated_ranges;
    HashTable<CharClass> lhs_char_classes;
    HashTable<CharClass> lhs_negated_char_classes;

    auto has_any_unicode_property = false;
    HashTable<Unicode::GeneralCategory> lhs_unicode_general_categories;
    HashTable<Unicode::Property> lhs_unicode_properties;
    HashTable<Unicode::Script> lhs_unicode_scripts;
    HashTable<Unicode::Script> lhs_unicode_script_extensions;
    HashTable<Unicode::GeneralCategory> lhs_negated_unicode_general_categories;
    HashTable<Unicode::Property> lhs_negated_unicode_properties;
    HashTable<Unicode::Script> lhs_negated_unicode_scripts;
    HashTable<Unicode::Script> lhs_negated_unicode_script_extensions;

    auto any_unicode_property_matches = [&](u32 code_point) {
        if (any_of(lhs_negated_unicode_general_categories, [code_point](auto category) { return Unicode::code_point_has_general_category(code_point, category); }))
            return false;
        if (any_of(lhs_negated_unicode_properties, [code_point](auto property) { return Unicode::code_point_has_property(code_point, property); }))
            return false;
        if (any_of(lhs_negated_unicode_scripts, [code_point](auto script) { return Unicode::code_point_has_script(code_point, script); }))
            return false;
        if (any_of(lhs_negated_unicode_script_extensions, [code_point](auto script) { return Unicode::code_point_has_script_extension(code_point, script); }))
            return false;

        if (any_of(lhs_unicode_general_categories, [code_point](auto category) { return Unicode::code_point_has_general_category(code_point, category); }))
            return true;
        if (any_of(lhs_unicode_properties, [code_point](auto property) { return Unicode::code_point_has_property(code_point, property); }))
            return true;
        if (any_of(lhs_unicode_scripts, [code_point](auto script) { return Unicode::code_point_has_script(code_point, script); }))
            return true;
        if (any_of(lhs_unicode_script_extensions, [code_point](auto script) { return Unicode::code_point_has_script_extension(code_point, script); }))
            return true;
        return false;
    };

    auto range_contains = [&]<typename T>(T& value) -> bool {
        u32 start;
        u32 end;

        if constexpr (IsSame<T, CharRange>) {
            start = value.from;
            end = value.to;
        } else {
            start = value;
            end = value;
        }

        if (has_any_unicode_property) {
            // We have some properties, and a range is present
            // Instead of checking every single code point in the range, assume it's a match.
            return start != end || any_unicode_property_matches(start);
        }

        auto* max = lhs_ranges.find_smallest_not_below(start);
        return max && *max <= end;
    };

    auto char_class_contains = [&](CharClass const& value) -> bool {
        if (lhs_char_classes.contains(value))
            return true;

        if (lhs_negated_char_classes.contains(value))
            return false;

        // This char class might match something in the ranges we have, and checking that is far too expensive, so just bail out.
        return true;
    };

    for (auto const& pair : lhs) {
        if (reset_temporary_inverse) {
            reset_temporary_inverse = false;
            temporary_inverse = false;
        } else {
            reset_temporary_inverse = true;
        }

        switch (pair.type) {
        case CharacterCompareType::Inverse:
            inverse = !inverse;
            break;
        case CharacterCompareType::TemporaryInverse:
            temporary_inverse = true;
            reset_temporary_inverse = true;
            break;
        case CharacterCompareType::AnyChar:
            // Special case: if not inverted, AnyChar is always in the range.
            if (!current_lhs_inversion_state())
                return true;
            break;
        case CharacterCompareType::Char:
            if (!current_lhs_inversion_state())
                lhs_ranges.insert(pair.value, pair.value);
            else
                lhs_negated_ranges.insert(pair.value, pair.value);
            break;
        case CharacterCompareType::String:
            // FIXME: We just need to look at the last character of this string, but we only have the first character here.
            //        Just bail out to avoid false positives.
            return true;
        case CharacterCompareType::CharClass:
            if (!current_lhs_inversion_state())
                lhs_char_classes.set(static_cast<CharClass>(pair.value));
            else
                lhs_negated_char_classes.set(static_cast<CharClass>(pair.value));
            break;
        case CharacterCompareType::CharRange: {
            auto range = CharRange(pair.value);
            if (!current_lhs_inversion_state())
                lhs_ranges.insert(range.from, range.to);
            else
                lhs_negated_ranges.insert(range.from, range.to);
            break;
        }
        case CharacterCompareType::LookupTable:
            // We've transformed this into a series of ranges in flat_compares(), so bail out if we see it.
            return true;
        case CharacterCompareType::Reference:
            // We've handled this before coming here.
            break;
        case CharacterCompareType::Property:
            has_any_unicode_property = true;
            if (!current_lhs_inversion_state())
                lhs_unicode_properties.set(static_cast<Unicode::Property>(pair.value));
            else
                lhs_negated_unicode_properties.set(static_cast<Unicode::Property>(pair.value));
            break;
        case CharacterCompareType::GeneralCategory:
            has_any_unicode_property = true;
            if (!current_lhs_inversion_state())
                lhs_unicode_general_categories.set(static_cast<Unicode::GeneralCategory>(pair.value));
            else
                lhs_negated_unicode_general_categories.set(static_cast<Unicode::GeneralCategory>(pair.value));
            break;
        case CharacterCompareType::Script:
            has_any_unicode_property = true;
            if (!current_lhs_inversion_state())
                lhs_unicode_scripts.set(static_cast<Unicode::Script>(pair.value));
            else
                lhs_negated_unicode_scripts.set(static_cast<Unicode::Script>(pair.value));
            break;
        case CharacterCompareType::ScriptExtension:
            has_any_unicode_property = true;
            if (!current_lhs_inversion_state())
                lhs_unicode_script_extensions.set(static_cast<Unicode::Script>(pair.value));
            else
                lhs_negated_unicode_script_extensions.set(static_cast<Unicode::Script>(pair.value));
            break;
        case CharacterCompareType::And:
        case CharacterCompareType::Or:
        case CharacterCompareType::EndAndOr:
            // FIXME: These are too difficult to handle, so bail out.
            return true;
        case CharacterCompareType::Undefined:
        case CharacterCompareType::RangeExpressionDummy:
            // These do not occur in valid bytecode.
            VERIFY_NOT_REACHED();
        }
    }

    if constexpr (REGEX_DEBUG) {
        dbgln("lhs ranges:");
        for (auto it = lhs_ranges.begin(); it != lhs_ranges.end(); ++it)
            dbgln("  {}..{}", it.key(), *it);
        dbgln("lhs negated ranges:");
        for (auto it = lhs_negated_ranges.begin(); it != lhs_negated_ranges.end(); ++it)
            dbgln("  {}..{}", it.key(), *it);
    }

    for (auto const& pair : rhs) {
        if (reset_temporary_inverse) {
            reset_temporary_inverse = false;
            temporary_inverse = false;
        } else {
            reset_temporary_inverse = true;
        }

        dbgln_if(REGEX_DEBUG, "check {} ({})...", character_compare_type_name(pair.type), pair.value);

        switch (pair.type) {
        case CharacterCompareType::Inverse:
            inverse = !inverse;
            break;
        case CharacterCompareType::TemporaryInverse:
            temporary_inverse = true;
            reset_temporary_inverse = true;
            break;
        case CharacterCompareType::AnyChar:
            // Special case: if not inverted, AnyChar is always in the range.
            if (!current_lhs_inversion_state())
                return true;
            break;
        case CharacterCompareType::Char:
            if (current_lhs_inversion_state() ^ range_contains(pair.value))
                return true;
            break;
        case CharacterCompareType::String:
            // FIXME: We just need to look at the last character of this string, but we only have the first character here.
            //        Just bail out to avoid false positives.
            return true;
        case CharacterCompareType::CharClass:
            if (current_lhs_inversion_state() ^ char_class_contains(static_cast<CharClass>(pair.value)))
                return true;
            break;
        case CharacterCompareType::CharRange: {
            auto range = CharRange(pair.value);
            if (current_lhs_inversion_state() ^ range_contains(range))
                return true;
            break;
        }
        case CharacterCompareType::LookupTable:
            // We've transformed this into a series of ranges in flat_compares(), so bail out if we see it.
            return true;
        case CharacterCompareType::Reference:
            // We've handled this before coming here.
            break;
        case CharacterCompareType::Property:
            // The only reasonable scenario where we can check these properties without spending too much time is if:
            //  - the ranges are empty
            //  - the char classes are empty
            //  - the unicode properties are empty or contain only this property
            if (!lhs_ranges.is_empty() || !lhs_negated_ranges.is_empty() || !lhs_char_classes.is_empty() || !lhs_negated_char_classes.is_empty())
                return true;
            if (has_any_unicode_property && !lhs_unicode_properties.is_empty() && !lhs_negated_unicode_properties.is_empty()) {
                if (current_lhs_inversion_state() ^ lhs_unicode_properties.contains(static_cast<Unicode::Property>(pair.value)))
                    return true;
                if (false == (current_lhs_inversion_state() ^ lhs_negated_unicode_properties.contains(static_cast<Unicode::Property>(pair.value))))
                    return true;
            }
            break;
        case CharacterCompareType::GeneralCategory:
            if (!lhs_ranges.is_empty() || !lhs_negated_ranges.is_empty() || !lhs_char_classes.is_empty() || !lhs_negated_char_classes.is_empty())
                return true;
            if (has_any_unicode_property && !lhs_unicode_general_categories.is_empty() && !lhs_negated_unicode_general_categories.is_empty()) {
                if (current_lhs_inversion_state() ^ lhs_unicode_general_categories.contains(static_cast<Unicode::GeneralCategory>(pair.value)))
                    return true;
                if (false == (current_lhs_inversion_state() ^ lhs_negated_unicode_general_categories.contains(static_cast<Unicode::GeneralCategory>(pair.value))))
                    return true;
            }
            break;
        case CharacterCompareType::Script:
            if (!lhs_ranges.is_empty() || !lhs_negated_ranges.is_empty() || !lhs_char_classes.is_empty() || !lhs_negated_char_classes.is_empty())
                return true;
            if (has_any_unicode_property && !lhs_unicode_scripts.is_empty() && !lhs_negated_unicode_scripts.is_empty()) {
                if (current_lhs_inversion_state() ^ lhs_unicode_scripts.contains(static_cast<Unicode::Script>(pair.value)))
                    return true;
                if (false == (current_lhs_inversion_state() ^ lhs_negated_unicode_scripts.contains(static_cast<Unicode::Script>(pair.value))))
                    return true;
            }
            break;
        case CharacterCompareType::ScriptExtension:
            if (!lhs_ranges.is_empty() || !lhs_negated_ranges.is_empty() || !lhs_char_classes.is_empty() || !lhs_negated_char_classes.is_empty())
                return true;
            if (has_any_unicode_property && !lhs_unicode_script_extensions.is_empty() && !lhs_negated_unicode_script_extensions.is_empty()) {
                if (current_lhs_inversion_state() ^ lhs_unicode_script_extensions.contains(static_cast<Unicode::Script>(pair.value)))
                    return true;
                if (false == (current_lhs_inversion_state() ^ lhs_negated_unicode_script_extensions.contains(static_cast<Unicode::Script>(pair.value))))
                    return true;
            }
            break;
        case CharacterCompareType::And:
        case CharacterCompareType::Or:
        case CharacterCompareType::EndAndOr:
            // FIXME: These are too difficult to handle, so bail out.
            return true;
        case CharacterCompareType::Undefined:
        case CharacterCompareType::RangeExpressionDummy:
            // These do not occur in valid bytecode.
            VERIFY_NOT_REACHED();
        }
    }

    return false;
}

enum class AtomicRewritePreconditionResult {
    SatisfiedWithProperHeader,
    SatisfiedWithEmptyHeader,
    NotSatisfied,
};
static AtomicRewritePreconditionResult block_satisfies_atomic_rewrite_precondition(ByteCode const& bytecode, Block const& repeated_block, Block const& following_block)
{
    Vector<Vector<CompareTypeAndValuePair>> repeated_values;
    HashTable<size_t> active_capture_groups;
    MatchState state;
    auto has_seen_actionable_opcode = false;
    for (state.instruction_position = repeated_block.start; state.instruction_position < repeated_block.end;) {
        auto& opcode = bytecode.get_opcode(state);
        switch (opcode.opcode_id()) {
        case OpCodeId::Compare: {
            has_seen_actionable_opcode = true;
            auto compares = static_cast<OpCode_Compare const&>(opcode).flat_compares();
            if (repeated_values.is_empty() && any_of(compares, [](auto& compare) { return compare.type == CharacterCompareType::AnyChar; }))
                return AtomicRewritePreconditionResult::NotSatisfied;
            repeated_values.append(move(compares));
            break;
        }
        case OpCodeId::CheckBegin:
        case OpCodeId::CheckEnd:
            has_seen_actionable_opcode = true;
            if (repeated_values.is_empty())
                return AtomicRewritePreconditionResult::SatisfiedWithProperHeader;
            break;
        case OpCodeId::CheckBoundary:
            // FIXME: What should we do with these? for now, let's fail.
            return AtomicRewritePreconditionResult::NotSatisfied;
        case OpCodeId::Restore:
        case OpCodeId::GoBack:
            return AtomicRewritePreconditionResult::NotSatisfied;
        case OpCodeId::SaveRightCaptureGroup:
            active_capture_groups.set(static_cast<OpCode_SaveRightCaptureGroup const&>(opcode).id());
            break;
        case OpCodeId::SaveLeftCaptureGroup:
            active_capture_groups.set(static_cast<OpCode_SaveLeftCaptureGroup const&>(opcode).id());
            break;
        case OpCodeId::ForkJump:
        case OpCodeId::ForkReplaceJump:
        case OpCodeId::JumpNonEmpty:
            // We could attempt to recursively resolve the follow set, but pretending that this just goes nowhere is faster.
            if (!has_seen_actionable_opcode)
                return AtomicRewritePreconditionResult::NotSatisfied;
            break;
        default:
            break;
        }

        state.instruction_position += opcode.size();
    }
    dbgln_if(REGEX_DEBUG, "Found {} entries in reference", repeated_values.size());
    dbgln_if(REGEX_DEBUG, "Found {} active capture groups", active_capture_groups.size());

    bool following_block_has_at_least_one_compare = false;
    // Find the first compare in the following block, it must NOT match any of the values in `repeated_values'.
    auto final_instruction = following_block.start;
    for (state.instruction_position = following_block.start; state.instruction_position < following_block.end;) {
        final_instruction = state.instruction_position;
        auto& opcode = bytecode.get_opcode(state);
        switch (opcode.opcode_id()) {
        // Note: These have to exist since we're effectively repeating the following block as well
        case OpCodeId::SaveRightCaptureGroup:
            active_capture_groups.set(static_cast<OpCode_SaveRightCaptureGroup const&>(opcode).id());
            break;
        case OpCodeId::SaveLeftCaptureGroup:
            active_capture_groups.set(static_cast<OpCode_SaveLeftCaptureGroup const&>(opcode).id());
            break;
        case OpCodeId::Compare: {
            following_block_has_at_least_one_compare = true;
            // We found a compare, let's see what it has.
            auto compares = static_cast<OpCode_Compare const&>(opcode).flat_compares();
            if (compares.is_empty())
                break;

            if (any_of(compares, [&](auto& compare) {
                    return compare.type == CharacterCompareType::AnyChar
                        || (compare.type == CharacterCompareType::Reference && active_capture_groups.contains(compare.value));
                }))
                return AtomicRewritePreconditionResult::NotSatisfied;

            if (any_of(repeated_values, [&](auto& repeated_value) { return has_overlap(compares, repeated_value); }))
                return AtomicRewritePreconditionResult::NotSatisfied;

            return AtomicRewritePreconditionResult::SatisfiedWithProperHeader;
        }
        case OpCodeId::CheckBegin:
        case OpCodeId::CheckEnd:
            return AtomicRewritePreconditionResult::SatisfiedWithProperHeader; // Nothing can match the end!
        case OpCodeId::CheckBoundary:
            // FIXME: What should we do with these? For now, consider them a failure.
            return AtomicRewritePreconditionResult::NotSatisfied;
        case OpCodeId::ForkJump:
        case OpCodeId::ForkReplaceJump:
        case OpCodeId::JumpNonEmpty:
            // See note in the previous switch, same cases.
            if (!following_block_has_at_least_one_compare)
                return AtomicRewritePreconditionResult::NotSatisfied;
            break;
        default:
            break;
        }

        state.instruction_position += opcode.size();
    }

    // If the following block falls through, we can't rewrite it.
    state.instruction_position = final_instruction;
    switch (bytecode.get_opcode(state).opcode_id()) {
    case OpCodeId::Jump:
    case OpCodeId::JumpNonEmpty:
    case OpCodeId::ForkJump:
    case OpCodeId::ForkReplaceJump:
        break;
    default:
        return AtomicRewritePreconditionResult::NotSatisfied;
    }

    if (following_block_has_at_least_one_compare)
        return AtomicRewritePreconditionResult::SatisfiedWithProperHeader;
    return AtomicRewritePreconditionResult::SatisfiedWithEmptyHeader;
}

template<typename Parser>
void Regex<Parser>::attempt_rewrite_loops_as_atomic_groups(BasicBlockList const& basic_blocks)
{
    auto& bytecode = parser_result.bytecode;
    if constexpr (REGEX_DEBUG) {
        RegexDebug dbg;
        dbg.print_bytecode(*this);
        for (auto const& block : basic_blocks)
            dbgln("block from {} to {}", block.start, block.end);
    }

    // A pattern such as:
    //     bb0       |  RE0
    //               |  ForkX bb0
    //     -------------------------
    //     bb1       |  RE1
    // can be rewritten as:
    //     -------------------------
    //     bb0       | RE0
    //               | ForkReplaceX bb0
    //     -------------------------
    //     bb1       | RE1
    // provided that first(RE1) not-in end(RE0), which is to say
    // that RE1 cannot start with whatever RE0 has matched (ever).
    //
    // Alternatively, a second form of this pattern can also occur:
    //     bb0 | *
    //         | ForkX bb2
    //     ------------------------
    //     bb1 | RE0
    //         | Jump bb0
    //     ------------------------
    //     bb2 | RE1
    // which can be transformed (with the same preconditions) to:
    //     bb0 | *
    //         | ForkReplaceX bb2
    //     ------------------------
    //     bb1 | RE0
    //         | Jump bb0
    //     ------------------------
    //     bb2 | RE1

    enum class AlternateForm {
        DirectLoopWithoutHeader,               // loop without proper header, a block forking to itself. i.e. the first form.
        DirectLoopWithoutHeaderAndEmptyFollow, // loop without proper header, a block forking to itself. i.e. the first form but with RE1 being empty.
        DirectLoopWithHeader,                  // loop with proper header, i.e. the second form.
    };
    struct CandidateBlock {
        Block forking_block;
        Optional<Block> new_target_block;
        AlternateForm form;
    };
    Vector<CandidateBlock> candidate_blocks;

    auto is_an_eligible_jump = [](OpCode const& opcode, size_t ip, size_t block_start, AlternateForm alternate_form) {
        switch (opcode.opcode_id()) {
        case OpCodeId::JumpNonEmpty: {
            auto const& op = static_cast<OpCode_JumpNonEmpty const&>(opcode);
            auto form = op.form();
            if (form != OpCodeId::Jump && alternate_form == AlternateForm::DirectLoopWithHeader)
                return false;
            if (form != OpCodeId::ForkJump && form != OpCodeId::ForkStay && alternate_form == AlternateForm::DirectLoopWithoutHeader)
                return false;
            return op.offset() + ip + opcode.size() == block_start;
        }
        case OpCodeId::ForkJump:
            if (alternate_form == AlternateForm::DirectLoopWithHeader)
                return false;
            return static_cast<OpCode_ForkJump const&>(opcode).offset() + ip + opcode.size() == block_start;
        case OpCodeId::ForkStay:
            if (alternate_form == AlternateForm::DirectLoopWithHeader)
                return false;
            return static_cast<OpCode_ForkStay const&>(opcode).offset() + ip + opcode.size() == block_start;
        case OpCodeId::Jump:
            // Infinite loop does *not* produce forks.
            if (alternate_form == AlternateForm::DirectLoopWithoutHeader)
                return false;
            if (alternate_form == AlternateForm::DirectLoopWithHeader)
                return static_cast<OpCode_Jump const&>(opcode).offset() + ip + opcode.size() == block_start;
            VERIFY_NOT_REACHED();
        default:
            return false;
        }
    };
    for (size_t i = 0; i < basic_blocks.size(); ++i) {
        auto forking_block = basic_blocks[i];
        Optional<Block> fork_fallback_block;
        if (i + 1 < basic_blocks.size())
            fork_fallback_block = basic_blocks[i + 1];
        MatchState state;
        // Check if the last instruction in this block is a jump to the block itself:
        {
            state.instruction_position = forking_block.end;
            auto& opcode = bytecode.get_opcode(state);
            if (is_an_eligible_jump(opcode, state.instruction_position, forking_block.start, AlternateForm::DirectLoopWithoutHeader)) {
                // We've found RE0 (and RE1 is just the following block, if any), let's see if the precondition applies.
                // if RE1 is empty, there's no first(RE1), so this is an automatic pass.
                if (!fork_fallback_block.has_value()
                    || (fork_fallback_block->end == fork_fallback_block->start && block_satisfies_atomic_rewrite_precondition(bytecode, forking_block, *fork_fallback_block) != AtomicRewritePreconditionResult::NotSatisfied)) {
                    candidate_blocks.append({ forking_block, fork_fallback_block, AlternateForm::DirectLoopWithoutHeader });
                    break;
                }

                auto precondition = block_satisfies_atomic_rewrite_precondition(bytecode, forking_block, *fork_fallback_block);
                if (precondition == AtomicRewritePreconditionResult::SatisfiedWithProperHeader) {
                    candidate_blocks.append({ forking_block, fork_fallback_block, AlternateForm::DirectLoopWithoutHeader });
                    break;
                }
                if (precondition == AtomicRewritePreconditionResult::SatisfiedWithEmptyHeader) {
                    candidate_blocks.append({ forking_block, fork_fallback_block, AlternateForm::DirectLoopWithoutHeaderAndEmptyFollow });
                    break;
                }
            }
        }
        // Check if the last instruction in the last block is a direct jump to this block
        if (fork_fallback_block.has_value()) {
            state.instruction_position = fork_fallback_block->end;
            auto& opcode = bytecode.get_opcode(state);
            if (is_an_eligible_jump(opcode, state.instruction_position, forking_block.start, AlternateForm::DirectLoopWithHeader)) {
                // We've found bb1 and bb0, let's just make sure that bb0 forks to bb2.
                state.instruction_position = forking_block.end;
                auto& opcode = bytecode.get_opcode(state);
                if (opcode.opcode_id() == OpCodeId::ForkJump || opcode.opcode_id() == OpCodeId::ForkStay) {
                    Optional<Block> block_following_fork_fallback;
                    if (i + 2 < basic_blocks.size())
                        block_following_fork_fallback = basic_blocks[i + 2];
                    if (!block_following_fork_fallback.has_value()
                        || block_satisfies_atomic_rewrite_precondition(bytecode, *fork_fallback_block, *block_following_fork_fallback) != AtomicRewritePreconditionResult::NotSatisfied) {
                        candidate_blocks.append({ forking_block, {}, AlternateForm::DirectLoopWithHeader });
                        break;
                    }
                }
            }
        }
    }

    dbgln_if(REGEX_DEBUG, "Found {} candidate blocks", candidate_blocks.size());
    if (candidate_blocks.is_empty()) {
        dbgln_if(REGEX_DEBUG, "Failed to find anything for {}", pattern_value);
        return;
    }

    RedBlackTree<size_t, size_t> needed_patches;

    // Reverse the blocks, so we can patch the bytecode without messing with the latter patches.
    quick_sort(candidate_blocks, [](auto& a, auto& b) { return b.forking_block.start > a.forking_block.start; });
    for (auto& candidate : candidate_blocks) {
        // Note that both forms share a ForkReplace patch in forking_block.
        // Patch the ForkX in forking_block to be a ForkReplaceX instead.
        auto& opcode_id = bytecode[candidate.forking_block.end];
        if (opcode_id == (ByteCodeValueType)OpCodeId::ForkStay) {
            opcode_id = (ByteCodeValueType)OpCodeId::ForkReplaceStay;
        } else if (opcode_id == (ByteCodeValueType)OpCodeId::ForkJump) {
            opcode_id = (ByteCodeValueType)OpCodeId::ForkReplaceJump;
        } else if (opcode_id == (ByteCodeValueType)OpCodeId::JumpNonEmpty) {
            auto& jump_opcode_id = bytecode[candidate.forking_block.end + 3];
            if (jump_opcode_id == (ByteCodeValueType)OpCodeId::ForkStay)
                jump_opcode_id = (ByteCodeValueType)OpCodeId::ForkReplaceStay;
            else if (jump_opcode_id == (ByteCodeValueType)OpCodeId::ForkJump)
                jump_opcode_id = (ByteCodeValueType)OpCodeId::ForkReplaceJump;
            else
                VERIFY_NOT_REACHED();
        } else {
            VERIFY_NOT_REACHED();
        }
    }

    if (!needed_patches.is_empty()) {
        MatchState state;
        auto bytecode_size = bytecode.size();
        state.instruction_position = 0;
        struct Patch {
            ssize_t value;
            size_t offset;
            bool should_negate { false };
        };
        for (;;) {
            if (state.instruction_position >= bytecode_size)
                break;

            auto& opcode = bytecode.get_opcode(state);
            Stack<Patch, 2> patch_points;

            switch (opcode.opcode_id()) {
            case OpCodeId::Jump:
                patch_points.push({ static_cast<OpCode_Jump const&>(opcode).offset(), state.instruction_position + 1 });
                break;
            case OpCodeId::JumpNonEmpty:
                patch_points.push({ static_cast<OpCode_JumpNonEmpty const&>(opcode).offset(), state.instruction_position + 1 });
                patch_points.push({ static_cast<OpCode_JumpNonEmpty const&>(opcode).checkpoint(), state.instruction_position + 2 });
                break;
            case OpCodeId::ForkJump:
                patch_points.push({ static_cast<OpCode_ForkJump const&>(opcode).offset(), state.instruction_position + 1 });
                break;
            case OpCodeId::ForkStay:
                patch_points.push({ static_cast<OpCode_ForkStay const&>(opcode).offset(), state.instruction_position + 1 });
                break;
            case OpCodeId::Repeat:
                patch_points.push({ -(ssize_t) static_cast<OpCode_Repeat const&>(opcode).offset(), state.instruction_position + 1, true });
                break;
            default:
                break;
            }

            while (!patch_points.is_empty()) {
                auto& patch_point = patch_points.top();
                auto target_offset = patch_point.value + state.instruction_position + opcode.size();

                constexpr auto do_patch = [](auto& patch_it, auto& patch_point, auto& target_offset, auto& bytecode, auto ip) {
                    if (patch_it.key() == ip)
                        return;

                    if (patch_point.value < 0 && target_offset <= patch_it.key() && ip > patch_it.key())
                        bytecode[patch_point.offset] += (patch_point.should_negate ? 1 : -1) * (*patch_it);
                    else if (patch_point.value > 0 && target_offset >= patch_it.key() && ip < patch_it.key())
                        bytecode[patch_point.offset] += (patch_point.should_negate ? -1 : 1) * (*patch_it);
                };

                if (auto patch_it = needed_patches.find_largest_not_above_iterator(target_offset); !patch_it.is_end())
                    do_patch(patch_it, patch_point, target_offset, bytecode, state.instruction_position);
                else if (auto patch_it = needed_patches.find_largest_not_above_iterator(state.instruction_position); !patch_it.is_end())
                    do_patch(patch_it, patch_point, target_offset, bytecode, state.instruction_position);

                patch_points.pop();
            }

            state.instruction_position += opcode.size();
        }
    }

    if constexpr (REGEX_DEBUG) {
        warnln("Transformed to:");
        RegexDebug dbg;
        dbg.print_bytecode(*this);
    }
}

void Optimizer::append_alternation(ByteCode& target, ByteCode&& left, ByteCode&& right)
{
    Array<ByteCode, 2> alternatives;
    alternatives[0] = move(left);
    alternatives[1] = move(right);

    append_alternation(target, alternatives);
}

void Optimizer::append_alternation(ByteCode& target, Span<ByteCode> alternatives)
{
    if (alternatives.size() == 0)
        return;

    if (alternatives.size() == 1)
        return target.extend(move(alternatives[0]));

    if (all_of(alternatives, [](auto& x) { return x.is_empty(); }))
        return;

    for (auto& entry : alternatives)
        entry.flatten();

#if REGEX_DEBUG
    ScopeLogger<true> log;
    warnln("Alternations:");
    RegexDebug dbg;
    for (auto& entry : alternatives) {
        warnln("----------");
        dbg.print_bytecode(entry);
    }
    ScopeGuard print_at_end {
        [&] {
            warnln("======================");
            RegexDebug dbg;
            dbg.print_bytecode(target);
        }
    };
#endif

    Vector<Vector<Detail::Block>> basic_blocks;
    basic_blocks.ensure_capacity(alternatives.size());

    for (auto& entry : alternatives)
        basic_blocks.append(Regex<PosixBasicParser>::split_basic_blocks(entry));

    Optional<size_t> left_skip;
    size_t shared_block_count = basic_blocks.first().size();
    for (auto& entry : basic_blocks)
        shared_block_count = min(shared_block_count, entry.size());

    MatchState state;
    for (size_t block_index = 0; block_index < shared_block_count; block_index++) {
        auto& left_block = basic_blocks.first()[block_index];
        auto left_end = block_index + 1 == basic_blocks.first().size() ? left_block.end : basic_blocks.first()[block_index + 1].start;
        auto can_continue = true;
        for (size_t i = 1; i < alternatives.size(); ++i) {
            auto& right_blocks = basic_blocks[i];
            auto& right_block = right_blocks[block_index];
            auto right_end = block_index + 1 == right_blocks.size() ? right_block.end : right_blocks[block_index + 1].start;

            if (left_end - left_block.start != right_end - right_block.start) {
                can_continue = false;
                break;
            }

            if (alternatives[0].spans().slice(left_block.start, left_end - left_block.start) != alternatives[i].spans().slice(right_block.start, right_end - right_block.start)) {
                can_continue = false;
                break;
            }
        }
        if (!can_continue)
            break;

        size_t i = 0;
        for (auto& entry : alternatives) {
            auto& blocks = basic_blocks[i++];
            auto& block = blocks[block_index];
            auto end = block_index + 1 == blocks.size() ? block.end : blocks[block_index + 1].start;
            state.instruction_position = block.start;
            size_t skip = 0;
            while (state.instruction_position < end) {
                auto& opcode = entry.get_opcode(state);
                state.instruction_position += opcode.size();
                skip = state.instruction_position;
            }

            if (left_skip.has_value())
                left_skip = min(skip, *left_skip);
            else
                left_skip = skip;
        }
    }

    // Remove forward jumps as they no longer make sense.
    state.instruction_position = 0;
    for (size_t i = 0; i < left_skip.value_or(0);) {
        auto& opcode = alternatives[0].get_opcode(state);
        switch (opcode.opcode_id()) {
        case OpCodeId::Jump:
        case OpCodeId::ForkJump:
        case OpCodeId::JumpNonEmpty:
        case OpCodeId::ForkStay:
        case OpCodeId::ForkReplaceJump:
        case OpCodeId::ForkReplaceStay:
            if (opcode.argument(0) + opcode.size() > left_skip.value_or(0)) {
                left_skip = i;
                goto break_out;
            }
            break;
        default:
            break;
        }
        i += opcode.size();
    }
break_out:;

    dbgln_if(REGEX_DEBUG, "Skipping {}/{} bytecode entries from {}", left_skip, 0, alternatives[0].size());

    if (left_skip.has_value() && *left_skip > 0) {
        target.extend(alternatives[0].release_slice(basic_blocks.first().first().start, *left_skip));
        auto first = true;
        for (auto& entry : alternatives) {
            if (first) {
                first = false;
                continue;
            }
            entry = entry.release_slice(*left_skip);
        }
    }

    if (all_of(alternatives, [](auto& entry) { return entry.is_empty(); }))
        return;

    size_t patch_start = target.size();
    for (size_t i = 1; i < alternatives.size(); ++i) {
        target.empend(static_cast<ByteCodeValueType>(OpCodeId::ForkJump));
        target.empend(0u); // To be filled later.
    }

    size_t size_to_jump = 0;
    bool seen_one_empty = false;
    for (size_t i = alternatives.size(); i > 0; --i) {
        auto& entry = alternatives[i - 1];
        if (entry.is_empty()) {
            if (seen_one_empty)
                continue;
            seen_one_empty = true;
        }

        auto is_first = i == 1;
        auto instruction_size = entry.size() + (is_first ? 0 : 2); // Jump; -> +2
        size_to_jump += instruction_size;

        if (!is_first)
            target[patch_start + (i - 2) * 2 + 1] = size_to_jump + (alternatives.size() - i) * 2;

        dbgln_if(REGEX_DEBUG, "{} size = {}, cum={}", i - 1, instruction_size, size_to_jump);
    }

    seen_one_empty = false;
    for (size_t i = alternatives.size(); i > 0; --i) {
        auto& chunk = alternatives[i - 1];
        if (chunk.is_empty()) {
            if (seen_one_empty)
                continue;
            seen_one_empty = true;
        }

        ByteCode* previous_chunk = nullptr;
        size_t j = i - 1;
        auto seen_one_empty_before = chunk.is_empty();
        while (j >= 1) {
            --j;
            auto& candidate_chunk = alternatives[j];
            if (candidate_chunk.is_empty()) {
                if (seen_one_empty_before)
                    continue;
            }
            previous_chunk = &candidate_chunk;
            break;
        }

        size_to_jump -= chunk.size() + (previous_chunk ? 2 : 0);

        target.extend(move(chunk));
        target.empend(static_cast<ByteCodeValueType>(OpCodeId::Jump));
        target.empend(size_to_jump); // Jump to the _END label
    }
}

enum class LookupTableInsertionOutcome {
    Successful,
    ReplaceWithAnyChar,
    TemporaryInversionNeeded,
    PermanentInversionNeeded,
    FlushOnInsertion,
    FinishFlushOnInsertion,
    CannotPlaceInTable,
};
static LookupTableInsertionOutcome insert_into_lookup_table(RedBlackTree<ByteCodeValueType, CharRange>& table, CompareTypeAndValuePair pair)
{
    switch (pair.type) {
    case CharacterCompareType::Inverse:
        return LookupTableInsertionOutcome::PermanentInversionNeeded;
    case CharacterCompareType::TemporaryInverse:
        return LookupTableInsertionOutcome::TemporaryInversionNeeded;
    case CharacterCompareType::AnyChar:
        return LookupTableInsertionOutcome::ReplaceWithAnyChar;
    case CharacterCompareType::CharClass:
        return LookupTableInsertionOutcome::CannotPlaceInTable;
    case CharacterCompareType::Char:
        table.insert(pair.value, { (u32)pair.value, (u32)pair.value });
        break;
    case CharacterCompareType::CharRange: {
        CharRange range { pair.value };
        table.insert(range.from, range);
        break;
    }
    case CharacterCompareType::EndAndOr:
        return LookupTableInsertionOutcome::FinishFlushOnInsertion;
    case CharacterCompareType::And:
        return LookupTableInsertionOutcome::FlushOnInsertion;
    case CharacterCompareType::Reference:
    case CharacterCompareType::Property:
    case CharacterCompareType::GeneralCategory:
    case CharacterCompareType::Script:
    case CharacterCompareType::ScriptExtension:
    case CharacterCompareType::Or:
        return LookupTableInsertionOutcome::CannotPlaceInTable;
    case CharacterCompareType::Undefined:
    case CharacterCompareType::RangeExpressionDummy:
    case CharacterCompareType::String:
    case CharacterCompareType::LookupTable:
        VERIFY_NOT_REACHED();
    }

    return LookupTableInsertionOutcome::Successful;
}

void Optimizer::append_character_class(ByteCode& target, Vector<CompareTypeAndValuePair>&& pairs)
{
    ByteCode arguments;
    size_t argument_count = 0;

    if (pairs.size() <= 1) {
        for (auto& pair : pairs) {
            arguments.append(to_underlying(pair.type));
            if (pair.type != CharacterCompareType::AnyChar
                && pair.type != CharacterCompareType::TemporaryInverse
                && pair.type != CharacterCompareType::Inverse
                && pair.type != CharacterCompareType::And
                && pair.type != CharacterCompareType::Or
                && pair.type != CharacterCompareType::EndAndOr)
                arguments.append(pair.value);
            ++argument_count;
        }
    } else {
        RedBlackTree<ByteCodeValueType, CharRange> table;
        RedBlackTree<ByteCodeValueType, CharRange> inverted_table;
        auto* current_table = &table;
        auto* current_inverted_table = &inverted_table;
        bool invert_for_next_iteration = false;
        bool is_currently_inverted = false;

        auto flush_tables = [&] {
            auto append_table = [&](auto& table) {
                ++argument_count;
                arguments.append(to_underlying(CharacterCompareType::LookupTable));
                auto size_index = arguments.size();
                arguments.append(0);
                Optional<CharRange> active_range;
                size_t range_count = 0;
                for (auto& range : table) {
                    if (!active_range.has_value()) {
                        active_range = range;
                        continue;
                    }

                    if (range.from <= active_range->to + 1 && range.to + 1 >= active_range->from) {
                        active_range = CharRange { min(range.from, active_range->from), max(range.to, active_range->to) };
                    } else {
                        ++range_count;
                        arguments.append(active_range.release_value());
                        active_range = range;
                    }
                }
                if (active_range.has_value()) {
                    ++range_count;
                    arguments.append(active_range.release_value());
                }
                arguments[size_index] = range_count;
            };

            auto contains_regular_table = !table.is_empty();
            auto contains_inverted_table = !inverted_table.is_empty();
            if (contains_regular_table)
                append_table(table);

            if (contains_inverted_table) {
                ++argument_count;
                arguments.append(to_underlying(CharacterCompareType::TemporaryInverse));
                append_table(inverted_table);
            }

            table.clear();
            inverted_table.clear();
        };

        auto flush_on_every_insertion = false;
        for (auto& value : pairs) {
            auto should_invert_after_this_iteration = invert_for_next_iteration;
            invert_for_next_iteration = false;

            auto insertion_result = insert_into_lookup_table(*current_table, value);
            switch (insertion_result) {
            case LookupTableInsertionOutcome::Successful:
                if (flush_on_every_insertion)
                    flush_tables();
                break;
            case LookupTableInsertionOutcome::ReplaceWithAnyChar: {
                table.clear();
                inverted_table.clear();
                arguments.append(to_underlying(CharacterCompareType::AnyChar));
                ++argument_count;
                break;
            }
            case LookupTableInsertionOutcome::TemporaryInversionNeeded:
                swap(current_table, current_inverted_table);
                invert_for_next_iteration = true;
                is_currently_inverted = !is_currently_inverted;
                break;
            case LookupTableInsertionOutcome::PermanentInversionNeeded:
                flush_tables();
                arguments.append(to_underlying(CharacterCompareType::Inverse));
                ++argument_count;
                break;
            case LookupTableInsertionOutcome::FlushOnInsertion:
            case LookupTableInsertionOutcome::FinishFlushOnInsertion:
                flush_tables();
                flush_on_every_insertion = insertion_result == LookupTableInsertionOutcome::FlushOnInsertion;
                [[fallthrough]];
            case LookupTableInsertionOutcome::CannotPlaceInTable:
                if (is_currently_inverted) {
                    arguments.append(to_underlying(CharacterCompareType::TemporaryInverse));
                    ++argument_count;
                }
                arguments.append(to_underlying(value.type));

                if (value.type != CharacterCompareType::AnyChar
                    && value.type != CharacterCompareType::TemporaryInverse
                    && value.type != CharacterCompareType::Inverse
                    && value.type != CharacterCompareType::And
                    && value.type != CharacterCompareType::Or
                    && value.type != CharacterCompareType::EndAndOr)
                    arguments.append(value.value);
                ++argument_count;
                break;
            }

            if (should_invert_after_this_iteration) {
                swap(current_table, current_inverted_table);
                is_currently_inverted = !is_currently_inverted;
            }
        }

        flush_tables();
    }

    target.empend(static_cast<ByteCodeValueType>(OpCodeId::Compare));
    target.empend(argument_count);   // number of arguments
    target.empend(arguments.size()); // size of arguments
    target.extend(move(arguments));
}

template void Regex<PosixBasicParser>::run_optimization_passes();
template void Regex<PosixExtendedParser>::run_optimization_passes();
template void Regex<ECMA262Parser>::run_optimization_passes();
}
