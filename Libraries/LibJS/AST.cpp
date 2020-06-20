/*
 * Copyright (c) 2020, Andreas Kling <kling@serenityos.org>
 * Copyright (c) 2020, Linus Groh <mail@linusgroh.de>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <AK/Function.h>
#include <AK/HashMap.h>
#include <AK/ScopeGuard.h>
#include <AK/StringBuilder.h>
#include <LibCrypto/BigInt/SignedBigInteger.h>
#include <LibJS/AST.h>
#include <LibJS/Interpreter.h>
#include <LibJS/Runtime/Accessor.h>
#include <LibJS/Runtime/Array.h>
#include <LibJS/Runtime/BigInt.h>
#include <LibJS/Runtime/Error.h>
#include <LibJS/Runtime/GlobalObject.h>
#include <LibJS/Runtime/MarkedValueList.h>
#include <LibJS/Runtime/NativeFunction.h>
#include <LibJS/Runtime/PrimitiveString.h>
#include <LibJS/Runtime/Reference.h>
#include <LibJS/Runtime/RegExpObject.h>
#include <LibJS/Runtime/ScriptFunction.h>
#include <LibJS/Runtime/Shape.h>
#include <LibJS/Runtime/StringObject.h>
#include <stdio.h>

namespace JS {

static void update_function_name(Value& value, const FlyString& name)
{
    if (!value.is_object())
        return;
    auto& object = value.as_object();
    if (object.is_function()) {
        auto& function = static_cast<Function&>(object);
        if (function.is_script_function() && function.name().is_empty())
            static_cast<ScriptFunction&>(function).set_name(name);
    } else if (object.is_array()) {
        auto& array = static_cast<Array&>(object);
        for (auto& entry : array.indexed_properties().values_unordered())
            update_function_name(entry.value, name);
    }
}

Value ScopeNode::execute(Interpreter& interpreter, GlobalObject& global_object) const
{
    return interpreter.run(global_object, *this);
}

Value FunctionDeclaration::execute(Interpreter&, GlobalObject&) const
{
    return js_undefined();
}

Value FunctionExpression::execute(Interpreter& interpreter, GlobalObject& global_object) const
{
    return ScriptFunction::create(global_object, name(), body(), parameters(), function_length(), interpreter.current_environment(), m_is_arrow_function);
}

Value ExpressionStatement::execute(Interpreter& interpreter, GlobalObject& global_object) const
{
    return m_expression->execute(interpreter, global_object);
}

CallExpression::ThisAndCallee CallExpression::compute_this_and_callee(Interpreter& interpreter, GlobalObject& global_object) const
{
    if (is_new_expression()) {
        // Computing |this| is irrelevant for "new" expression.
        return { js_undefined(), m_callee->execute(interpreter, global_object) };
    }

    if (m_callee->is_member_expression()) {
        auto& member_expression = static_cast<const MemberExpression&>(*m_callee);
        auto object_value = member_expression.object().execute(interpreter, global_object);
        if (interpreter.exception())
            return {};
        auto* this_value = object_value.to_object(interpreter, global_object);
        if (interpreter.exception())
            return {};
        auto callee = this_value->get(member_expression.computed_property_name(interpreter, global_object)).value_or(js_undefined());
        return { this_value, callee };
    }
    return { &global_object, m_callee->execute(interpreter, global_object) };
}

Value CallExpression::execute(Interpreter& interpreter, GlobalObject& global_object) const
{
    auto [this_value, callee] = compute_this_and_callee(interpreter, global_object);
    if (interpreter.exception())
        return {};

    ASSERT(!callee.is_empty());

    if (!callee.is_function()
        || (is_new_expression() && (callee.as_object().is_native_function() && !static_cast<NativeFunction&>(callee.as_object()).has_constructor()))) {
        String error_message;
        auto call_type = is_new_expression() ? "constructor" : "function";
        if (m_callee->is_identifier() || m_callee->is_member_expression()) {
            String expression_string;
            if (m_callee->is_identifier()) {
                expression_string = static_cast<const Identifier&>(*m_callee).string();
            } else {
                expression_string = static_cast<const MemberExpression&>(*m_callee).to_string_approximation();
            }
            return interpreter.throw_exception<TypeError>(ErrorType::IsNotAEvaluatedFrom, callee.to_string_without_side_effects().characters(), call_type, expression_string.characters());
        } else {
            return interpreter.throw_exception<TypeError>(ErrorType::IsNotA, callee.to_string_without_side_effects().characters(), call_type);
        }
    }

    auto& function = callee.as_function();

    MarkedValueList arguments(interpreter.heap());
    arguments.values().append(function.bound_arguments());

    for (size_t i = 0; i < m_arguments.size(); ++i) {
        auto value = m_arguments[i].value->execute(interpreter, global_object);
        if (interpreter.exception())
            return {};
        if (m_arguments[i].is_spread) {
            // FIXME: Support generic iterables
            if (value.is_string()) {
                for (auto ch : value.as_string().string())
                    arguments.append(Value(js_string(interpreter, String::format("%c", ch))));
            } else if (value.is_object() && value.as_object().is_array()) {
                auto& array = static_cast<Array&>(value.as_object());
                for (auto& entry : array.indexed_properties()) {
                    arguments.append(entry.value_and_attributes(&array).value);
                    if (interpreter.exception())
                        return {};
                }
            } else if (value.is_object() && value.as_object().is_string_object()) {
                for (auto ch : static_cast<const StringObject&>(value.as_object()).primitive_string().string())
                    arguments.append(Value(js_string(interpreter, String::format("%c", ch))));
            } else {
                interpreter.throw_exception<TypeError>(ErrorType::NotIterable, value.to_string_without_side_effects().characters());
            }
        } else {
            arguments.append(value);
        }
    }

    auto& call_frame = interpreter.push_call_frame();
    call_frame.function_name = function.name();
    call_frame.arguments = arguments.values();
    call_frame.environment = function.create_environment();

    Object* new_object = nullptr;
    Value result;
    if (is_new_expression()) {
        new_object = Object::create_empty(interpreter, global_object);
        auto prototype = function.get("prototype");
        if (interpreter.exception())
            return {};
        if (prototype.is_object()) {
            new_object->set_prototype(&prototype.as_object());
            if (interpreter.exception())
                return {};
        }
        call_frame.this_value = new_object;
        result = function.construct(interpreter);
    } else {
        call_frame.this_value = function.bound_this().value_or(this_value);
        result = function.call(interpreter);
    }

    interpreter.pop_call_frame();

    if (interpreter.exception())
        return {};

    if (is_new_expression()) {
        if (result.is_object())
            return result;
        return new_object;
    }
    return result;
}

Value ReturnStatement::execute(Interpreter& interpreter, GlobalObject& global_object) const
{
    auto value = argument() ? argument()->execute(interpreter, global_object) : js_undefined();
    if (interpreter.exception())
        return {};
    interpreter.unwind(ScopeType::Function);
    return value;
}

Value IfStatement::execute(Interpreter& interpreter, GlobalObject& global_object) const
{
    auto predicate_result = m_predicate->execute(interpreter, global_object);
    if (interpreter.exception())
        return {};

    if (predicate_result.to_boolean())
        return interpreter.run(global_object, *m_consequent);

    if (m_alternate)
        return interpreter.run(global_object, *m_alternate);

    return js_undefined();
}

Value WhileStatement::execute(Interpreter& interpreter, GlobalObject& global_object) const
{
    Value last_value = js_undefined();
    while (m_test->execute(interpreter, global_object).to_boolean()) {
        if (interpreter.exception())
            return {};
        last_value = interpreter.run(global_object, *m_body);
        if (interpreter.exception())
            return {};
    }

    return last_value;
}

Value DoWhileStatement::execute(Interpreter& interpreter, GlobalObject& global_object) const
{
    Value last_value = js_undefined();
    do {
        if (interpreter.exception())
            return {};
        last_value = interpreter.run(global_object, *m_body);
        if (interpreter.exception())
            return {};
    } while (m_test->execute(interpreter, global_object).to_boolean());

    return last_value;
}

Value ForStatement::execute(Interpreter& interpreter, GlobalObject& global_object) const
{
    RefPtr<BlockStatement> wrapper;

    if (m_init && m_init->is_variable_declaration() && static_cast<const VariableDeclaration*>(m_init.ptr())->declaration_kind() != DeclarationKind::Var) {
        wrapper = create_ast_node<BlockStatement>();
        NonnullRefPtrVector<VariableDeclaration> decls;
        decls.append(*static_cast<const VariableDeclaration*>(m_init.ptr()));
        wrapper->add_variables(decls);
        interpreter.enter_scope(*wrapper, {}, ScopeType::Block, global_object);
    }

    auto wrapper_cleanup = ScopeGuard([&] {
        if (wrapper)
            interpreter.exit_scope(*wrapper);
    });

    Value last_value = js_undefined();

    if (m_init) {
        m_init->execute(interpreter, global_object);
        if (interpreter.exception())
            return {};
    }

    if (m_test) {
        while (true) {
            auto test_result = m_test->execute(interpreter, global_object);
            if (interpreter.exception())
                return {};
            if (!test_result.to_boolean())
                break;
            last_value = interpreter.run(global_object, *m_body);
            if (interpreter.exception())
                return {};
            if (interpreter.should_unwind()) {
                if (interpreter.should_unwind_until(ScopeType::Continuable, m_label)) {
                    interpreter.stop_unwind();
                } else if (interpreter.should_unwind_until(ScopeType::Breakable, m_label)) {
                    interpreter.stop_unwind();
                    break;
                } else {
                    return js_undefined();
                }
            }
            if (m_update) {
                m_update->execute(interpreter, global_object);
                if (interpreter.exception())
                    return {};
            }
        }
    } else {
        while (true) {
            last_value = interpreter.run(global_object, *m_body);
            if (interpreter.exception())
                return {};
            if (interpreter.should_unwind()) {
                if (interpreter.should_unwind_until(ScopeType::Continuable, m_label)) {
                    interpreter.stop_unwind();
                } else if (interpreter.should_unwind_until(ScopeType::Breakable, m_label)) {
                    interpreter.stop_unwind();
                    break;
                } else {
                    return js_undefined();
                }
            }
            if (m_update) {
                m_update->execute(interpreter, global_object);
                if (interpreter.exception())
                    return {};
            }
        }
    }

    return last_value;
}

static FlyString variable_from_for_declaration(Interpreter& interpreter, GlobalObject& global_object, NonnullRefPtr<ASTNode> node, RefPtr<BlockStatement> wrapper)
{
    FlyString variable_name;
    if (node->is_variable_declaration()) {
        auto* variable_declaration = static_cast<const VariableDeclaration*>(node.ptr());
        ASSERT(!variable_declaration->declarations().is_empty());
        if (variable_declaration->declaration_kind() != DeclarationKind::Var) {
            wrapper = create_ast_node<BlockStatement>();
            interpreter.enter_scope(*wrapper, {}, ScopeType::Block, global_object);
        }
        variable_declaration->execute(interpreter, global_object);
        variable_name = variable_declaration->declarations().first().id().string();
    } else if (node->is_identifier()) {
        variable_name = static_cast<const Identifier&>(*node).string();
    } else {
        ASSERT_NOT_REACHED();
    }
    return variable_name;
}

Value ForInStatement::execute(Interpreter& interpreter, GlobalObject& global_object) const
{
    if (!m_lhs->is_variable_declaration() && !m_lhs->is_identifier()) {
        // FIXME: Implement "for (foo.bar in baz)", "for (foo[0] in bar)"
        ASSERT_NOT_REACHED();
    }
    RefPtr<BlockStatement> wrapper;
    auto variable_name = variable_from_for_declaration(interpreter, global_object, m_lhs, wrapper);
    auto wrapper_cleanup = ScopeGuard([&] {
        if (wrapper)
            interpreter.exit_scope(*wrapper);
    });
    auto last_value = js_undefined();
    auto rhs_result = m_rhs->execute(interpreter, global_object);
    if (interpreter.exception())
        return {};
    auto* object = rhs_result.to_object(interpreter, global_object);
    while (object) {
        auto property_names = object->get_own_properties(*object, Object::GetOwnPropertyMode::Key, true);
        for (auto& property_name : property_names.as_object().indexed_properties()) {
            interpreter.set_variable(variable_name, property_name.value_and_attributes(object).value, global_object);
            if (interpreter.exception())
                return {};
            last_value = interpreter.run(global_object, *m_body);
            if (interpreter.exception())
                return {};
            if (interpreter.should_unwind()) {
                if (interpreter.should_unwind_until(ScopeType::Continuable, m_label)) {
                    interpreter.stop_unwind();
                } else if (interpreter.should_unwind_until(ScopeType::Breakable, m_label)) {
                    interpreter.stop_unwind();
                    break;
                } else {
                    return js_undefined();
                }
            }
        }
        object = object->prototype();
        if (interpreter.exception())
            return {};
    }
    return last_value;
}

Value ForOfStatement::execute(Interpreter& interpreter, GlobalObject& global_object) const
{
    if (!m_lhs->is_variable_declaration() && !m_lhs->is_identifier()) {
        // FIXME: Implement "for (foo.bar of baz)", "for (foo[0] of bar)"
        ASSERT_NOT_REACHED();
    }
    RefPtr<BlockStatement> wrapper;
    auto variable_name = variable_from_for_declaration(interpreter, global_object, m_lhs, wrapper);
    auto wrapper_cleanup = ScopeGuard([&] {
        if (wrapper)
            interpreter.exit_scope(*wrapper);
    });
    auto last_value = js_undefined();
    auto rhs_result = m_rhs->execute(interpreter, global_object);
    if (interpreter.exception())
        return {};
    // FIXME: We need to properly implement the iterator protocol
    auto is_iterable = rhs_result.is_array() || rhs_result.is_string() || (rhs_result.is_object() && rhs_result.as_object().is_string_object());
    if (!is_iterable)
        return interpreter.throw_exception<TypeError>(ErrorType::ForOfNotIterable);

    size_t index = 0;
    auto next = [&]() -> Optional<Value> {
        if (rhs_result.is_array()) {
            auto& array_elements = rhs_result.as_object().indexed_properties();
            if (index < array_elements.array_like_size()) {
                auto result = array_elements.get(&rhs_result.as_object(), index);
                if (interpreter.exception())
                    return {};
                return result.value().value;
            }
        } else if (rhs_result.is_string()) {
            auto string = rhs_result.as_string().string();
            if (index < string.length())
                return js_string(interpreter, string.substring(index, 1));
        } else if (rhs_result.is_object() && rhs_result.as_object().is_string_object()) {
            auto string = static_cast<StringObject*>(&rhs_result.as_object())->primitive_string().string();
            if (index < string.length())
                return js_string(interpreter, string.substring(index, 1));
        }
        return {};
    };

    for (;;) {
        auto next_item = next();
        if (!next_item.has_value())
            break;
        interpreter.set_variable(variable_name, next_item.value(), global_object);
        last_value = interpreter.run(global_object, *m_body);
        if (interpreter.exception())
            return {};
        if (interpreter.should_unwind()) {
            if (interpreter.should_unwind_until(ScopeType::Continuable, m_label)) {
                interpreter.stop_unwind();
            } else if (interpreter.should_unwind_until(ScopeType::Breakable, m_label)) {
                interpreter.stop_unwind();
                break;
            } else {
                return js_undefined();
            }
        }
        ++index;
    }
    return last_value;
}

Value BinaryExpression::execute(Interpreter& interpreter, GlobalObject& global_object) const
{
    auto lhs_result = m_lhs->execute(interpreter, global_object);
    if (interpreter.exception())
        return {};
    auto rhs_result = m_rhs->execute(interpreter, global_object);
    if (interpreter.exception())
        return {};

    switch (m_op) {
    case BinaryOp::Addition:
        return add(interpreter, lhs_result, rhs_result);
    case BinaryOp::Subtraction:
        return sub(interpreter, lhs_result, rhs_result);
    case BinaryOp::Multiplication:
        return mul(interpreter, lhs_result, rhs_result);
    case BinaryOp::Division:
        return div(interpreter, lhs_result, rhs_result);
    case BinaryOp::Modulo:
        return mod(interpreter, lhs_result, rhs_result);
    case BinaryOp::Exponentiation:
        return exp(interpreter, lhs_result, rhs_result);
    case BinaryOp::TypedEquals:
        return Value(strict_eq(interpreter, lhs_result, rhs_result));
    case BinaryOp::TypedInequals:
        return Value(!strict_eq(interpreter, lhs_result, rhs_result));
    case BinaryOp::AbstractEquals:
        return Value(abstract_eq(interpreter, lhs_result, rhs_result));
    case BinaryOp::AbstractInequals:
        return Value(!abstract_eq(interpreter, lhs_result, rhs_result));
    case BinaryOp::GreaterThan:
        return greater_than(interpreter, lhs_result, rhs_result);
    case BinaryOp::GreaterThanEquals:
        return greater_than_equals(interpreter, lhs_result, rhs_result);
    case BinaryOp::LessThan:
        return less_than(interpreter, lhs_result, rhs_result);
    case BinaryOp::LessThanEquals:
        return less_than_equals(interpreter, lhs_result, rhs_result);
    case BinaryOp::BitwiseAnd:
        return bitwise_and(interpreter, lhs_result, rhs_result);
    case BinaryOp::BitwiseOr:
        return bitwise_or(interpreter, lhs_result, rhs_result);
    case BinaryOp::BitwiseXor:
        return bitwise_xor(interpreter, lhs_result, rhs_result);
    case BinaryOp::LeftShift:
        return left_shift(interpreter, lhs_result, rhs_result);
    case BinaryOp::RightShift:
        return right_shift(interpreter, lhs_result, rhs_result);
    case BinaryOp::UnsignedRightShift:
        return unsigned_right_shift(interpreter, lhs_result, rhs_result);
    case BinaryOp::In:
        return in(interpreter, lhs_result, rhs_result);
    case BinaryOp::InstanceOf:
        return instance_of(interpreter, lhs_result, rhs_result);
    }

    ASSERT_NOT_REACHED();
}

Value LogicalExpression::execute(Interpreter& interpreter, GlobalObject& global_object) const
{
    auto lhs_result = m_lhs->execute(interpreter, global_object);
    if (interpreter.exception())
        return {};

    switch (m_op) {
    case LogicalOp::And:
        if (lhs_result.to_boolean()) {
            auto rhs_result = m_rhs->execute(interpreter, global_object);
            if (interpreter.exception())
                return {};
            return rhs_result;
        }
        return lhs_result;
    case LogicalOp::Or: {
        if (lhs_result.to_boolean())
            return lhs_result;
        auto rhs_result = m_rhs->execute(interpreter, global_object);
        if (interpreter.exception())
            return {};
        return rhs_result;
    }
    case LogicalOp::NullishCoalescing:
        if (lhs_result.is_null() || lhs_result.is_undefined()) {
            auto rhs_result = m_rhs->execute(interpreter, global_object);
            if (interpreter.exception())
                return {};
            return rhs_result;
        }
        return lhs_result;
    }

    ASSERT_NOT_REACHED();
}

Reference Expression::to_reference(Interpreter&, GlobalObject&) const
{
    return {};
}

Reference Identifier::to_reference(Interpreter& interpreter, GlobalObject&) const
{
    return interpreter.get_reference(string());
}

Reference MemberExpression::to_reference(Interpreter& interpreter, GlobalObject& global_object) const
{
    auto object_value = m_object->execute(interpreter, global_object);
    if (interpreter.exception())
        return {};
    auto property_name = computed_property_name(interpreter, global_object);
    if (!property_name.is_valid())
        return {};
    return { object_value, property_name };
}

Value UnaryExpression::execute(Interpreter& interpreter, GlobalObject& global_object) const
{
    if (m_op == UnaryOp::Delete) {
        auto reference = m_lhs->to_reference(interpreter, global_object);
        if (interpreter.exception())
            return {};
        if (reference.is_unresolvable())
            return Value(true);
        // FIXME: Support deleting locals
        ASSERT(!reference.is_local_variable());
        if (reference.is_global_variable())
            return global_object.delete_property(reference.name());
        auto* base_object = reference.base().to_object(interpreter, global_object);
        if (!base_object)
            return {};
        return base_object->delete_property(reference.name());
    }

    Value lhs_result;
    if (m_op == UnaryOp::Typeof && m_lhs->is_identifier()) {
        auto reference = m_lhs->to_reference(interpreter, global_object);
        if (interpreter.exception()) {
            return {};
        }
        // FIXME: standard recommends checking with is_unresolvable but it ALWAYS return false here
        if (reference.is_local_variable() || reference.is_global_variable()) {
            auto name = reference.name();
            lhs_result = interpreter.get_variable(name.to_string(), global_object).value_or(js_undefined());
            if (interpreter.exception())
                return {};
        }
    } else {
        lhs_result = m_lhs->execute(interpreter, global_object);
        if (interpreter.exception())
            return {};
    }

    switch (m_op) {
    case UnaryOp::BitwiseNot:
        return bitwise_not(interpreter, lhs_result);
    case UnaryOp::Not:
        return Value(!lhs_result.to_boolean());
    case UnaryOp::Plus:
        return unary_plus(interpreter, lhs_result);
    case UnaryOp::Minus:
        return unary_minus(interpreter, lhs_result);
    case UnaryOp::Typeof:
        switch (lhs_result.type()) {
        case Value::Type::Empty:
            ASSERT_NOT_REACHED();
            return {};
        case Value::Type::Undefined:
            return js_string(interpreter, "undefined");
        case Value::Type::Null:
            // yes, this is on purpose. yes, this is how javascript works.
            // yes, it's silly.
            return js_string(interpreter, "object");
        case Value::Type::Number:
            return js_string(interpreter, "number");
        case Value::Type::String:
            return js_string(interpreter, "string");
        case Value::Type::Object:
            if (lhs_result.is_function())
                return js_string(interpreter, "function");
            return js_string(interpreter, "object");
        case Value::Type::Boolean:
            return js_string(interpreter, "boolean");
        case Value::Type::Symbol:
            return js_string(interpreter, "symbol");
        case Value::Type::BigInt:
            return js_string(interpreter, "bigint");
        default:
            ASSERT_NOT_REACHED();
        }
    case UnaryOp::Void:
        return js_undefined();
    case UnaryOp::Delete:
        ASSERT_NOT_REACHED();
    }

    ASSERT_NOT_REACHED();
}

static void print_indent(int indent)
{
    for (int i = 0; i < indent * 2; ++i)
        putchar(' ');
}

void ASTNode::dump(int indent) const
{
    print_indent(indent);
    printf("%s\n", class_name());
}

void ScopeNode::dump(int indent) const
{
    ASTNode::dump(indent);
    if (!m_variables.is_empty()) {
        print_indent(indent + 1);
        printf("(Variables)\n");
        for (auto& variable : m_variables)
            variable.dump(indent + 2);
    }
    if (!m_children.is_empty()) {
        print_indent(indent + 1);
        printf("(Children)\n");
        for (auto& child : children())
            child.dump(indent + 2);
    }
}

void BinaryExpression::dump(int indent) const
{
    const char* op_string = nullptr;
    switch (m_op) {
    case BinaryOp::Addition:
        op_string = "+";
        break;
    case BinaryOp::Subtraction:
        op_string = "-";
        break;
    case BinaryOp::Multiplication:
        op_string = "*";
        break;
    case BinaryOp::Division:
        op_string = "/";
        break;
    case BinaryOp::Modulo:
        op_string = "%";
        break;
    case BinaryOp::Exponentiation:
        op_string = "**";
        break;
    case BinaryOp::TypedEquals:
        op_string = "===";
        break;
    case BinaryOp::TypedInequals:
        op_string = "!==";
        break;
    case BinaryOp::AbstractEquals:
        op_string = "==";
        break;
    case BinaryOp::AbstractInequals:
        op_string = "!=";
        break;
    case BinaryOp::GreaterThan:
        op_string = ">";
        break;
    case BinaryOp::GreaterThanEquals:
        op_string = ">=";
        break;
    case BinaryOp::LessThan:
        op_string = "<";
        break;
    case BinaryOp::LessThanEquals:
        op_string = "<=";
        break;
    case BinaryOp::BitwiseAnd:
        op_string = "&";
        break;
    case BinaryOp::BitwiseOr:
        op_string = "|";
        break;
    case BinaryOp::BitwiseXor:
        op_string = "^";
        break;
    case BinaryOp::LeftShift:
        op_string = "<<";
        break;
    case BinaryOp::RightShift:
        op_string = ">>";
        break;
    case BinaryOp::UnsignedRightShift:
        op_string = ">>>";
        break;
    case BinaryOp::In:
        op_string = "in";
        break;
    case BinaryOp::InstanceOf:
        op_string = "instanceof";
        break;
    }

    print_indent(indent);
    printf("%s\n", class_name());
    m_lhs->dump(indent + 1);
    print_indent(indent + 1);
    printf("%s\n", op_string);
    m_rhs->dump(indent + 1);
}

void LogicalExpression::dump(int indent) const
{
    const char* op_string = nullptr;
    switch (m_op) {
    case LogicalOp::And:
        op_string = "&&";
        break;
    case LogicalOp::Or:
        op_string = "||";
        break;
    case LogicalOp::NullishCoalescing:
        op_string = "??";
        break;
    }

    print_indent(indent);
    printf("%s\n", class_name());
    m_lhs->dump(indent + 1);
    print_indent(indent + 1);
    printf("%s\n", op_string);
    m_rhs->dump(indent + 1);
}

void UnaryExpression::dump(int indent) const
{
    const char* op_string = nullptr;
    switch (m_op) {
    case UnaryOp::BitwiseNot:
        op_string = "~";
        break;
    case UnaryOp::Not:
        op_string = "!";
        break;
    case UnaryOp::Plus:
        op_string = "+";
        break;
    case UnaryOp::Minus:
        op_string = "-";
        break;
    case UnaryOp::Typeof:
        op_string = "typeof ";
        break;
    case UnaryOp::Void:
        op_string = "void ";
        break;
    case UnaryOp::Delete:
        op_string = "delete ";
        break;
    }

    print_indent(indent);
    printf("%s\n", class_name());
    print_indent(indent + 1);
    printf("%s\n", op_string);
    m_lhs->dump(indent + 1);
}

void CallExpression::dump(int indent) const
{
    print_indent(indent);
    printf("CallExpression %s\n", is_new_expression() ? "[new]" : "");
    m_callee->dump(indent + 1);
    for (auto& argument : m_arguments)
        argument.value->dump(indent + 1);
}

void StringLiteral::dump(int indent) const
{
    print_indent(indent);
    printf("StringLiteral \"%s\"\n", m_value.characters());
}

void NumericLiteral::dump(int indent) const
{
    print_indent(indent);
    printf("NumericLiteral %g\n", m_value);
}

void BigIntLiteral::dump(int indent) const
{
    print_indent(indent);
    printf("BigIntLiteral %s\n", m_value.characters());
}

void BooleanLiteral::dump(int indent) const
{
    print_indent(indent);
    printf("BooleanLiteral %s\n", m_value ? "true" : "false");
}

void NullLiteral::dump(int indent) const
{
    print_indent(indent);
    printf("null\n");
}

void FunctionNode::dump(int indent, const char* class_name) const
{
    print_indent(indent);
    printf("%s '%s'\n", class_name, name().characters());
    if (!m_parameters.is_empty()) {
        print_indent(indent + 1);
        printf("(Parameters)\n");

        for (auto& parameter : m_parameters) {
            print_indent(indent + 2);
            if (parameter.is_rest)
                printf("...");
            printf("%s\n", parameter.name.characters());
            if (parameter.default_value)
                parameter.default_value->dump(indent + 3);
        }
    }
    if (!m_variables.is_empty()) {
        print_indent(indent + 1);
        printf("(Variables)\n");

        for (auto& variable : m_variables)
            variable.dump(indent + 2);
    }
    print_indent(indent + 1);
    printf("(Body)\n");
    body().dump(indent + 2);
}

void FunctionDeclaration::dump(int indent) const
{
    FunctionNode::dump(indent, class_name());
}

void FunctionExpression::dump(int indent) const
{
    FunctionNode::dump(indent, class_name());
}

void ReturnStatement::dump(int indent) const
{
    ASTNode::dump(indent);
    if (argument())
        argument()->dump(indent + 1);
}

void IfStatement::dump(int indent) const
{
    ASTNode::dump(indent);

    print_indent(indent);
    printf("If\n");
    predicate().dump(indent + 1);
    consequent().dump(indent + 1);
    if (alternate()) {
        print_indent(indent);
        printf("Else\n");
        alternate()->dump(indent + 1);
    }
}

void WhileStatement::dump(int indent) const
{
    ASTNode::dump(indent);

    print_indent(indent);
    printf("While\n");
    test().dump(indent + 1);
    body().dump(indent + 1);
}

void DoWhileStatement::dump(int indent) const
{
    ASTNode::dump(indent);

    print_indent(indent);
    printf("DoWhile\n");
    test().dump(indent + 1);
    body().dump(indent + 1);
}

void ForStatement::dump(int indent) const
{
    ASTNode::dump(indent);

    print_indent(indent);
    printf("For\n");
    if (init())
        init()->dump(indent + 1);
    if (test())
        test()->dump(indent + 1);
    if (update())
        update()->dump(indent + 1);
    body().dump(indent + 1);
}

void ForInStatement::dump(int indent) const
{
    ASTNode::dump(indent);

    print_indent(indent);
    printf("ForIn\n");
    lhs().dump(indent + 1);
    rhs().dump(indent + 1);
    body().dump(indent + 1);
}

void ForOfStatement::dump(int indent) const
{
    ASTNode::dump(indent);

    print_indent(indent);
    printf("ForOf\n");
    lhs().dump(indent + 1);
    rhs().dump(indent + 1);
    body().dump(indent + 1);
}

Value Identifier::execute(Interpreter& interpreter, GlobalObject& global_object) const
{
    auto value = interpreter.get_variable(string(), global_object);
    if (value.is_empty())
        return interpreter.throw_exception<ReferenceError>(ErrorType::UnknownIdentifier, string().characters());
    return value;
}

void Identifier::dump(int indent) const
{
    print_indent(indent);
    printf("Identifier \"%s\"\n", m_string.characters());
}

void SpreadExpression::dump(int indent) const
{
    ASTNode::dump(indent);
    m_target->dump(indent + 1);
}

Value SpreadExpression::execute(Interpreter& interpreter, GlobalObject& global_object) const
{
    return m_target->execute(interpreter, global_object);
}

Value ThisExpression::execute(Interpreter& interpreter, GlobalObject& global_object) const
{
    return interpreter.this_value(global_object);
}

void ThisExpression::dump(int indent) const
{
    ASTNode::dump(indent);
}

Value AssignmentExpression::execute(Interpreter& interpreter, GlobalObject& global_object) const
{
    auto rhs_result = m_rhs->execute(interpreter, global_object);
    if (interpreter.exception())
        return {};

    Value lhs_result;
    switch (m_op) {
    case AssignmentOp::Assignment:
        break;
    case AssignmentOp::AdditionAssignment:
        lhs_result = m_lhs->execute(interpreter, global_object);
        if (interpreter.exception())
            return {};
        rhs_result = add(interpreter, lhs_result, rhs_result);
        break;
    case AssignmentOp::SubtractionAssignment:
        lhs_result = m_lhs->execute(interpreter, global_object);
        if (interpreter.exception())
            return {};
        rhs_result = sub(interpreter, lhs_result, rhs_result);
        break;
    case AssignmentOp::MultiplicationAssignment:
        lhs_result = m_lhs->execute(interpreter, global_object);
        if (interpreter.exception())
            return {};
        rhs_result = mul(interpreter, lhs_result, rhs_result);
        break;
    case AssignmentOp::DivisionAssignment:
        lhs_result = m_lhs->execute(interpreter, global_object);
        if (interpreter.exception())
            return {};
        rhs_result = div(interpreter, lhs_result, rhs_result);
        break;
    case AssignmentOp::ModuloAssignment:
        lhs_result = m_lhs->execute(interpreter, global_object);
        if (interpreter.exception())
            return {};
        rhs_result = mod(interpreter, lhs_result, rhs_result);
        break;
    case AssignmentOp::ExponentiationAssignment:
        lhs_result = m_lhs->execute(interpreter, global_object);
        if (interpreter.exception())
            return {};
        rhs_result = exp(interpreter, lhs_result, rhs_result);
        break;
    case AssignmentOp::BitwiseAndAssignment:
        lhs_result = m_lhs->execute(interpreter, global_object);
        if (interpreter.exception())
            return {};
        rhs_result = bitwise_and(interpreter, lhs_result, rhs_result);
        break;
    case AssignmentOp::BitwiseOrAssignment:
        lhs_result = m_lhs->execute(interpreter, global_object);
        if (interpreter.exception())
            return {};
        rhs_result = bitwise_or(interpreter, lhs_result, rhs_result);
        break;
    case AssignmentOp::BitwiseXorAssignment:
        lhs_result = m_lhs->execute(interpreter, global_object);
        if (interpreter.exception())
            return {};
        rhs_result = bitwise_xor(interpreter, lhs_result, rhs_result);
        break;
    case AssignmentOp::LeftShiftAssignment:
        lhs_result = m_lhs->execute(interpreter, global_object);
        if (interpreter.exception())
            return {};
        rhs_result = left_shift(interpreter, lhs_result, rhs_result);
        break;
    case AssignmentOp::RightShiftAssignment:
        lhs_result = m_lhs->execute(interpreter, global_object);
        if (interpreter.exception())
            return {};
        rhs_result = right_shift(interpreter, lhs_result, rhs_result);
        break;
    case AssignmentOp::UnsignedRightShiftAssignment:
        lhs_result = m_lhs->execute(interpreter, global_object);
        if (interpreter.exception())
            return {};
        rhs_result = unsigned_right_shift(interpreter, lhs_result, rhs_result);
        break;
    }
    if (interpreter.exception())
        return {};

    auto reference = m_lhs->to_reference(interpreter, global_object);
    if (interpreter.exception())
        return {};

    if (reference.is_unresolvable())
        return interpreter.throw_exception<ReferenceError>(ErrorType::InvalidLeftHandAssignment);

    update_function_name(rhs_result, reference.name().as_string());
    reference.put(interpreter, global_object, rhs_result);

    if (interpreter.exception())
        return {};
    return rhs_result;
}

Value UpdateExpression::execute(Interpreter& interpreter, GlobalObject& global_object) const
{
    auto reference = m_argument->to_reference(interpreter, global_object);
    if (interpreter.exception())
        return {};
    auto old_value = reference.get(interpreter, global_object);
    if (interpreter.exception())
        return {};
    old_value = old_value.to_numeric(interpreter);
    if (interpreter.exception())
        return {};

    Value new_value;
    switch (m_op) {
    case UpdateOp::Increment:
        if (old_value.is_number())
            new_value = Value(old_value.as_double() + 1);
        else
            new_value = js_bigint(interpreter, old_value.as_bigint().big_integer().plus(Crypto::SignedBigInteger { 1 }));
        break;
    case UpdateOp::Decrement:
        if (old_value.is_number())
            new_value = Value(old_value.as_double() - 1);
        else
            new_value = js_bigint(interpreter, old_value.as_bigint().big_integer().minus(Crypto::SignedBigInteger { 1 }));
        break;
    default:
        ASSERT_NOT_REACHED();
    }

    reference.put(interpreter, global_object, new_value);
    if (interpreter.exception())
        return {};
    return m_prefixed ? new_value : old_value;
}

void AssignmentExpression::dump(int indent) const
{
    const char* op_string = nullptr;
    switch (m_op) {
    case AssignmentOp::Assignment:
        op_string = "=";
        break;
    case AssignmentOp::AdditionAssignment:
        op_string = "+=";
        break;
    case AssignmentOp::SubtractionAssignment:
        op_string = "-=";
        break;
    case AssignmentOp::MultiplicationAssignment:
        op_string = "*=";
        break;
    case AssignmentOp::DivisionAssignment:
        op_string = "/=";
        break;
    case AssignmentOp::ModuloAssignment:
        op_string = "%=";
        break;
    case AssignmentOp::ExponentiationAssignment:
        op_string = "**=";
        break;
    case AssignmentOp::BitwiseAndAssignment:
        op_string = "&=";
        break;
    case AssignmentOp::BitwiseOrAssignment:
        op_string = "|=";
        break;
    case AssignmentOp::BitwiseXorAssignment:
        op_string = "^=";
        break;
    case AssignmentOp::LeftShiftAssignment:
        op_string = "<<=";
        break;
    case AssignmentOp::RightShiftAssignment:
        op_string = ">>=";
        break;
    case AssignmentOp::UnsignedRightShiftAssignment:
        op_string = ">>>=";
        break;
    }

    ASTNode::dump(indent);
    print_indent(indent + 1);
    printf("%s\n", op_string);
    m_lhs->dump(indent + 1);
    m_rhs->dump(indent + 1);
}

void UpdateExpression::dump(int indent) const
{
    const char* op_string = nullptr;
    switch (m_op) {
    case UpdateOp::Increment:
        op_string = "++";
        break;
    case UpdateOp::Decrement:
        op_string = "--";
        break;
    }

    ASTNode::dump(indent);
    print_indent(indent + 1);
    if (m_prefixed)
        printf("%s\n", op_string);
    m_argument->dump(indent + 1);
    if (!m_prefixed) {
        print_indent(indent + 1);
        printf("%s\n", op_string);
    }
}

Value VariableDeclaration::execute(Interpreter& interpreter, GlobalObject& global_object) const
{
    for (auto& declarator : m_declarations) {
        if (auto* init = declarator.init()) {
            auto initalizer_result = init->execute(interpreter, global_object);
            if (interpreter.exception())
                return {};
            auto variable_name = declarator.id().string();
            update_function_name(initalizer_result, variable_name);
            interpreter.set_variable(variable_name, initalizer_result, global_object, true);
        }
    }
    return js_undefined();
}

Value VariableDeclarator::execute(Interpreter&, GlobalObject&) const
{
    // NOTE: This node is handled by VariableDeclaration.
    ASSERT_NOT_REACHED();
}

void VariableDeclaration::dump(int indent) const
{
    const char* declaration_kind_string = nullptr;
    switch (m_declaration_kind) {
    case DeclarationKind::Let:
        declaration_kind_string = "Let";
        break;
    case DeclarationKind::Var:
        declaration_kind_string = "Var";
        break;
    case DeclarationKind::Const:
        declaration_kind_string = "Const";
        break;
    }

    ASTNode::dump(indent);
    print_indent(indent + 1);
    printf("%s\n", declaration_kind_string);

    for (auto& declarator : m_declarations)
        declarator.dump(indent + 1);
}

void VariableDeclarator::dump(int indent) const
{
    ASTNode::dump(indent);
    m_id->dump(indent + 1);
    if (m_init)
        m_init->dump(indent + 1);
}

void ObjectProperty::dump(int indent) const
{
    ASTNode::dump(indent);
    m_key->dump(indent + 1);
    m_value->dump(indent + 1);
}

void ObjectExpression::dump(int indent) const
{
    ASTNode::dump(indent);
    for (auto& property : m_properties) {
        property.dump(indent + 1);
    }
}

void ExpressionStatement::dump(int indent) const
{
    ASTNode::dump(indent);
    m_expression->dump(indent + 1);
}

Value ObjectProperty::execute(Interpreter&, GlobalObject&) const
{
    // NOTE: ObjectProperty execution is handled by ObjectExpression.
    ASSERT_NOT_REACHED();
}

Value ObjectExpression::execute(Interpreter& interpreter, GlobalObject& global_object) const
{
    auto* object = Object::create_empty(interpreter, global_object);
    for (auto& property : m_properties) {
        auto key_result = property.key().execute(interpreter, global_object);
        if (interpreter.exception())
            return {};

        if (property.type() == ObjectProperty::Type::Spread) {
            if (key_result.is_array()) {
                auto& array_to_spread = static_cast<Array&>(key_result.as_object());
                for (auto& entry : array_to_spread.indexed_properties()) {
                    object->indexed_properties().append(entry.value_and_attributes(&array_to_spread).value);
                    if (interpreter.exception())
                        return {};
                }
            } else if (key_result.is_object()) {
                auto& obj_to_spread = key_result.as_object();

                for (auto& it : obj_to_spread.shape().property_table_ordered()) {
                    if (it.value.attributes.is_enumerable()) {
                        object->define_property(it.key, obj_to_spread.get(it.key));
                        if (interpreter.exception())
                            return {};
                    }
                }
            } else if (key_result.is_string()) {
                auto& str_to_spread = key_result.as_string().string();

                for (size_t i = 0; i < str_to_spread.length(); i++) {
                    object->define_property(i, js_string(interpreter, str_to_spread.substring(i, 1)));
                    if (interpreter.exception())
                        return {};
                }
            }

            continue;
        }

        auto key = key_result.to_string(interpreter);
        if (interpreter.exception())
            return {};
        auto value = property.value().execute(interpreter, global_object);
        if (interpreter.exception())
            return {};

        String name = key;
        if (property.type() == ObjectProperty::Type::Getter) {
            name = String::format("get %s", key.characters());
        } else if (property.type() == ObjectProperty::Type::Setter) {
            name = String::format("set %s", key.characters());
        }

        update_function_name(value, name);

        if (property.type() == ObjectProperty::Type::Getter || property.type() == ObjectProperty::Type::Setter) {
            ASSERT(value.is_function());
            Accessor* accessor { nullptr };
            auto property_metadata = object->shape().lookup(key);
            if (property_metadata.has_value()) {
                auto existing_property = object->get_direct(property_metadata.value().offset);
                if (existing_property.is_accessor())
                    accessor = &existing_property.as_accessor();
            }
            if (!accessor) {
                accessor = Accessor::create(interpreter, global_object, nullptr, nullptr);
                object->define_property(key, accessor, Attribute::Configurable | Attribute::Enumerable);
                if (interpreter.exception())
                    return {};
            }
            if (property.type() == ObjectProperty::Type::Getter)
                accessor->set_getter(&value.as_function());
            else
                accessor->set_setter(&value.as_function());
        } else {
            object->define_property(key, value);
            if (interpreter.exception())
                return {};
        }
    }
    return object;
}

void MemberExpression::dump(int indent) const
{
    print_indent(indent);
    printf("%s (computed=%s)\n", class_name(), is_computed() ? "true" : "false");
    m_object->dump(indent + 1);
    m_property->dump(indent + 1);
}

PropertyName MemberExpression::computed_property_name(Interpreter& interpreter, GlobalObject& global_object) const
{
    if (!is_computed()) {
        ASSERT(m_property->is_identifier());
        return static_cast<const Identifier&>(*m_property).string();
    }
    auto index = m_property->execute(interpreter, global_object);
    if (interpreter.exception())
        return {};

    ASSERT(!index.is_empty());

    if (index.is_integer() && index.as_i32() >= 0)
        return index.as_i32();

    auto index_string = index.to_string(interpreter);
    if (interpreter.exception())
        return {};
    return index_string;
}

String MemberExpression::to_string_approximation() const
{
    String object_string = "<object>";
    if (m_object->is_identifier())
        object_string = static_cast<const Identifier&>(*m_object).string();
    if (is_computed())
        return String::format("%s[<computed>]", object_string.characters());
    ASSERT(m_property->is_identifier());
    return String::format("%s.%s", object_string.characters(), static_cast<const Identifier&>(*m_property).string().characters());
}

Value MemberExpression::execute(Interpreter& interpreter, GlobalObject& global_object) const
{
    auto object_value = m_object->execute(interpreter, global_object);
    if (interpreter.exception())
        return {};
    auto* object_result = object_value.to_object(interpreter, global_object);
    if (interpreter.exception())
        return {};
    return object_result->get(computed_property_name(interpreter, global_object)).value_or(js_undefined());
}

Value StringLiteral::execute(Interpreter& interpreter, GlobalObject&) const
{
    return js_string(interpreter, m_value);
}

Value NumericLiteral::execute(Interpreter&, GlobalObject&) const
{
    return Value(m_value);
}

Value BigIntLiteral::execute(Interpreter& interpreter, GlobalObject&) const
{
    return js_bigint(interpreter, Crypto::SignedBigInteger::from_base10(m_value.substring(0, m_value.length() - 1)));
}

Value BooleanLiteral::execute(Interpreter&, GlobalObject&) const
{
    return Value(m_value);
}

Value NullLiteral::execute(Interpreter&, GlobalObject&) const
{
    return js_null();
}

void RegExpLiteral::dump(int indent) const
{
    print_indent(indent);
    printf("%s (/%s/%s)\n", class_name(), content().characters(), flags().characters());
}

Value RegExpLiteral::execute(Interpreter&, GlobalObject& global_object) const
{
    return RegExpObject::create(global_object, content(), flags());
}

void ArrayExpression::dump(int indent) const
{
    ASTNode::dump(indent);
    for (auto& element : m_elements) {
        if (element) {
            element->dump(indent + 1);
        } else {
            print_indent(indent + 1);
            printf("<empty>\n");
        }
    }
}

Value ArrayExpression::execute(Interpreter& interpreter, GlobalObject& global_object) const
{
    auto* array = Array::create(global_object);
    for (auto& element : m_elements) {
        auto value = Value();
        if (element) {
            value = element->execute(interpreter, global_object);

            if (interpreter.exception())
                return {};

            if (element->is_spread_expression()) {
                // FIXME: Support arbitrary iterables
                if (value.is_array()) {
                    auto& array_to_spread = static_cast<Array&>(value.as_object());
                    for (auto& entry : array_to_spread.indexed_properties()) {
                        array->indexed_properties().append(entry.value_and_attributes(&array_to_spread).value);
                        if (interpreter.exception())
                            return {};
                    }
                    continue;
                }
                if (value.is_string() || (value.is_object() && value.as_object().is_string_object())) {
                    String string_to_spread;
                    if (value.is_string()) {
                        string_to_spread = value.as_string().string();
                    } else {
                        string_to_spread = static_cast<const StringObject&>(value.as_object()).primitive_string().string();
                    }
                    for (size_t i = 0; i < string_to_spread.length(); ++i)
                        array->indexed_properties().append(js_string(interpreter, string_to_spread.substring(i, 1)));
                    continue;
                }
                interpreter.throw_exception<TypeError>(ErrorType::NotIterable, value.to_string_without_side_effects().characters());
                return {};
            }
        }
        array->indexed_properties().append(value);
    }
    return array;
}

void TemplateLiteral::dump(int indent) const
{
    ASTNode::dump(indent);
    for (auto& expression : m_expressions)
        expression.dump(indent + 1);
}

Value TemplateLiteral::execute(Interpreter& interpreter, GlobalObject& global_object) const
{
    StringBuilder string_builder;

    for (auto& expression : m_expressions) {
        auto expr = expression.execute(interpreter, global_object);
        if (interpreter.exception())
            return {};
        auto string = expr.to_string(interpreter);
        if (interpreter.exception())
            return {};
        string_builder.append(string);
    }

    return js_string(interpreter, string_builder.build());
}

void TaggedTemplateLiteral::dump(int indent) const
{
    ASTNode::dump(indent);
    print_indent(indent + 1);
    printf("(Tag)\n");
    m_tag->dump(indent + 2);
    print_indent(indent + 1);
    printf("(Template Literal)\n");
    m_template_literal->dump(indent + 2);
}

Value TaggedTemplateLiteral::execute(Interpreter& interpreter, GlobalObject& global_object) const
{
    auto tag = m_tag->execute(interpreter, global_object);
    if (interpreter.exception())
        return {};
    if (!tag.is_function()) {
        interpreter.throw_exception<TypeError>(ErrorType::NotAFunction, tag.to_string_without_side_effects().characters());
        return {};
    }
    auto& tag_function = tag.as_function();
    auto& expressions = m_template_literal->expressions();
    auto* strings = Array::create(global_object);
    MarkedValueList arguments(interpreter.heap());
    arguments.append(strings);
    for (size_t i = 0; i < expressions.size(); ++i) {
        auto value = expressions[i].execute(interpreter, global_object);
        if (interpreter.exception())
            return {};
        // tag`${foo}`             -> "", foo, ""                -> tag(["", ""], foo)
        // tag`foo${bar}baz${qux}` -> "foo", bar, "baz", qux, "" -> tag(["foo", "baz", ""], bar, qux)
        if (i % 2 == 0) {
            strings->indexed_properties().append(value);
        } else {
            arguments.append(value);
        }
    }

    auto* raw_strings = Array::create(global_object);
    for (auto& raw_string : m_template_literal->raw_strings()) {
        auto value = raw_string.execute(interpreter, global_object);
        if (interpreter.exception())
            return {};
        raw_strings->indexed_properties().append(value);
    }
    strings->define_property("raw", raw_strings, 0);
    if (interpreter.exception())
        return {};

    return interpreter.call(tag_function, js_undefined(), move(arguments));
}

void TryStatement::dump(int indent) const
{
    ASTNode::dump(indent);
    print_indent(indent);
    printf("(Block)\n");
    block().dump(indent + 1);

    if (handler()) {
        print_indent(indent);
        printf("(Handler)\n");
        handler()->dump(indent + 1);
    }

    if (finalizer()) {
        print_indent(indent);
        printf("(Finalizer)\n");
        finalizer()->dump(indent + 1);
    }
}

void CatchClause::dump(int indent) const
{
    print_indent(indent);
    printf("CatchClause");
    if (!m_parameter.is_null())
        printf(" (%s)", m_parameter.characters());
    printf("\n");
    body().dump(indent + 1);
}

void ThrowStatement::dump(int indent) const
{
    ASTNode::dump(indent);
    argument().dump(indent + 1);
}

Value TryStatement::execute(Interpreter& interpreter, GlobalObject& global_object) const
{
    interpreter.run(global_object, block(), {}, ScopeType::Try);
    if (auto* exception = interpreter.exception()) {
        if (m_handler) {
            interpreter.clear_exception();
            ArgumentVector arguments { { m_handler->parameter(), exception->value() } };
            interpreter.run(global_object, m_handler->body(), move(arguments));
        }
    }

    if (m_finalizer)
        m_finalizer->execute(interpreter, global_object);

    return js_undefined();
}

Value CatchClause::execute(Interpreter&, GlobalObject&) const
{
    // NOTE: CatchClause execution is handled by TryStatement.
    ASSERT_NOT_REACHED();
    return {};
}

Value ThrowStatement::execute(Interpreter& interpreter, GlobalObject& global_object) const
{
    auto value = m_argument->execute(interpreter, global_object);
    if (interpreter.exception())
        return {};
    return interpreter.throw_exception(value);
}

Value SwitchStatement::execute(Interpreter& interpreter, GlobalObject& global_object) const
{
    auto discriminant_result = m_discriminant->execute(interpreter, global_object);
    if (interpreter.exception())
        return {};

    bool falling_through = false;

    for (auto& switch_case : m_cases) {
        if (!falling_through && switch_case.test()) {
            auto test_result = switch_case.test()->execute(interpreter, global_object);
            if (interpreter.exception())
                return {};
            if (!strict_eq(interpreter, discriminant_result, test_result))
                continue;
        }
        falling_through = true;

        for (auto& statement : switch_case.consequent()) {
            statement.execute(interpreter, global_object);
            if (interpreter.exception())
                return {};
            if (interpreter.should_unwind()) {
                if (interpreter.should_unwind_until(ScopeType::Breakable, m_label)) {
                    interpreter.stop_unwind();
                    return {};
                }
                return {};
            }
        }
    }

    return js_undefined();
}

Value SwitchCase::execute(Interpreter&, GlobalObject&) const
{
    return {};
}

Value BreakStatement::execute(Interpreter& interpreter, GlobalObject&) const
{
    interpreter.unwind(ScopeType::Breakable, m_target_label);
    return js_undefined();
}

Value ContinueStatement::execute(Interpreter& interpreter, GlobalObject&) const
{
    interpreter.unwind(ScopeType::Continuable, m_target_label);
    return js_undefined();
}

void SwitchStatement::dump(int indent) const
{
    ASTNode::dump(indent);
    m_discriminant->dump(indent + 1);
    for (auto& switch_case : m_cases) {
        switch_case.dump(indent + 1);
    }
}

void SwitchCase::dump(int indent) const
{
    ASTNode::dump(indent);
    print_indent(indent + 1);
    if (m_test) {
        printf("(Test)\n");
        m_test->dump(indent + 2);
    } else {
        printf("(Default)\n");
    }
    print_indent(indent + 1);
    printf("(Consequent)\n");
    for (auto& statement : m_consequent)
        statement.dump(indent + 2);
}

Value ConditionalExpression::execute(Interpreter& interpreter, GlobalObject& global_object) const
{
    auto test_result = m_test->execute(interpreter, global_object);
    if (interpreter.exception())
        return {};
    Value result;
    if (test_result.to_boolean()) {
        result = m_consequent->execute(interpreter, global_object);
    } else {
        result = m_alternate->execute(interpreter, global_object);
    }
    if (interpreter.exception())
        return {};
    return result;
}

void ConditionalExpression::dump(int indent) const
{
    ASTNode::dump(indent);
    print_indent(indent + 1);
    printf("(Test)\n");
    m_test->dump(indent + 2);
    print_indent(indent + 1);
    printf("(Consequent)\n");
    m_consequent->dump(indent + 2);
    print_indent(indent + 1);
    printf("(Alternate)\n");
    m_alternate->dump(indent + 2);
}

void SequenceExpression::dump(int indent) const
{
    ASTNode::dump(indent);
    for (auto& expression : m_expressions)
        expression.dump(indent + 1);
}

Value SequenceExpression::execute(Interpreter& interpreter, GlobalObject& global_object) const
{
    Value last_value;
    for (auto& expression : m_expressions) {
        last_value = expression.execute(interpreter, global_object);
        if (interpreter.exception())
            return {};
    }
    return last_value;
}

Value DebuggerStatement::execute(Interpreter&, GlobalObject&) const
{
    dbg() << "Sorry, no JavaScript debugger available (yet)!";
    return js_undefined();
}

void ScopeNode::add_variables(NonnullRefPtrVector<VariableDeclaration> variables)
{
    m_variables.append(move(variables));
}

void ScopeNode::add_functions(NonnullRefPtrVector<FunctionDeclaration> functions)
{
    m_functions.append(move(functions));
}

}
