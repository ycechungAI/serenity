/*
 * Copyright (c) 2021, Andreas Kling <kling@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/OwnPtr.h>
#include <AK/SinglyLinkedList.h>
#include <LibJS/Bytecode/BasicBlock.h>
#include <LibJS/Bytecode/CodeGenerationError.h>
#include <LibJS/Bytecode/Executable.h>
#include <LibJS/Bytecode/IdentifierTable.h>
#include <LibJS/Bytecode/Label.h>
#include <LibJS/Bytecode/Op.h>
#include <LibJS/Bytecode/Register.h>
#include <LibJS/Bytecode/StringTable.h>
#include <LibJS/Forward.h>
#include <LibJS/Runtime/FunctionKind.h>

namespace JS::Bytecode {

class Generator {
public:
    enum class SurroundingScopeKind {
        Global,
        Function,
        Block,
    };
    static CodeGenerationErrorOr<NonnullOwnPtr<Executable>> generate(ASTNode const&, FunctionKind = FunctionKind::Normal);

    Register allocate_register();

    void ensure_enough_space(size_t size)
    {
        // Make sure there's always enough space for a single jump at the end.
        if (!m_current_basic_block->can_grow(size + sizeof(Op::Jump))) {
            auto& new_block = make_block();
            emit<Op::Jump>().set_targets(
                Label { new_block },
                {});
            switch_to_basic_block(new_block);
        }
    }

    template<typename OpType, typename... Args>
    OpType& emit(Args&&... args)
    {
        VERIFY(!is_current_block_terminated());
        // If the block doesn't have enough space, switch to another block
        if constexpr (!OpType::IsTerminator)
            ensure_enough_space(sizeof(OpType));

        void* slot = next_slot();
        grow(sizeof(OpType));
        new (slot) OpType(forward<Args>(args)...);
        if constexpr (OpType::IsTerminator)
            m_current_basic_block->terminate({}, static_cast<Instruction const*>(slot));
        return *static_cast<OpType*>(slot);
    }

    template<typename OpType, typename... Args>
    OpType& emit_with_extra_register_slots(size_t extra_register_slots, Args&&... args)
    {
        VERIFY(!is_current_block_terminated());

        size_t size_to_allocate = round_up_to_power_of_two(sizeof(OpType) + extra_register_slots * sizeof(Register), alignof(void*));

        // If the block doesn't have enough space, switch to another block
        if constexpr (!OpType::IsTerminator)
            ensure_enough_space(size_to_allocate);

        void* slot = next_slot();
        grow(size_to_allocate);
        new (slot) OpType(forward<Args>(args)...);
        if constexpr (OpType::IsTerminator)
            m_current_basic_block->terminate({}, static_cast<Instruction const*>(slot));
        return *static_cast<OpType*>(slot);
    }

    CodeGenerationErrorOr<void> emit_load_from_reference(JS::ASTNode const&);
    CodeGenerationErrorOr<void> emit_store_to_reference(JS::ASTNode const&);
    CodeGenerationErrorOr<void> emit_delete_reference(JS::ASTNode const&);

    void push_home_object(Register);
    void pop_home_object();
    void emit_new_function(JS::FunctionNode const&);

    void begin_continuable_scope(Label continue_target, Vector<DeprecatedFlyString> const& language_label_set);
    void end_continuable_scope();
    void begin_breakable_scope(Label breakable_target, Vector<DeprecatedFlyString> const& language_label_set);
    void end_breakable_scope();

    [[nodiscard]] Label nearest_continuable_scope() const;
    [[nodiscard]] Label nearest_breakable_scope() const;

    void switch_to_basic_block(BasicBlock& block)
    {
        m_current_basic_block = &block;
    }

    [[nodiscard]] BasicBlock& current_block() { return *m_current_basic_block; }

    BasicBlock& make_block(DeprecatedString name = {})
    {
        if (name.is_empty())
            name = DeprecatedString::number(m_next_block++);
        m_root_basic_blocks.append(BasicBlock::create(name));
        return *m_root_basic_blocks.last();
    }

    bool is_current_block_terminated() const
    {
        return m_current_basic_block->is_terminated();
    }

    StringTableIndex intern_string(DeprecatedString string)
    {
        return m_string_table->insert(move(string));
    }

    IdentifierTableIndex intern_identifier(DeprecatedFlyString string)
    {
        return m_identifier_table->insert(move(string));
    }

    bool is_in_generator_or_async_function() const { return m_enclosing_function_kind == FunctionKind::Async || m_enclosing_function_kind == FunctionKind::Generator; }
    bool is_in_generator_function() const { return m_enclosing_function_kind == FunctionKind::Generator; }
    bool is_in_async_function() const { return m_enclosing_function_kind == FunctionKind::Async; }

    enum class BindingMode {
        Lexical,
        Var,
        Global,
    };
    struct LexicalScope {
        SurroundingScopeKind kind;
    };

    void block_declaration_instantiation(ScopeNode const&);

    void begin_variable_scope();
    void end_variable_scope();

    enum class BlockBoundaryType {
        Break,
        Continue,
        Unwind,
        ReturnToFinally,
        LeaveLexicalEnvironment,
    };
    template<typename OpType>
    void perform_needed_unwinds()
    requires(OpType::IsTerminator && !IsSame<OpType, Op::Jump>)
    {
        for (size_t i = m_boundaries.size(); i > 0; --i) {
            auto boundary = m_boundaries[i - 1];
            using enum BlockBoundaryType;
            switch (boundary) {
            case Unwind:
                if constexpr (IsSame<OpType, Bytecode::Op::Throw>)
                    return;
                emit<Bytecode::Op::LeaveUnwindContext>();
                break;
            case LeaveLexicalEnvironment:
                emit<Bytecode::Op::LeaveEnvironment>(Bytecode::Op::EnvironmentMode::Lexical);
                break;
            case Break:
            case Continue:
                break;
            case ReturnToFinally:
                return;
            };
        }
    }

    void generate_break();
    void generate_break(DeprecatedFlyString const& break_label);

    void generate_continue();
    void generate_continue(DeprecatedFlyString const& continue_label);

    void start_boundary(BlockBoundaryType type) { m_boundaries.append(type); }
    void end_boundary(BlockBoundaryType type)
    {
        VERIFY(m_boundaries.last() == type);
        m_boundaries.take_last();
    }

private:
    Generator();
    ~Generator() = default;

    void grow(size_t);
    void* next_slot();

    struct LabelableScope {
        Label bytecode_target;
        Vector<DeprecatedFlyString> language_label_set;
    };

    BasicBlock* m_current_basic_block { nullptr };
    Vector<NonnullOwnPtr<BasicBlock>> m_root_basic_blocks;
    NonnullOwnPtr<StringTable> m_string_table;
    NonnullOwnPtr<IdentifierTable> m_identifier_table;

    u32 m_next_register { 2 };
    u32 m_next_block { 1 };
    FunctionKind m_enclosing_function_kind { FunctionKind::Normal };
    Vector<LabelableScope> m_continuable_scopes;
    Vector<LabelableScope> m_breakable_scopes;
    Vector<BlockBoundaryType> m_boundaries;
    Vector<Register> m_home_objects;
};

}
