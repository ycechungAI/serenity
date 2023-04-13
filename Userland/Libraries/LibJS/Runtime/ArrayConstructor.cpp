/*
 * Copyright (c) 2020, Andreas Kling <kling@serenityos.org>
 * Copyright (c) 2020-2023, Linus Groh <linusg@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/Function.h>
#include <LibJS/Runtime/AbstractOperations.h>
#include <LibJS/Runtime/Array.h>
#include <LibJS/Runtime/ArrayConstructor.h>
#include <LibJS/Runtime/Error.h>
#include <LibJS/Runtime/GlobalObject.h>
#include <LibJS/Runtime/IteratorOperations.h>
#include <LibJS/Runtime/Shape.h>

namespace JS {

ArrayConstructor::ArrayConstructor(Realm& realm)
    : NativeFunction(realm.vm().names.Array.as_string(), realm.intrinsics().function_prototype())
{
}

ThrowCompletionOr<void> ArrayConstructor::initialize(Realm& realm)
{
    auto& vm = this->vm();
    MUST_OR_THROW_OOM(NativeFunction::initialize(realm));

    // 23.1.2.4 Array.prototype, https://tc39.es/ecma262/#sec-array.prototype
    define_direct_property(vm.names.prototype, realm.intrinsics().array_prototype(), 0);

    u8 attr = Attribute::Writable | Attribute::Configurable;
    define_native_function(realm, vm.names.from, from, 1, attr);
    define_native_function(realm, vm.names.isArray, is_array, 1, attr);
    define_native_function(realm, vm.names.of, of, 0, attr);

    // 23.1.2.5 get Array [ @@species ], https://tc39.es/ecma262/#sec-get-array-@@species
    define_native_accessor(realm, vm.well_known_symbol_species(), symbol_species_getter, {}, Attribute::Configurable);

    define_direct_property(vm.names.length, Value(1), Attribute::Configurable);

    return {};
}

// 23.1.1.1 Array ( ...values ), https://tc39.es/ecma262/#sec-array
ThrowCompletionOr<Value> ArrayConstructor::call()
{
    // 1. If NewTarget is undefined, let newTarget be the active function object; else let newTarget be NewTarget.
    return TRY(construct(*this));
}

// 23.1.1.1 Array ( ...values ), https://tc39.es/ecma262/#sec-array
ThrowCompletionOr<NonnullGCPtr<Object>> ArrayConstructor::construct(FunctionObject& new_target)
{
    auto& vm = this->vm();
    auto& realm = *vm.current_realm();

    // 2. Let proto be ? GetPrototypeFromConstructor(newTarget, "%Array.prototype%").
    auto* proto = TRY(get_prototype_from_constructor(vm, new_target, &Intrinsics::array_prototype));

    // 3. Let numberOfArgs be the number of elements in values.
    // 4. If numberOfArgs = 0, then
    if (vm.argument_count() == 0) {
        // a. Return ! ArrayCreate(0, proto).
        return MUST(Array::create(realm, 0, proto));
    }

    // 5. Else if numberOfArgs = 1, then
    if (vm.argument_count() == 1) {
        // a. Let len be values[0].
        auto length = vm.argument(0);

        // b. Let array be ! ArrayCreate(0, proto).
        auto array = MUST(Array::create(realm, 0, proto));

        size_t int_length;

        // c. If len is not a Number, then
        if (!length.is_number()) {
            // i. Perform ! CreateDataPropertyOrThrow(array, "0", len).
            MUST(array->create_data_property_or_throw(0, length));

            // ii. Let intLen be 1𝔽.
            int_length = 1;
        }
        // d. Else,
        else {
            // i. Let intLen be ! ToUint32(len).
            int_length = MUST(length.to_u32(vm));

            // ii. If SameValueZero(intLen, len) is false, throw a RangeError exception.
            if (int_length != length.as_double())
                return vm.throw_completion<RangeError>(ErrorType::InvalidLength, "array");
        }

        // e. Perform ! Set(array, "length", intLen, true).
        TRY(array->set(vm.names.length, Value(int_length), Object::ShouldThrowExceptions::Yes));

        // f. Return array.
        return array;
    }

    // 6. Else,

    // a. Assert: numberOfArgs ≥ 2.
    VERIFY(vm.argument_count() >= 2);

    // b. Let array be ? ArrayCreate(numberOfArgs, proto).
    auto array = TRY(Array::create(realm, vm.argument_count(), proto));

    // c. Let k be 0.
    // d. Repeat, while k < numberOfArgs,
    for (size_t k = 0; k < vm.argument_count(); ++k) {
        // i. Let Pk be ! ToString(𝔽(k)).
        auto property_key = PropertyKey { k };

        // ii. Let itemK be values[k].
        auto item_k = vm.argument(k);

        // iii. Perform ! CreateDataPropertyOrThrow(array, Pk, itemK).
        MUST(array->create_data_property_or_throw(property_key, item_k));

        // iv. Set k to k + 1.
    }

    // e. Assert: The mathematical value of array's "length" property is numberOfArgs.

    // f. Return array.
    return array;
}

// 23.1.2.1 Array.from ( items [ , mapfn [ , thisArg ] ] ), https://tc39.es/ecma262/#sec-array.from
JS_DEFINE_NATIVE_FUNCTION(ArrayConstructor::from)
{
    auto& realm = *vm.current_realm();

    auto items = vm.argument(0);
    auto mapfn_value = vm.argument(1);
    auto this_arg = vm.argument(2);

    // 1. Let C be the this value.
    auto constructor = vm.this_value();

    // 2. If mapfn is undefined, let mapping be false.
    GCPtr<FunctionObject> mapfn;

    // 3. Else,
    if (!mapfn_value.is_undefined()) {
        // a. If IsCallable(mapfn) is false, throw a TypeError exception.
        if (!mapfn_value.is_function())
            return vm.throw_completion<TypeError>(ErrorType::NotAFunction, TRY_OR_THROW_OOM(vm, mapfn_value.to_string_without_side_effects()));

        // b. Let mapping be true.
        mapfn = &mapfn_value.as_function();
    }

    // 4. Let usingIterator be ? GetMethod(items, @@iterator).
    auto using_iterator = TRY(items.get_method(vm, vm.well_known_symbol_iterator()));

    // 5. If usingIterator is not undefined, then
    if (using_iterator) {
        GCPtr<Object> array;

        // a. If IsConstructor(C) is true, then
        if (constructor.is_constructor()) {
            // i. Let A be ? Construct(C).
            array = TRY(JS::construct(vm, constructor.as_function()));
        }
        // b. Else,
        else {
            // i. Let A be ! ArrayCreate(0).
            array = MUST(Array::create(realm, 0));
        }

        // c. Let iteratorRecord be ? GetIterator(items, sync, usingIterator).
        auto iterator = TRY(get_iterator(vm, items, IteratorHint::Sync, using_iterator));

        // d. Let k be 0.
        // e. Repeat,
        for (size_t k = 0;; ++k) {
            // i. If k ≥ 2^53 - 1, then
            if (k >= MAX_ARRAY_LIKE_INDEX) {
                // 1. Let error be ThrowCompletion(a newly created TypeError object).
                auto error = vm.throw_completion<TypeError>(ErrorType::ArrayMaxSize);

                // 2. Return ? IteratorClose(iteratorRecord, error).
                return *TRY(iterator_close(vm, iterator, move(error)));
            }

            // ii. Let Pk be ! ToString(𝔽(k)).
            auto property_key = PropertyKey { k };

            // iii. Let next be ? IteratorStep(iteratorRecord).
            auto* next = TRY(iterator_step(vm, iterator));

            // iv. If next is false, then
            if (!next) {
                // 1. Perform ? Set(A, "length", 𝔽(k), true).
                TRY(array->set(vm.names.length, Value(k), Object::ShouldThrowExceptions::Yes));

                // 2. Return A.
                return array;
            }

            // v. Let nextValue be ? IteratorValue(next).
            auto next_value = TRY(iterator_value(vm, *next));

            Value mapped_value;

            // vi. If mapping is true, then
            if (mapfn) {
                // 1. Let mappedValue be Completion(Call(mapfn, thisArg, « nextValue, 𝔽(k) »)).
                auto mapped_value_or_error = JS::call(vm, *mapfn, this_arg, next_value, Value(k));

                // 2. IfAbruptCloseIterator(mappedValue, iteratorRecord).
                if (mapped_value_or_error.is_error())
                    return *TRY(iterator_close(vm, iterator, mapped_value_or_error.release_error()));
                mapped_value = mapped_value_or_error.release_value();
            }
            // vii. Else, let mappedValue be nextValue.
            else {
                mapped_value = next_value;
            }

            // viii. Let defineStatus be Completion(CreateDataPropertyOrThrow(A, Pk, mappedValue)).
            auto result_or_error = array->create_data_property_or_throw(property_key, mapped_value);

            // IfAbruptCloseIterator(defineStatus, iteratorRecord).
            if (result_or_error.is_error())
                return *TRY(iterator_close(vm, iterator, result_or_error.release_error()));

            // x. Set k to k + 1.
        }
    }

    // 6. NOTE: items is not an Iterable so assume it is an array-like object.

    // 7. Let arrayLike be ! ToObject(items).
    auto array_like = MUST(items.to_object(vm));

    // 8. Let len be ? LengthOfArrayLike(arrayLike).
    auto length = TRY(length_of_array_like(vm, array_like));

    GCPtr<Object> array;

    // 9. If IsConstructor(C) is true, then
    if (constructor.is_constructor()) {
        // a. Let A be ? Construct(C, « 𝔽(len) »).
        array = TRY(JS::construct(vm, constructor.as_function(), Value(length)));
    } else {
        // a. Let A be ? ArrayCreate(len).
        array = TRY(Array::create(realm, length));
    }

    // 11. Let k be 0.
    // 12. Repeat, while k < len,
    for (size_t k = 0; k < length; ++k) {
        // a. Let Pk be ! ToString(𝔽(k)).
        auto property_key = PropertyKey { k };

        // b. Let kValue be ? Get(arrayLike, Pk).
        auto k_value = TRY(array_like->get(property_key));

        Value mapped_value;

        // c. If mapping is true, then
        if (mapfn) {
            // i. Let mappedValue be ? Call(mapfn, thisArg, « kValue, 𝔽(k) »).
            mapped_value = TRY(JS::call(vm, *mapfn, this_arg, k_value, Value(k)));
        }
        // d. Else, let mappedValue be kValue.
        else {
            mapped_value = k_value;
        }

        // e. Perform ? CreateDataPropertyOrThrow(A, Pk, mappedValue).
        TRY(array->create_data_property_or_throw(property_key, mapped_value));

        // f. Set k to k + 1.
    }

    // 13. Perform ? Set(A, "length", 𝔽(len), true).
    TRY(array->set(vm.names.length, Value(length), Object::ShouldThrowExceptions::Yes));

    // 14. Return A.
    return array;
}

// 23.1.2.2 Array.isArray ( arg ), https://tc39.es/ecma262/#sec-array.isarray
JS_DEFINE_NATIVE_FUNCTION(ArrayConstructor::is_array)
{
    auto arg = vm.argument(0);

    // 1. Return ? IsArray(arg).
    return Value(TRY(arg.is_array(vm)));
}

// 23.1.2.3 Array.of ( ...items ), https://tc39.es/ecma262/#sec-array.of
JS_DEFINE_NATIVE_FUNCTION(ArrayConstructor::of)
{
    auto& realm = *vm.current_realm();

    // 1. Let len be the number of elements in items.
    auto len = vm.argument_count();

    // 2. Let lenNumber be 𝔽(len).
    auto len_number = Value(len);

    // 3. Let C be the this value.
    auto constructor = vm.this_value();

    GCPtr<Object> array;

    // 4. If IsConstructor(C) is true, then
    if (constructor.is_constructor()) {
        // a. Let A be ? Construct(C, « lenNumber »).
        array = TRY(JS::construct(vm, constructor.as_function(), Value(vm.argument_count())));
    } else {
        // a. Let A be ? ArrayCreate(len).
        array = TRY(Array::create(realm, len));
    }

    // 6. Let k be 0.
    // 7. Repeat, while k < len,
    for (size_t k = 0; k < len; ++k) {
        // a. Let kValue be items[k].
        auto k_value = vm.argument(k);

        // b. Let Pk be ! ToString(𝔽(k)).
        auto property_key = PropertyKey { k };

        // c. Perform ? CreateDataPropertyOrThrow(A, Pk, kValue).
        TRY(array->create_data_property_or_throw(property_key, k_value));

        // d. Set k to k + 1.
    }

    // 8. Perform ? Set(A, "length", lenNumber, true).
    TRY(array->set(vm.names.length, len_number, Object::ShouldThrowExceptions::Yes));

    // 9. Return A.
    return array;
}

// 23.1.2.5 get Array [ @@species ], https://tc39.es/ecma262/#sec-get-array-@@species
JS_DEFINE_NATIVE_FUNCTION(ArrayConstructor::symbol_species_getter)
{
    // 1. Return the this value.
    return vm.this_value();
}

}
