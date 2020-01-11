#pragma once

//==============================================================================
// QJson 1.1.0
// Austin Quick
// July 2019
//------------------------------------------------------------------------------
// Basic, lightweight JSON decoder.
//
// Example:
//      qjson::Object root(qjson::decode(myJsonString));
//      const std::string & name(root["Price"]->asString());
//      const qjson::Array & favoriteBooks(root["Favorite Books"]->asArray());
//      const std::string & bookTitle(favoriteBooks[0]->asString());
//      ...
//------------------------------------------------------------------------------

#include <string>
#include <map>
#include <vector>
#include <memory>
#include <stdexcept>
#include <deque>

#include "QJsonDecode.hpp"
#include "QJsonEncode.hpp"

namespace qjson {

    constexpr uint32_t ceil2(uint32_t v) {
        --v;
        v |= v >>  1;
        v |= v >>  2;
        v |= v >>  4;
        v |= v >>  8;
        v |= v >> 16;
        return v + 1;
    }

    using std::string;
    using std::string_view;
    using std::unique_ptr;
    using namespace std::string_literals;
    using namespace std::string_view_literals;

    // This will be thrown when attempting to access as value as the wrong type
    struct TypeError : public Error {};

    enum class Type : uint32_t {
           null = 0b00000000'00000000'00000000'00000000u,
         object = 0b00100000'00000000'00000000'00000000u,
          array = 0b01000000'00000000'00000000'00000000u,
         string = 0b01100000'00000000'00000000'00000000u,
        integer = 0b10000000'00000000'00000000'00000000u,
        floater = 0b10100000'00000000'00000000'00000000u,
        boolean = 0b11000000'00000000'00000000'00000000u,
            hex = 0b11100000'00000000'00000000'00000000u
    };

    constexpr bool isPow2(uint32_t v) {
        return (v & (v - 1)) == 0;
    }

    class Object;
    class Array;
    class String;

    class Value {

        public:

        Value() = default;
        Value(const Value &) = delete;
        Value(Value && other);
        Value(Object && val);
        Value(Array && val);
        Value(String && val);
        Value(string_view val);
        Value(const string & val);
        Value(const char * val);
        Value(char * val);
        Value(char val);
        Value(int64_t val);
        Value(int32_t val);
        Value(int16_t val);
        Value(int8_t val);
        Value(uint64_t val);
        Value(uint32_t val);
        Value(uint16_t val);
        Value(uint8_t val);
        Value(double val);
        Value(float val);
        Value(bool val);
        Value(nullptr_t);
        template <typename T> Value(const T & val);

        Value & operator=(const Value &) = delete;
        Value & operator=(Value && other);

        ~Value();

        Type type() const;

        bool is(Type t) const;

        template <bool unsafe = false> const Object & asObject() const;
        template <bool unsafe = false>       Object & asObject();

        template <bool unsafe = false> const Array & asArray() const;
        template <bool unsafe = false>       Array & asArray();

        template <bool unsafe = false> const String & asString() const;
        template <bool unsafe = false>       String & asString();

        template <bool unsafe = false> const int64_t & asInteger() const;
        template <bool unsafe = false>       int64_t & asInteger();

        template <bool unsafe = false> const double & asFloater() const;
        template <bool unsafe = false>       double & asFloater();

        template <bool unsafe = false> const bool & asBoolean() const;
        template <bool unsafe = false>       bool & asBoolean();

        template <bool unsafe = false> const uint64_t & asHex() const;
        template <bool unsafe = false>       uint64_t & asHex();

        template <typename T, bool unsafe = false> T as() const;

        private:

        uint32_t m_type_data0{0};
        uint32_t m_data1{0};
        union {
            uint64_t m_data2{0};
            int64_t m_integer;
            double m_floater;
            bool m_boolean;
            uint64_t m_hex;
        };

    };

    class Object {

        public:

        using Pair = std::pair<string, Value>;

        Object() = default;

        Object(const Object &) = delete;

        Object(Object && other) :
            m_type_capacity(other.m_type_capacity),
            m_size(other.m_size),
            m_pairs(other.m_pairs)
        {
            other.m_type_capacity = uint32_t(Type::object);
            other.m_size = 0;
            other.m_pairs = nullptr;
        }

        template <typename... Ts>
        Object(std::pair<std::string, Ts>... pairs) :
            m_type_capacity(uint32_t(Type::object)), // For explicitly populated objects, capacity is 0 (avoiding unneccessary allocation)
            m_size(sizeof...(pairs)),
            m_pairs(::operator new(m_size * sizeof(Pair)))
        {
            // Populate `m_pairs` using fold expression
            int index(0);
            auto f([&index](std::string key, auto val) { new (m_pairs + index++) Pair(std::move(key), std::move(val)); });
            (f(std::move(pairs.first), std::move(pairs.second)), ...);
        }

        Object & operator=(const Object &) = delete;

        Object & operator=(Object && other) {
            m_type_capacity = other.m_type_capacity;
            m_size = other.m_size;
            m_pairs = other.m_pairs;
            other.m_type_capacity = uint32_t(Type::object);
            other.m_size = 0;
            other.m_pairs = nullptr;
            return *this;
        }

        ~Object() {
            if (m_size > 0) {
                for (uint32_t i(0); i < m_size; ++i) m_pairs[i].~pair();
                ::operator delete(m_pairs);
            }
        }

        uint32_t size() const {
            return m_size;
        }

        bool empty() const {
            return m_size == 0;
        }

        const Value & operator[](string_view key) const {
            auto [pos, found](m_find(key));
            if (!found) {
                throw std::out_of_range("Key not found");
            }
            return pos->second;
        }

        Value & operator[](string_view key) {
            return const_cast<Value &>(const_cast<const Object &>(*this)[key]);
        }

        Pair & add(string && key, Value && val) {
            if (m_size == 0) {
                m_pairs = static_cast<Pair *>(::operator new(8 * sizeof(Pair)));
                m_type_capacity = uint32_t(Type::object) | 1u;
            }

            auto [pos, found](m_find(key));

            // If key already exists, replace its value
            if (found) {
                pos->second.~Value();
                pos->first = std::move(key);
                new (&pos->second) Value(std::move(val));
                return *pos;
            }

            // If we're at capacity, expand
            if (uint32_t capacity(m_type_capacity << 3); m_size >= capacity) {
                uint32_t newCapacity(capacity == 0 ? (m_size <= 4 ? 8 : ceil2(m_size << 1)) : capacity << 1);
                Pair * newPairs(static_cast<Pair *>(::operator new(newCapacity * sizeof(Pair))));
                Pair * newPos(newPairs + (pos - m_pairs));
                // Copy the pairs before the one we're inserting
                std::copy(reinterpret_cast<const uint64_t *>(m_pairs), reinterpret_cast<const uint64_t *>(pos), reinterpret_cast<uint64_t *>(newPairs));
                // Copy the pairs after the one we're inserting, leaving a gap
                std::copy(reinterpret_cast<const uint64_t *>(pos), reinterpret_cast<const uint64_t *>(end()), reinterpret_cast<uint64_t *>(newPos + 1));
                ::operator delete(m_pairs);
                m_pairs = newPairs;
                m_type_capacity = uint32_t(Type::object) | (newCapacity >> 3);
                pos = newPos;
            }
            // We've still got space
            else {
                // Shift back the pairs after the one we're inserting, leaving a gap
                Pair * endPos(end());
                std::copy_backward(reinterpret_cast<uint64_t *>(pos), reinterpret_cast<uint64_t *>(endPos), reinterpret_cast<uint64_t *>(endPos + 1));
            }

            new (&pos->first) string(std::move(key));
            new (&pos->second) Value(std::move(val));
            ++m_size;
            return *pos;
        }

        using iterator = Pair *;
        using const_iterator = const Pair *;

        iterator begin() {
            return m_pairs;
        }

        const_iterator begin() const {
            return m_pairs;
        }

        const_iterator cbegin() const {
            return m_pairs;
        }

        iterator end() {
            return m_pairs + m_size;
        }

        const_iterator end() const {
            return m_pairs + m_size;
        }

        const_iterator cend() const {
            return m_pairs + m_size;
        }

        private:

        uint32_t m_type_capacity{uint32_t(Type::object)};
        uint32_t m_size{0};
        Pair * m_pairs{nullptr};

        std::pair<const Pair *, bool> m_find(string_view key) const {
            const Pair * endPos(end());
            const Pair * low(m_pairs), * high(endPos);
            while (low < high) {
                const Pair * mid(low + ((high - low) >> 1));
                int delta(std::strcmp(key.data(), mid->first.c_str())); // TODO: replace with spaceship operator
                if (delta < 0) {
                    high = mid;
                }
                else if (delta > 0) {
                    low = mid + 1;
                }
                else {
                    return {mid, true};
                }
            }
            return {low, low != endPos && low->first == key};
        }

        std::pair<Pair *, bool> m_find(string_view key) {
            auto [pos, found](const_cast<const Object *>(this)->m_find(key));
            return {const_cast<Pair *>(pos), found};
        }

    };

    class Array {

        public:

        Array() = default;

        Array(const Array & other) = delete;

        Array(Array && other) :
            m_type_capacity(other.m_type_capacity),
            m_size(other.m_size),
            m_values(other.m_values)
        {
            other.m_type_capacity = uint32_t(Type::array);
            other.m_size = 0;
            other.m_values = nullptr;
        }

        template <typename... Ts>
        Array(Ts &&... vals) :
            m_type_capacity(uint32_t(Type::array)), // For explicitly populated arrays, capacity is 0 (avoiding unneccessary allocation)
            m_size(sizeof...(vals)),
            m_values(static_cast<Value *>(::operator new(m_size * sizeof(Value))))
        {
            // Populate `m_values` using fold expression
            int index(0);
            auto f([this, &index](auto && val) { new (m_values + index++) Value(std::forward<decltype(val)>(val)); });
            (f(std::forward<Ts>(vals)), ...);
        }

        Array & operator=(const Array &) = delete;

        Array & operator=(Array && other) {
            m_type_capacity = other.m_type_capacity;
            m_size = other.m_size;
            m_values = other.m_values;
            other.m_type_capacity = uint32_t(Type::array);
            other.m_size = 0;
            other.m_values = nullptr;
            return *this;
        }

        ~Array() {
            if (m_size > 0) {
                for (uint32_t i(0); i < m_size; ++i) m_values[i].~Value();
                ::operator delete(m_values);
            }
        }

        uint32_t size() const {
            return m_size;
        }

        bool empty() const {
            return m_size == 0;
        }

        const Value & operator[](uint32_t i) const {
            return m_values[i];
        }

        Value & operator[](uint32_t i) {
            return m_values[i];
        }

        Value & add(Value && val) {
            // If this is the first value, allocate initial storage
            if (m_size == 0) {
                m_values = static_cast<Value *>(::operator new(8 * sizeof(Value)));
                m_type_capacity = uint32_t(Type::array) | 1u;
            }
            // If we're at capacity, expand
            else if (uint32_t capacity(m_type_capacity << 3); m_size >= capacity) {
                uint32_t newCapacity(capacity == 0 ? (m_size <= 4 ? 8 : ceil2(m_size << 1)) : capacity << 1);
                Value * newValues(static_cast<Value *>(::operator new(newCapacity * sizeof(Value))));
                std::copy(reinterpret_cast<const uint64_t *>(m_values), reinterpret_cast<const uint64_t *>(end()), reinterpret_cast<uint64_t *>(newValues));
                ::operator delete(m_values);
                m_values = newValues;
                m_type_capacity = uint32_t(Type::array) | (newCapacity >> 3);
            }

            return *(new (m_values + m_size++) Value(std::move(val)));
        }

        using iterator = Value *;
        using const_iterator = const Value *;

        iterator begin() {
            return m_values;
        }

        const_iterator begin() const {
            return m_values;
        }

        const_iterator cbegin() const {
            return m_values;
        }

        iterator end() {
            return m_values + m_size;
        }

        const_iterator end() const {
            return m_values + m_size;
        }

        const_iterator cend() const {
            return m_values + m_size;
        }

        private:

        uint32_t m_type_capacity{uint32_t(Type::array)};
        uint32_t m_size{0};
        alignas(8) Value * m_values{nullptr};

    };

    class String {

        public:

        String() = default;

        String(const String &) = delete;

        String(String && other) :
            m_type_size(other.m_type_size),
            m_inlineChars(other.m_inlineChars),
            m_dynamicChars(other.m_dynamicChars)
        {
            other.m_type_size = uint32_t(Type::string);
            other.m_inlineChars = 0;
            other.m_dynamicChars = nullptr;
        }

        String(std::string_view str) :
            m_type_size(uint32_t(Type::string) | uint32_t(str.size())),
            m_inlineChars(),
            m_dynamicChars()
        {
            if (str.size() <= 12) {
                std::copy(str.cbegin(), str.cend(), &m_inlineChars);
            }
            else {
                m_dynamicChars = static_cast<char *>(::operator new(str.size()));
                std::copy(str.cbegin(), str.cend(), m_dynamicChars);
            }
        }

        String(char c) :
            m_type_size(uint32_t(Type::string) | 1u),
            m_inlineChars(),
            m_dynamicChars()
        {
            *reinterpret_cast<char *>(&m_inlineChars) = c;
        }

        String & operator=(const String &) = delete;

        String & operator=(String && other) {
            m_type_size = other.m_type_size;
            m_inlineChars = other.m_inlineChars;
            m_dynamicChars = other.m_dynamicChars;
            other.m_type_size = uint32_t(Type::string);
            other.m_inlineChars = 0;
            other.m_dynamicChars = nullptr;
            return *this;
        }

        ~String() {
            if (size() > 12) ::operator delete(m_dynamicChars);
        }

        uint32_t size() const {
            return m_type_size & 0b00011111'11111111'11111111'11111111u;
        }

        bool empty() const {
            return size() == 0;
        }

        string_view view() const {
            uint32_t size(size());
            return {size > 12 ? m_dynamicChars : reinterpret_cast<const char *>(&m_inlineChars), size};
        }

        private:

        uint32_t m_type_size{uint32_t(Type::string)};
        uint32_t m_inlineChars{};
        alignas(8) char * m_dynamicChars{};

    };

    Value decode(string_view json);

    string encode(const Value & val, bool compact = k_defaultCompact, int indentSize = k_defaultIndentSize);

}

// Specialize `qjson_valueTo` to enable Value::as for custom types
// Example:
//      template <bool unsafe>
//      struct qjson_valueTo<std::pair<int, int>, unsafe> {
//          std::pair<int, int> operator()(const qjson::Value & v) {
//              const Array & arr(v.asArray<unsafe>());
//              return {arr.at(0)->asInteger<unsafe>(), arr.at(1)->asInteger<unsafe>()};
//          }
//      };
//
template <typename T, bool unsafe> struct qjson_valueTo;

// Specialize `qjson_valueFrom` to enable value construction for custom types
// Example:
//      template <>
//      struct qjson_valueFrom<std::pair<int, int>> {
//          qjson::Value operator()(const std::pair<int, int> & v) {
//              return qjson::Array(v.first, f.second);
//          }
//      };
//
template <typename T> struct qjson_valueFrom;

// IMPLEMENTATION //////////////////////////////////////////////////////////////////////////////////////////////////////

namespace qjson {

    namespace detail {

        class Decoder {

            public:

            struct State {
                Value * node;
                bool isObject;
                bool isArray;
            };

            State object(State & outerState) {
                Value * innerNode;
                if (outerState.isObject) innerNode = &outerState.node->asObject().add(std::move(m_key), Object()).second;
                else if (outerState.isArray) innerNode = &outerState.node->asArray().add(Object());
                else innerNode = &(*outerState.node = Object());
                return {innerNode, true, false};
            }

            State array(State & outerState) {
                Value * innerNode;
                if (outerState.isObject) innerNode = &outerState.node->asObject().add(std::move(m_key), Array()).second;
                else if (outerState.isArray) innerNode = &outerState.node->asArray().add(Array());
                else innerNode = &(*outerState.node = Array());
                return {innerNode, false, true};
            }

            void key(std::string && k, State & state) {
                m_key = std::move(k);
            }

            void end(State && innerState, State & outerState) {}

            template <typename T>
            void val(T v, State & state) {
                if (state.isObject) state.node->asObject().add(std::move(m_key), v);
                else if (state.isArray) state.node->asArray().add(v);
                else *state.node = v;
            }

            private:

            std::string m_key;

        };

        inline void encodeRecursive(Encoder & encoder, const Value & val) {
            switch (val.type()) {
                case Type::null: {
                    encoder.val(nullptr);
                    break;
                }
                case Type::object: {
                    encoder.object();
                    for (const auto & [key, v] : val.asObject()) {
                        encoder.key(key);
                        encodeRecursive(encoder, v);
                    }
                    encoder.end();
                    break;
                }
                case Type::array: {
                    encoder.array();
                    for (const auto & v : val.asArray()) {
                        encodeRecursive(encoder, v);
                    }
                    encoder.end();
                    break;
                }
                case Type::string: {
                    encoder.val(val.asString().view());
                    break;
                }
                case Type::integer: {
                    encoder.val(val.asInteger());
                    break;
                }
                case Type::floater: {
                    encoder.val(val.asFloater());
                    break;
                }
                case Type::boolean: {
                    encoder.val(val.asBoolean());
                    break;
                }
                case Type::hex: {
                    encoder.val(val.asHex());
                    break;
                }
            }
        }

    }

    using namespace detail;

    inline Value::Value(Value && other) :
        m_type_data0(other.m_type_data0),
        m_data1(other.m_data1),
        m_data2(other.m_data2)
    {
        other.m_type_data0 = 0;
        other.m_data1 = 0;
        other.m_data2 = 0;
    }

    inline Value::Value(Object && val) :
        Value(reinterpret_cast<Value &&>(val))
    {}

    inline Value::Value(Array && val) :
        Value(reinterpret_cast<Value &&>(val))
    {}

    inline Value::Value(String && val) :
        Value(reinterpret_cast<Value &&>(val))
    {}

    inline Value::Value(string_view val) :
        Value(String(val))
    {}

    inline Value::Value(const string & val) :
        Value(String(val))
    {}

    inline Value::Value(const char * val) :
        Value(String(val))
    {}

    inline Value::Value(char * val) :
        Value(String(val))
    {}

    inline Value::Value(char val) :
        Value(String(val))
    {}

    inline Value::Value(int64_t val) :
        m_type_data0(uint32_t(Type::integer)),
        m_data1(),
        m_integer(val)
    {}

    inline Value::Value(int32_t val) :
        Value(int64_t(val))
    {}

    inline Value::Value(int16_t val) :
        Value(int64_t(val))
    {}

    inline Value::Value(int8_t val) :
        Value(int64_t(val))
    {}

    inline Value::Value(uint64_t val) :
        m_type_data0(uint32_t(Type::hex)),
        m_data1(),
        m_hex(val)
    {}

    inline Value::Value(uint32_t val) :
        Value(uint64_t(val))
    {}

    inline Value::Value(uint16_t val) :
        Value(uint64_t(val))
    {}

    inline Value::Value(uint8_t val) :
        Value(uint64_t(val))
    {}

    inline Value::Value(double val) :
        m_type_data0(uint32_t(Type::floater)),
        m_data1(),
        m_floater(val)
    {}

    inline Value::Value(float val) :
        Value(double(val))
    {}

    inline Value::Value(bool val) :
        m_type_data0(uint32_t(Type::boolean)),
        m_data1(),
        m_boolean(val)
    {}

    inline Value::Value(nullptr_t) :
        Value()
    {}

    template <typename T>
    inline Value::Value(const T & val) :
        Value(qjson_valueFrom<T>()(val))
    {}

    inline Value & Value::operator=(Value && other) {
        m_type_data0 = other.m_type_data0;
        m_data1 = other.m_data1;
        m_data2 = other.m_data2;
        other.m_type_data0 = 0;
        other.m_data1 = 0;
        other.m_data2 = 0;
        return *this;
    }

    inline Value::~Value() {
        switch (type()) {
            case Type::object:
                reinterpret_cast<Object *>(this)->~Object();
                break;
            case Type::array:
                reinterpret_cast<Array *>(this)->~Array();
                break;
            case Type::string:
                reinterpret_cast<String *>(this)->~String();
                break;
        }
    }

    inline Type Value::type() const {
        return Type(m_type_data0 & 0b111'00000'00000000'00000000u);
    }

    inline bool Value::is(Type t) const {
        return type() == t;
    }

    template <bool unsafe>
    inline const Object & Value::asObject() const {
        if constexpr (!unsafe) if (type() != Type::object) throw TypeError();
        return *reinterpret_cast<const Object *>(this);
    }

    template <bool unsafe>
    inline Object & Value::asObject() {
        if constexpr (!unsafe) if (type() != Type::object) throw TypeError();
        return *reinterpret_cast<Object *>(this);
    }

    template <bool unsafe>
    inline const Array & Value::asArray() const {
        if constexpr (!unsafe) if (type() != Type::array) throw TypeError();
        return *reinterpret_cast<const Array *>(this);
    }

    template <bool unsafe>
    inline Array & Value::asArray() {
        if constexpr (!unsafe) if (type() != Type::array) throw TypeError();
        return *reinterpret_cast<Array *>(this);
    }

    template <bool unsafe>
    inline const String & Value::asString() const {
        if constexpr (!unsafe) if (type() != Type::string) throw TypeError();
        return *reinterpret_cast<const String *>(this);
    }

    template <bool unsafe>
    inline String & Value::asString() {
        if constexpr (!unsafe) if (type() != Type::string) throw TypeError();
        return *reinterpret_cast<String *>(this);
    }

    template <bool unsafe>
    inline const int64_t & Value::asInteger() const {
        if constexpr (!unsafe) if (type() != Type::integer) throw TypeError();
        return m_integer;
    }

    template <bool unsafe>
    inline int64_t & Value::asInteger() {
        if constexpr (!unsafe) if (type() != Type::integer) throw TypeError();
        return m_integer;
    }

    template <bool unsafe>
    inline const double & Value::asFloater() const {
        if constexpr (!unsafe) if (type() != Type::floater) throw TypeError();
        return m_floater;
    }

    template <bool unsafe>
    inline double & Value::asFloater() {
        if constexpr (!unsafe) if (type() != Type::floater) throw TypeError();
        return m_floater;
    }

    template <bool unsafe>
    inline const bool & Value::asBoolean() const {
        if constexpr (!unsafe) if (type() != Type::boolean) throw TypeError();
        return m_boolean;
    }

    template <bool unsafe>
    inline bool & Value::asBoolean() {
        if constexpr (!unsafe) if (type() != Type::boolean) throw TypeError();
        return m_boolean;
    }

    template <bool unsafe>
    inline const uint64_t & Value::asHex() const {
        if constexpr (!unsafe) if (type() != Type::hex) throw TypeError();
        return m_hex;
    }

    template <bool unsafe>
    inline uint64_t & Value::asHex() {
        if constexpr (!unsafe) if (type() != Type::hex) throw TypeError();
        return m_hex;
    }

    template <typename T, bool unsafe>
    inline T Value::as() const {
        return qjson_valueTo<T, unsafe>()(*this);
    }

    template <>
    inline string_view Value::as<string_view, false>() const {
        return asString<false>().view();
    }

    template <>
    inline string_view Value::as<string_view, true>() const {
        return asString<true>().view();
    }

    template <>
    inline int64_t Value::as<int64_t, false>() const {
        return asInteger<false>();
    }

    template <>
    inline int64_t Value::as<int64_t, true>() const {
        return asInteger<true>();
    }

    template <>
    inline int32_t Value::as<int32_t, false>() const {
        return int32_t(asInteger<false>());
    }

    template <>
    inline int32_t Value::as<int32_t, true>() const {
        return int32_t(asInteger<true>());
    }

    template <>
    inline int16_t Value::as<int16_t, false>() const {
        return int16_t(asInteger<false>());
    }

    template <>
    inline int16_t Value::as<int16_t, true>() const {
        return int16_t(asInteger<true>());
    }

    template <>
    inline int8_t Value::as<int8_t, false>() const {
        return int8_t(asInteger<false>());
    }

    template <>
    inline int8_t Value::as<int8_t, true>() const {
        return int8_t(asInteger<true>());
    }

    template <>
    inline uint64_t Value::as<uint64_t, false>() const {
        return uint64_t(asInteger<false>());
    }

    template <>
    inline uint64_t Value::as<uint64_t, true>() const {
        return uint64_t(asInteger<true>());
    }

    template <>
    inline uint32_t Value::as<uint32_t, false>() const {
        return uint32_t(asInteger<false>());
    }

    template <>
    inline uint32_t Value::as<uint32_t, true>() const {
        return uint32_t(asInteger<true>());
    }

    template <>
    inline uint16_t Value::as<uint16_t, false>() const {
        return uint16_t(asInteger<false>());
    }

    template <>
    inline uint16_t Value::as<uint16_t, true>() const {
        return uint16_t(asInteger<true>());
    }

    template <>
    inline uint8_t Value::as<uint8_t, false>() const {
        return uint8_t(asInteger<false>());
    }

    template <>
    inline uint8_t Value::as<uint8_t, true>() const {
        return uint8_t(asInteger<true>());
    }

    template <>
    inline double Value::as<double, false>() const {
        return asFloater<false>();
    }

    template <>
    inline double Value::as<double, true>() const {
        return asFloater<true>();
    }

    template <>
    inline float Value::as<float, false>() const {
        return float(asFloater<false>());
    }

    template <>
    inline float Value::as<float, true>() const {
        return float(asFloater<true>());
    }

    template <>
    inline bool Value::as<bool, false>() const {
        return asBoolean<false>();
    }

    template <>
    inline bool Value::as<bool, true>() const {
        return asBoolean<true>();
    }

    inline Value decode(string_view json) {
        Value root;
        Decoder decoder;
        decode(json, decoder, Decoder::State{&root, false, false});
        return root;
    }

    inline string encode(const Value & val, bool compact, int indentSize) {
        Encoder encoder(compact, indentSize);
        encodeRecursive(encoder, val);
        return encoder.finish();
    }

}
