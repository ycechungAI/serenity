#pragma once

#include <AK/AKString.h>

class JsonArray;
class JsonObject;

class JsonValue {
public:
    enum class Type {
        Undefined,
        Null,
        Int,
        Double,
        Bool,
        String,
        Array,
        Object,
    };

    explicit JsonValue(Type = Type::Null);
    ~JsonValue() { clear(); }

    JsonValue(const JsonValue&);
    JsonValue(JsonValue&&);

    JsonValue& operator=(const JsonValue&);
    JsonValue& operator=(JsonValue&&);

    JsonValue(int);
    JsonValue(double);
    JsonValue(bool);
    JsonValue(const String&);
    JsonValue(const JsonArray&);
    JsonValue(const JsonObject&);

    String to_string() const;

private:
    void clear();
    void copy_from(const JsonValue&);

    Type m_type { Type::Undefined };

    union {
        StringImpl* as_string { nullptr };
        JsonArray* as_array;
        JsonObject* as_object;
        double as_double;
        int as_int;
        bool as_bool;
    } m_value;
};
