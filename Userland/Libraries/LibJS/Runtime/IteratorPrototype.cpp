/*
 * Copyright (c) 2020, Matthew Olsson <mattco@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/Function.h>
#include <LibJS/Runtime/AbstractOperations.h>
#include <LibJS/Runtime/FunctionObject.h>
#include <LibJS/Runtime/GlobalObject.h>
#include <LibJS/Runtime/IteratorHelper.h>
#include <LibJS/Runtime/IteratorOperations.h>
#include <LibJS/Runtime/IteratorPrototype.h>

namespace JS {

// 27.1.2 The %IteratorPrototype% Object, https://tc39.es/ecma262/#sec-%iteratorprototype%-object
IteratorPrototype::IteratorPrototype(Realm& realm)
    : PrototypeObject(realm.intrinsics().object_prototype())
{
}

ThrowCompletionOr<void> IteratorPrototype::initialize(Realm& realm)
{
    auto& vm = this->vm();
    MUST_OR_THROW_OOM(Base::initialize(realm));

    // 3.1.3.13 Iterator.prototype [ @@toStringTag ], https://tc39.es/proposal-iterator-helpers/#sec-iteratorprototype-@@tostringtag
    define_direct_property(vm.well_known_symbol_to_string_tag(), MUST_OR_THROW_OOM(PrimitiveString::create(vm, "Iterator"sv)), Attribute::Configurable | Attribute::Writable);

    u8 attr = Attribute::Writable | Attribute::Configurable;
    define_native_function(realm, vm.well_known_symbol_iterator(), symbol_iterator, 0, attr);
    define_native_function(realm, vm.names.map, map, 1, attr);
    define_native_function(realm, vm.names.filter, filter, 1, attr);
    define_native_function(realm, vm.names.take, take, 1, attr);
    define_native_function(realm, vm.names.drop, drop, 1, attr);

    return {};
}

// 27.1.2.1 %IteratorPrototype% [ @@iterator ] ( ), https://tc39.es/ecma262/#sec-%iteratorprototype%-@@iterator
JS_DEFINE_NATIVE_FUNCTION(IteratorPrototype::symbol_iterator)
{
    // 1. Return the this value.
    return vm.this_value();
}

// 3.1.3.2 Iterator.prototype.map ( mapper ), https://tc39.es/proposal-iterator-helpers/#sec-iteratorprototype.map
JS_DEFINE_NATIVE_FUNCTION(IteratorPrototype::map)
{
    auto& realm = *vm.current_realm();

    auto mapper = vm.argument(0);

    // 1. Let O be the this value.
    // 2. If O is not an Object, throw a TypeError exception.
    auto object = TRY(this_object(vm));

    // 3. If IsCallable(mapper) is false, throw a TypeError exception.
    if (!mapper.is_function())
        return vm.throw_completion<TypeError>(ErrorType::NotAFunction, "mapper"sv);

    // 4. Let iterated be ? GetIteratorDirect(O).
    auto iterated = TRY(get_iterator_direct(vm, object));

    // 5. Let closure be a new Abstract Closure with no parameters that captures iterated and mapper and performs the following steps when called:
    IteratorHelper::Closure closure = [mapper = NonnullGCPtr { mapper.as_function() }](auto& iterator) -> ThrowCompletionOr<Value> {
        auto& vm = iterator.vm();

        auto const& iterated = iterator.underlying_iterator();

        // a. Let counter be 0.
        // b. Repeat,

        // i. Let next be ? IteratorStep(iterated).
        auto next = TRY(iterator_step(vm, iterated));

        // ii. If next is false, return undefined.
        if (!next)
            return iterator.result(js_undefined());

        // iii. Let value be ? IteratorValue(next).
        auto value = TRY(iterator_value(vm, *next));

        // iv. Let mapped be Completion(Call(mapper, undefined, « value, 𝔽(counter) »)).
        auto mapped = call(vm, *mapper, js_undefined(), value, Value { iterator.counter() });

        // v. IfAbruptCloseIterator(mapped, iterated).
        if (mapped.is_error())
            return iterator.close_result(mapped.release_error());

        // viii. Set counter to counter + 1.
        // NOTE: We do this step early to ensure it occurs before returning.
        iterator.increment_counter();

        // vi. Let completion be Completion(Yield(mapped)).
        // vii. IfAbruptCloseIterator(completion, iterated).
        return iterator.result(mapped.release_value());
    };

    // 6. Let result be CreateIteratorFromClosure(closure, "Iterator Helper", %IteratorHelperPrototype%, « [[UnderlyingIterator]] »).
    // 7. Set result.[[UnderlyingIterator]] to iterated.
    auto result = TRY(IteratorHelper::create(realm, move(iterated), move(closure)));

    // 8. Return result.
    return result;
}

// 3.1.3.3 Iterator.prototype.filter ( predicate ), https://tc39.es/proposal-iterator-helpers/#sec-iteratorprototype.filter
JS_DEFINE_NATIVE_FUNCTION(IteratorPrototype::filter)
{
    auto& realm = *vm.current_realm();

    auto predicate = vm.argument(0);

    // 1. Let O be the this value.
    // 2. If O is not an Object, throw a TypeError exception.
    auto object = TRY(this_object(vm));

    // 3. If IsCallable(predicate) is false, throw a TypeError exception.
    if (!predicate.is_function())
        return vm.throw_completion<TypeError>(ErrorType::NotAFunction, "predicate"sv);

    // 4. Let iterated be ? GetIteratorDirect(O).
    auto iterated = TRY(get_iterator_direct(vm, object));

    // 5. Let closure be a new Abstract Closure with no parameters that captures iterated and predicate and performs the following steps when called:
    IteratorHelper::Closure closure = [predicate = NonnullGCPtr { predicate.as_function() }](auto& iterator) -> ThrowCompletionOr<Value> {
        auto& vm = iterator.vm();

        auto const& iterated = iterator.underlying_iterator();

        // a. Let counter be 0.

        // b. Repeat,
        while (true) {
            // i. Let next be ? IteratorStep(iterated).
            auto next = TRY(iterator_step(vm, iterated));

            // ii. If next is false, return undefined.
            if (!next)
                return iterator.result(js_undefined());

            // iii. Let value be ? IteratorValue(next).
            auto value = TRY(iterator_value(vm, *next));

            // iv. Let selected be Completion(Call(predicate, undefined, « value, 𝔽(counter) »)).
            auto selected = call(vm, *predicate, js_undefined(), value, Value { iterator.counter() });

            // v. IfAbruptCloseIterator(selected, iterated).
            if (selected.is_error())
                return iterator.close_result(selected.release_error());

            // vii. Set counter to counter + 1.
            // NOTE: We do this step early to ensure it occurs before returning.
            iterator.increment_counter();

            // vi. If ToBoolean(selected) is true, then
            if (selected.value().to_boolean()) {
                // 1. Let completion be Completion(Yield(value)).
                // 2. IfAbruptCloseIterator(completion, iterated).
                return iterator.result(value);
            }
        }
    };

    // 6. Let result be CreateIteratorFromClosure(closure, "Iterator Helper", %IteratorHelperPrototype%, « [[UnderlyingIterator]] »).
    // 7. Set result.[[UnderlyingIterator]] to iterated.
    auto result = TRY(IteratorHelper::create(realm, move(iterated), move(closure)));

    // 8. Return result.
    return result;
}

// 3.1.3.4 Iterator.prototype.take ( limit ), https://tc39.es/proposal-iterator-helpers/#sec-iteratorprototype.take
JS_DEFINE_NATIVE_FUNCTION(IteratorPrototype::take)
{
    auto& realm = *vm.current_realm();

    auto limit = vm.argument(0);

    // 1. Let O be the this value.
    // 2. If O is not an Object, throw a TypeError exception.
    auto object = TRY(this_object(vm));

    // 3. Let numLimit be ? ToNumber(limit).
    auto numeric_limit = TRY(limit.to_number(vm));

    // 4. If numLimit is NaN, throw a RangeError exception.
    if (numeric_limit.is_nan())
        return vm.throw_completion<RangeError>(ErrorType::NumberIsNaN, "limit"sv);

    // 5. Let integerLimit be ! ToIntegerOrInfinity(numLimit).
    auto integer_limit = MUST(numeric_limit.to_integer_or_infinity(vm));

    // 6. If integerLimit < 0, throw a RangeError exception.
    if (integer_limit < 0)
        return vm.throw_completion<RangeError>(ErrorType::NumberIsNegative, "limit"sv);

    // 7. Let iterated be ? GetIteratorDirect(O).
    auto iterated = TRY(get_iterator_direct(vm, object));

    // 8. Let closure be a new Abstract Closure with no parameters that captures iterated and integerLimit and performs the following steps when called:
    IteratorHelper::Closure closure = [integer_limit](auto& iterator) -> ThrowCompletionOr<Value> {
        auto& vm = iterator.vm();

        auto const& iterated = iterator.underlying_iterator();

        // a. Let remaining be integerLimit.
        // b. Repeat,

        // i. If remaining is 0, then
        if (iterator.counter() >= integer_limit) {
            // 1. Return ? IteratorClose(iterated, NormalCompletion(undefined)).
            return iterator.close_result(normal_completion(js_undefined()));
        }

        // ii. If remaining is not +∞, then
        //     1. Set remaining to remaining - 1.
        iterator.increment_counter();

        // iii. Let next be ? IteratorStep(iterated).
        auto next = TRY(iterator_step(vm, iterated));

        // iv. If next is false, return undefined.
        if (!next)
            return iterator.result(js_undefined());

        // v. Let completion be Completion(Yield(? IteratorValue(next))).
        // vi. IfAbruptCloseIterator(completion, iterated).
        return iterator.result(TRY(iterator_value(vm, *next)));
    };

    // 9. Let result be CreateIteratorFromClosure(closure, "Iterator Helper", %IteratorHelperPrototype%, « [[UnderlyingIterator]] »).
    // 10. Set result.[[UnderlyingIterator]] to iterated.
    auto result = TRY(IteratorHelper::create(realm, move(iterated), move(closure)));

    // 11. Return result.
    return result;
}

// 3.1.3.5 Iterator.prototype.drop ( limit ), https://tc39.es/proposal-iterator-helpers/#sec-iteratorprototype.drop
JS_DEFINE_NATIVE_FUNCTION(IteratorPrototype::drop)
{
    auto& realm = *vm.current_realm();

    auto limit = vm.argument(0);

    // 1. Let O be the this value.
    // 2. If O is not an Object, throw a TypeError exception.
    auto object = TRY(this_object(vm));

    // 3. Let numLimit be ? ToNumber(limit).
    auto numeric_limit = TRY(limit.to_number(vm));

    // 4. If numLimit is NaN, throw a RangeError exception.
    if (numeric_limit.is_nan())
        return vm.throw_completion<RangeError>(ErrorType::NumberIsNaN, "limit"sv);

    // 5. Let integerLimit be ! ToIntegerOrInfinity(numLimit).
    auto integer_limit = MUST(numeric_limit.to_integer_or_infinity(vm));

    // 6. If integerLimit < 0, throw a RangeError exception.
    if (integer_limit < 0)
        return vm.throw_completion<RangeError>(ErrorType::NumberIsNegative, "limit"sv);

    // 7. Let iterated be ? GetIteratorDirect(O).
    auto iterated = TRY(get_iterator_direct(vm, object));

    // 8. Let closure be a new Abstract Closure with no parameters that captures iterated and integerLimit and performs the following steps when called:
    IteratorHelper::Closure closure = [integer_limit](auto& iterator) -> ThrowCompletionOr<Value> {
        auto& vm = iterator.vm();

        auto const& iterated = iterator.underlying_iterator();

        // a. Let remaining be integerLimit.
        // b. Repeat, while remaining > 0,
        while (iterator.counter() < integer_limit) {
            // i. If remaining is not +∞, then
            //     1. Set remaining to remaining - 1.
            iterator.increment_counter();

            // ii. Let next be ? IteratorStep(iterated).
            auto next = TRY(iterator_step(vm, iterated));

            // iii. If next is false, return undefined.
            if (!next)
                return iterator.result(js_undefined());
        }

        // c. Repeat,

        // i. Let next be ? IteratorStep(iterated).
        auto next = TRY(iterator_step(vm, iterated));

        // ii. If next is false, return undefined.
        if (!next)
            return iterator.result(js_undefined());

        // iii. Let completion be Completion(Yield(? IteratorValue(next))).
        // iv. IfAbruptCloseIterator(completion, iterated).
        return iterator.result(TRY(iterator_value(vm, *next)));
    };

    // 9. Let result be CreateIteratorFromClosure(closure, "Iterator Helper", %IteratorHelperPrototype%, « [[UnderlyingIterator]] »).
    // 10. Set result.[[UnderlyingIterator]] to iterated.
    auto result = TRY(IteratorHelper::create(realm, move(iterated), move(closure)));

    // 11. Return result.
    return result;
}

}
