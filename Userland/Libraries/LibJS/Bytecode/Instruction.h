/*
 * Copyright (c) 2021, Andreas Kling <kling@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Forward.h>
#include <LibJS/Forward.h>

#define ENUMERATE_BYTECODE_OPS(O)    \
    O(Add)                           \
    O(BitwiseAnd)                    \
    O(BitwiseNot)                    \
    O(BitwiseOr)                     \
    O(BitwiseXor)                    \
    O(Call)                          \
    O(ConcatString)                  \
    O(ContinuePendingUnwind)         \
    O(CopyObjectExcludingProperties) \
    O(Decrement)                     \
    O(Div)                           \
    O(EnterUnwindContext)            \
    O(Exp)                           \
    O(GetById)                       \
    O(GetByValue)                    \
    O(GetIterator)                   \
    O(GetVariable)                   \
    O(GreaterThan)                   \
    O(GreaterThanEquals)             \
    O(In)                            \
    O(Increment)                     \
    O(InstanceOf)                    \
    O(IteratorNext)                  \
    O(IteratorResultDone)            \
    O(IteratorResultValue)           \
    O(IteratorToArray)               \
    O(Jump)                          \
    O(JumpConditional)               \
    O(JumpNullish)                   \
    O(JumpUndefined)                 \
    O(LeaveUnwindContext)            \
    O(LeftShift)                     \
    O(LessThan)                      \
    O(LessThanEquals)                \
    O(Load)                          \
    O(LoadImmediate)                 \
    O(LooselyEquals)                 \
    O(LooselyInequals)               \
    O(Mod)                           \
    O(Mul)                           \
    O(NewArray)                      \
    O(NewBigInt)                     \
    O(NewClass)                      \
    O(NewFunction)                   \
    O(NewObject)                     \
    O(NewRegExp)                     \
    O(NewString)                     \
    O(Not)                           \
    O(PushDeclarativeEnvironment)    \
    O(PutById)                       \
    O(PutByValue)                    \
    O(ResolveThisBinding)            \
    O(Return)                        \
    O(RightShift)                    \
    O(SetVariable)                   \
    O(Store)                         \
    O(StrictlyEquals)                \
    O(StrictlyInequals)              \
    O(Sub)                           \
    O(Throw)                         \
    O(Typeof)                        \
    O(UnaryMinus)                    \
    O(UnaryPlus)                     \
    O(UnsignedRightShift)            \
    O(Yield)

namespace JS::Bytecode {

class Instruction {
public:
    constexpr static bool IsTerminator = false;

    enum class Type {
#define __BYTECODE_OP(op) \
    op,
        ENUMERATE_BYTECODE_OPS(__BYTECODE_OP)
#undef __BYTECODE_OP
    };

    bool is_terminator() const;
    Type type() const { return m_type; }
    size_t length() const;
    String to_string(Bytecode::Executable const&) const;
    void execute(Bytecode::Interpreter&) const;
    void replace_references(BasicBlock const&, BasicBlock const&);
    static void destroy(Instruction&);

protected:
    explicit Instruction(Type type)
        : m_type(type)
    {
    }

private:
    Type m_type {};
};
}
