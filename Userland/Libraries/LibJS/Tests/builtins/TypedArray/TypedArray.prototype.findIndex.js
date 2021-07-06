const TYPED_ARRAYS = [
    Uint8Array,
    Uint8ClampedArray,
    Uint16Array,
    Uint32Array,
    Int8Array,
    Int16Array,
    Int32Array,
    Float32Array,
    Float64Array,
];

const BIGINT_TYPED_ARRAYS = [BigUint64Array, BigInt64Array];

test("length is 1", () => {
    TYPED_ARRAYS.forEach(T => {
        expect(T.prototype.findIndex).toHaveLength(1);
    });

    BIGINT_TYPED_ARRAYS.forEach(T => {
        expect(T.prototype.findIndex).toHaveLength(1);
    });
});

describe("errors", () => {
    function errorTests(T) {
        test(`requires at least one argument (${T.name})`, () => {
            expect(() => {
                new T().findIndex();
            }).toThrowWithMessage(
                TypeError,
                "TypedArray.prototype.findIndex() requires at least one argument"
            );
        });

        test(`callback must be a function (${T.name})`, () => {
            expect(() => {
                new T().findIndex(undefined);
            }).toThrowWithMessage(TypeError, "undefined is not a function");
        });
    }

    TYPED_ARRAYS.forEach(T => errorTests(T));
    BIGINT_TYPED_ARRAYS.forEach(T => errorTests(T));
});

describe("normal behaviour", () => {
    test("basic functionality", () => {
        TYPED_ARRAYS.forEach(T => {
            const typedArray = new T([1, 2, 3]);

            expect(typedArray.findIndex(value => value === 1)).toBe(0);
            expect(typedArray.findIndex(value => value === 2)).toBe(1);
            expect(typedArray.findIndex(value => value === 3)).toBe(2);
            expect(typedArray.findIndex((value, index) => index === 1)).toBe(1);
            expect(typedArray.findIndex(value => value == "1")).toBe(0);
            expect(typedArray.findIndex(value => value === 10)).toBe(-1);
        });

        BIGINT_TYPED_ARRAYS.forEach(T => {
            const typedArray = new T([1n, 2n, 3n]);

            expect(typedArray.findIndex(value => value === 1n)).toBe(0);
            expect(typedArray.findIndex(value => value === 2n)).toBe(1);
            expect(typedArray.findIndex(value => value === 3n)).toBe(2);
            expect(typedArray.findIndex((value, index) => index === 1)).toBe(1);
            expect(typedArray.findIndex(value => value == 1)).toBe(0);
            expect(typedArray.findIndex(value => value == "1")).toBe(0);
            expect(typedArray.findIndex(value => value === 1)).toBe(-1);
        });
    });

    test("never calls callback with empty array", () => {
        function emptyTest(T) {
            var callbackCalled = 0;
            expect(
                new T().findIndex(() => {
                    callbackCalled++;
                })
            ).toBe(-1);
            expect(callbackCalled).toBe(0);
        }

        TYPED_ARRAYS.forEach(T => emptyTest(T));
        BIGINT_TYPED_ARRAYS.forEach(T => emptyTest(T));
    });

    test("calls callback once for every item", () => {
        TYPED_ARRAYS.forEach(T => {
            var callbackCalled = 0;
            expect(
                new T([1, 2, 3]).findIndex(() => {
                    callbackCalled++;
                })
            ).toBe(-1);
            expect(callbackCalled).toBe(3);
        });

        BIGINT_TYPED_ARRAYS.forEach(T => {
            var callbackCalled = 0;
            expect(
                new T([1n, 2n, 3n]).findIndex(() => {
                    callbackCalled++;
                })
            ).toBe(-1);
            expect(callbackCalled).toBe(3);
        });
    });
});
