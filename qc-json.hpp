#pragma once

//
// QJson 1.1.0
// Austin Quick
// July 2019 - February 2020
//
// Basic, lightweight JSON decoder.
//
// Example:
//      qc::json::Object root(qc::json::decode(myJsonString));
//      const std::string & name(root["Price"]->asString());
//      const qc::json::Array & favoriteBooks(root["Favorite Books"]->asArray());
//      const std::string & bookTitle(favoriteBooks[0]->asString());
//      ...
//

#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include "qc-json-decode.hpp"
#include "qc-json-encode.hpp"

namespace qc {

    namespace json {

        using std::string;
        using std::string_view;
        using namespace std::string_literals;
        using namespace std::string_view_literals;

        //
        // This will be thrown when attempting to access a value as the wrong type.
        //
        struct TypeError : public std::runtime_error {

            TypeError() : std::runtime_error(nullptr) {}

        };

        enum class Type : uint32_t {
            null = 0b000'00000'00000000'00000000'00000000u,
            object = 0b001'00000'00000000'00000000'00000000u,
            array = 0b010'00000'00000000'00000000'00000000u,
            string = 0b011'00000'00000000'00000000'00000000u,
            integer = 0b100'00000'00000000'00000000'00000000u,
            floater = 0b101'00000'00000000'00000000'00000000u,
            boolean = 0b110'00000'00000000'00000000'00000000u
        };

        class Object;
        class Array;

        class Value {

            public:

            Value() = default;
            Value(Object && val);
            Value(Array && val);
            Value(string_view val);
            Value(const string & val);
            Value(const char * val);
            Value(char * val);
            Value(char val);
            Value(int64_t val);
            Value(uint64_t val);
            Value(int32_t val);
            Value(uint32_t val);
            Value(int16_t val);
            Value(uint16_t val);
            Value(int8_t val);
            Value(uint8_t val);
            Value(double val);
            Value(float val);
            Value(bool val);
            Value(nullptr_t);
            template <typename T> Value(const T & val);

            Value(const Value &) = delete;
            Value(Value && other);

            Value & operator=(const Value &) = delete;
            Value & operator=(Value && other);

            ~Value();

            Type type() const;

            bool is(Type t) const;

            template <bool unsafe = false> const Object & asObject() const;
            template <bool unsafe = false>       Object & asObject();

            template <bool unsafe = false> const Array & asArray() const;
            template <bool unsafe = false>       Array & asArray();

            template <bool unsafe = false> std::string_view asString() const;

            template <bool unsafe = false> const int64_t & asInteger() const;
            template <bool unsafe = false>       int64_t & asInteger();

            template <bool unsafe = false> const double & asFloater() const;
            template <bool unsafe = false>       double & asFloater();

            template <bool unsafe = false> const bool & asBoolean() const;
            template <bool unsafe = false>       bool & asBoolean();

            template <bool unsafe = false> nullptr_t asNull() const;

            template <typename T, bool unsafe = false> T as() const;

            private:

            uint32_t m_type_data0{0};
            uint32_t m_data1{0};
            union {
                uint64_t m_data2{0};
                int64_t m_integer;
                double m_floater;
                bool m_boolean;
            };

        };

        class Object {

            public:

            using Pair = std::pair<string, Value>;

            using iterator = Pair *;
            using const_iterator = const Pair *;

            Object() = default;

            Object(const Object &) = delete;
            Object(Object && other);

            Object & operator=(const Object &) = delete;
            Object & operator=(Object && other);

            ~Object();

            uint32_t size() const;

            uint32_t capacity() const;

            bool empty() const;

            Pair & add(string && key, Value && val);

            bool contains(string_view key) const;

            const Value & at(string_view key) const;
            Value & at(string_view key);

            const_iterator find(string_view key) const;
            iterator find(string_view key);

            Pair remove(iterator it);

            void clear();

            const_iterator begin() const;
            iterator begin();

            const_iterator cbegin() const;

            const_iterator end() const;
            iterator end();

            const_iterator cend() const;

            private:

            uint32_t m_type_capacity{uint32_t(Type::object)};
            uint32_t m_size{0};
            alignas(8) Pair * m_pairs{nullptr};

            std::pair<const Pair *, bool> m_search(string_view key) const;
            std::pair<Pair *, bool> m_search(string_view key);

        };

        class Array {

            public:

            using iterator = Value *;
            using const_iterator = const Value *;

            Array() = default;
            template <typename T, typename... Ts> Array(T && val, Ts &&... vals);

            Array(const Array & other) = delete;
            Array(Array && other);

            Array & operator=(const Array &) = delete;
            Array & operator=(Array && other);

            ~Array();

            uint32_t size() const;

            uint32_t capacity() const;

            bool empty() const;

            Value & add(Value && val);

            const Value & at(uint32_t i) const;
            Value & at(uint32_t i);

            Value remove(uint32_t i);
            Value remove(iterator it);
            void remove(iterator it1, iterator it2);

            void clear();

            const_iterator begin() const;
            iterator begin();

            const_iterator cbegin() const;

            const_iterator end() const;
            iterator end();

            const_iterator cend() const;

            private:

            uint32_t m_type_capacity{uint32_t(Type::array)};
            uint32_t m_size{0};
            alignas(8) Value * m_values{nullptr};

        };

        class String {

            public:

            String(std::string_view str);

            String(const String &) = delete;
            String(String && other);

            String & operator=(const String &) = delete;
            String & operator=(String && other);

            ~String();

            uint32_t size() const;

            string_view view() const;

            private:

            uint32_t m_type_size;
            uint32_t m_inlineChars0;
            union {
                uint64_t m_inlineChars1;
                char * m_dynamicChars;
            };

        };

        Value decode(string_view json);

        string encode(const Value & val, bool compact = defaultCompact);

    }

}

//
// Specialize `qc_json_valueTo` to enable Value::as for custom types
// Example:
//      template <bool unsafe>
//      struct qc_json_valueTo<std::pair<int, int>, unsafe> {
//          std::pair<int, int> operator()(const qc::json::Value & v) {
//              const Array & arr(v.asArray<unsafe>());
//              return {arr.at(0)->asInteger<unsafe>(), arr.at(1)->asInteger<unsafe>()};
//          }
//      };
//
template <typename T, bool unsafe> struct qc_json_valueTo;

//
// Specialize `qc_json_valueFrom` to enable Value construction from custom types
// Example:
//      template <>
//      struct qc_json_valueFrom<std::pair<int, int>> {
//          qc::json::Value operator()(const std::pair<int, int> & v) {
//              return qc::json::Array(v.first, f.second);
//          }
//      };
//
template <typename T> struct qc_json_valueFrom;

// IMPLEMENTATION //////////////////////////////////////////////////////////////////////////////////////////////////////

namespace qc {

    namespace json {

        namespace detail {

            constexpr uint32_t ceil2(uint32_t v) {
                --v;
                v |= v >> 1;
                v |= v >> 2;
                v |= v >> 4;
                v |= v >> 8;
                v |= v >> 16;
                return v + 1;
            }

            class Composer {

                public:

                struct State {
                    Value * node;
                    bool isObject;
                    bool isArray;
                };

                State object(State & outerState) {
                    Value * innerNode;
                    if (outerState.isObject) {
                        innerNode = &outerState.node->asObject<true>().add(std::move(m_key), Object()).second;
                    }
                    else if (outerState.isArray) {
                        innerNode = &outerState.node->asArray<true>().add(Object());
                    }
                    else {
                        *outerState.node = Object();
                        innerNode = outerState.node;
                    }
                    return {innerNode, true, false};
                }

                State array(State & outerState) {
                    Value * innerNode;
                    if (outerState.isObject) {
                        innerNode = &outerState.node->asObject<true>().add(std::move(m_key), Array()).second;
                    }
                    else if (outerState.isArray) {
                        innerNode = &outerState.node->asArray<true>().add(Array());
                    }
                    else {
                        *outerState.node = Array();
                        innerNode = outerState.node;
                    }
                    return {innerNode, false, true};
                }

                void key(std::string && k, State & state) {
                    m_key = std::move(k);
                }

                void end(State && innerState, State & outerState) {}

                template <typename T>
                void val(T v, State & state) {
                    if (state.isObject) {
                        state.node->asObject<true>().add(std::move(m_key), v);
                    }
                    else if (state.isArray) {
                        state.node->asArray<true>().add(v);
                    }
                    else {
                        *state.node = v;
                    }
                }

                private:

                std::string m_key;

            };

            inline void encodeRecursive(Encoder & encoder, const Value & val, bool compact) {
                switch (val.type()) {
                    case Type::null: {
                        encoder.val(nullptr);
                        break;
                    }
                    case Type::object: {
                        encoder.object(compact);
                        for (const auto & [key, v] : val.asObject<true>()) {
                            encoder.key(key);
                            encodeRecursive(encoder, v, compact);
                        }
                        encoder.end();
                        break;
                    }
                    case Type::array: {
                        encoder.array(compact);
                        for (const auto & v : val.asArray<true>()) {
                            encodeRecursive(encoder, v, compact);
                        }
                        encoder.end();
                        break;
                    }
                    case Type::string: {
                        encoder.val(val.asString<true>());
                        break;
                    }
                    case Type::integer: {
                        encoder.val(val.asInteger<true>());
                        break;
                    }
                    case Type::floater: {
                        encoder.val(val.asFloater<true>());
                        break;
                    }
                    case Type::boolean: {
                        encoder.val(val.asBoolean<true>());
                        break;
                    }
                }
            }

        }

        inline Value::Value(Object && val) :
            Value(reinterpret_cast<Value &&>(val))
        {}

        inline Value::Value(Array && val) :
            Value(reinterpret_cast<Value &&>(val))
        {}

        inline Value::Value(string_view val) :
            Value(reinterpret_cast<Value &&>(String(val)))
        {}

        inline Value::Value(const string & val) :
            Value(string_view(val))
        {}

        inline Value::Value(const char * val) :
            Value(string_view(val))
        {}

        inline Value::Value(char * val) :
            Value(string_view(val))
        {}

        inline Value::Value(char val) :
            Value(string_view(&val, 1))
        {}

        inline Value::Value(int64_t val) :
            m_type_data0(uint32_t(Type::integer)),
            m_data1(),
            m_integer(val)
        {}

        inline Value::Value(uint64_t val) :
            Value(int64_t(val))
        {}

        inline Value::Value(int32_t val) :
            Value(int64_t(val))
        {}

        inline Value::Value(uint32_t val) :
            Value(int64_t(val))
        {}

        inline Value::Value(int16_t val) :
            Value(int64_t(val))
        {}

        inline Value::Value(uint16_t val) :
            Value(int64_t(val))
        {}

        inline Value::Value(int8_t val) :
            Value(int64_t(val))
        {}

        inline Value::Value(uint8_t val) :
            Value(int64_t(val))
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
            Value(qc_json_valueFrom<T>()(val))
        {}

        inline Value::Value(Value && other) :
            m_type_data0(other.m_type_data0),
            m_data1(other.m_data1),
            m_data2(other.m_data2)
        {
            other.m_type_data0 = 0;
            other.m_data1 = 0;
            other.m_data2 = 0;
        }

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
                    reinterpret_cast<Object &>(*this).~Object();
                    break;
                case Type::array:
                    reinterpret_cast<Array &>(*this).~Array();
                    break;
                case Type::string:
                    reinterpret_cast<String &>(*this).~String();
                    break;
            }
        }

        inline Type Value::type() const {
            return Type(m_type_data0 & 0b111'00000'00000000'00000000'00000000u);
        }

        inline bool Value::is(Type t) const {
            return type() == t;
        }

        template <bool unsafe>
        inline const Object & Value::asObject() const {
            if constexpr (!unsafe) if (type() != Type::object) throw TypeError();
            return reinterpret_cast<const Object &>(*this);
        }

        template <bool unsafe>
        inline Object & Value::asObject() {
            return const_cast<Object &>(const_cast<const Value &>(*this).asObject<unsafe>());
        }

        template <bool unsafe>
        inline const Array & Value::asArray() const {
            if constexpr (!unsafe) if (type() != Type::array) throw TypeError();
            return reinterpret_cast<const Array &>(*this);
        }

        template <bool unsafe>
        inline Array & Value::asArray() {
            return const_cast<Array &>(const_cast<const Value &>(*this).asArray<unsafe>());
        }

        template <bool unsafe>
        inline std::string_view Value::asString() const {
            if constexpr (!unsafe) if (type() != Type::string) throw TypeError();
            return reinterpret_cast<const String &>(*this).view();
        }

        template <bool unsafe>
        inline const int64_t & Value::asInteger() const {
            if constexpr (!unsafe) if (type() != Type::integer) throw TypeError();
            return m_integer;
        }

        template <bool unsafe>
        inline int64_t & Value::asInteger() {
            return const_cast<int64_t &>(const_cast<const Value &>(*this).asInteger<unsafe>());
        }

        template <bool unsafe>
        inline nullptr_t Value::asNull() const {
            if constexpr (!unsafe) if (type() != Type::null) throw TypeError();
            return nullptr;
        }

        template <bool unsafe>
        inline const double & Value::asFloater() const {
            if constexpr (!unsafe) if (type() != Type::floater) throw TypeError();
            return m_floater;
        }

        template <bool unsafe>
        inline double & Value::asFloater() {
            return const_cast<double &>(const_cast<const Value &>(*this).asFloater<unsafe>());
        }

        template <bool unsafe>
        inline const bool & Value::asBoolean() const {
            if constexpr (!unsafe) if (type() != Type::boolean) throw TypeError();
            return m_boolean;
        }

        template <bool unsafe>
        inline bool & Value::asBoolean() {
            return const_cast<bool &>(const_cast<const Value &>(*this).asBoolean<unsafe>());
        }

        template <typename T, bool unsafe>
        inline T Value::as() const {
            // `T` should not be `qc::json::Object`
            static_assert(!std::is_same_v<T, Object>, "Use `qc::json::Value::asObject` instead, as this function must return by value");
            // `T` should not be `qc::json::Array`
            static_assert(!std::is_same_v<T, Object>, "Use `qc::json::Value::asArray` instead, as this function must return by value");
            // `T` should not be `std::string`, `const char *`, `char *`, `char`, or `qc::json::String`
            static_assert(
                !(std::is_same_v<T, string> || std::is_same_v<T, const char *> || std::is_same_v<T, char *> || std::is_same_v<T, char> || std::is_same_v<T, String>),
                "Use `qc::json::Value::asString` or `qc::json::Value::as<std::string_view>` instead"
                );

            // String
            if constexpr (std::is_same_v<T, string_view>) {
                return asString<unsafe>();
            }
            // Boolean
            else if constexpr (std::is_same_v<T, bool>) {
                return asBoolean<unsafe>();
            }
            // Integer
            else if constexpr (std::is_integral_v<T>) {
                return T(asInteger<unsafe>());
            }
            // Floater
            else if constexpr (std::is_floating_point_v<T>) {
                return T(asFloater<unsafe>());
            }
            // Null
            else if constexpr (std::is_same_v<T, nullptr_t>) {
                return asNull<unsafe>();
            }
            // Other
            else {
                return qc_json_valueTo<T, unsafe>()(*this);
            }
        }

        inline Object::Object(Object && other) :
            m_type_capacity(other.m_type_capacity),
            m_size(other.m_size),
            m_pairs(other.m_pairs)
        {
            other.m_type_capacity = uint32_t(Type::object);
            other.m_size = 0;
            other.m_pairs = nullptr;
        }

        inline Object & Object::operator=(Object && other) {
            m_type_capacity = other.m_type_capacity;
            m_size = other.m_size;
            m_pairs = other.m_pairs;
            other.m_type_capacity = uint32_t(Type::object);
            other.m_size = 0;
            other.m_pairs = nullptr;
            return *this;
        }

        inline Object::~Object() {
            if (m_size > 0) {
                clear();
                ::operator delete(m_pairs);
            }
        }

        inline uint32_t Object::size() const {
            return m_size;
        }

        inline uint32_t Object::capacity() const {
            return m_type_capacity << 3;
        }

        inline bool Object::empty() const {
            return m_size == 0;
        }

        inline Object::Pair & Object::add(string && key, Value && val) {
            // If this is the first pair, allocate backing array
            if (!m_pairs) {
                m_pairs = static_cast<Pair *>(::operator new(8 * sizeof(Pair)));
                m_type_capacity = uint32_t(Type::object) | 1u;
            }

            // Find the position in the backing array where this pair should go
            auto [pos, found](m_search(key));

            // If key already exists, replace it
            if (found) {
                pos->second.~Value();
                pos->first = std::move(key);
                new (&pos->second) Value(std::move(val));
                return *pos;
            }

            // If we're at capacity, expand
            if (uint32_t capacity(capacity()); m_size >= capacity) {
                uint32_t newCapacity(capacity << 1);
                Pair * newPairs(static_cast<Pair *>(::operator new(newCapacity * sizeof(Pair))));
                Pair * newPos(newPairs + (pos - m_pairs));
                // Copy the pairs before the one we're inserting
                std::copy(reinterpret_cast<const uint64_t *>(m_pairs), reinterpret_cast<const uint64_t *>(pos), reinterpret_cast<uint64_t *>(newPairs));
                // Copy the pairs after the one we're inserting, leaving a gap
                std::copy(reinterpret_cast<const uint64_t *>(pos), reinterpret_cast<const uint64_t *>(cend()), reinterpret_cast<uint64_t *>(newPos + 1));
                // Update our current state
                ::operator delete(m_pairs);
                m_pairs = newPairs;
                m_type_capacity = uint32_t(Type::object) | (newCapacity >> 3);
                pos = newPos;
            }
            // Otherwise, we've still got space
            else {
                // Shift back the pairs after the one we're inserting, leaving a gap
                Pair * endPos(end());
                std::copy_backward(reinterpret_cast<uint64_t *>(pos), reinterpret_cast<uint64_t *>(endPos), reinterpret_cast<uint64_t *>(endPos + 1));
            }

            // Construct the new pair
            new (&pos->first) string(std::move(key));
            new (&pos->second) Value(std::move(val));
            ++m_size;

            return *pos;
        }

        inline bool Object::contains(string_view key) const {
            return m_search(key).second;
        }

        inline const Value & Object::at(string_view key) const {
            auto [pos, found](m_search(key));
            if (!found) {
                throw std::out_of_range("Key not found");
            }
            return pos->second;
        }

        inline Value & Object::at(string_view key) {
            return const_cast<Value &>(const_cast<const Object &>(*this).at(key));
        }

        inline Object::const_iterator Object::find(string_view key) const {
            auto [pos, found](m_search(key));
            return found ? pos : cend();
        }

        inline Object::iterator Object::find(string_view key) {
            return const_cast<iterator>(const_cast<const Object &>(*this).find(key));
        }

        inline Object::Pair Object::remove(iterator it) {
            // Save off pair and destruct
            Pair pair(std::move(*it));
            it->~pair();

            // Shift forward posterior pairs
            std::copy(reinterpret_cast<const uint64_t *>(it + 1), reinterpret_cast<const uint64_t *>(cend()), reinterpret_cast<uint64_t *>(it));

            --m_size;

            return pair;
        }

        inline void Object::clear() {
            // Destruct the pairs
            for (Pair & pair : *this) pair.~pair();
            m_size = 0;
        }

        inline Object::iterator Object::begin() {
            return m_pairs;
        }

        inline Object::const_iterator Object::begin() const {
            return m_pairs;
        }

        inline Object::const_iterator Object::cbegin() const {
            return m_pairs;
        }

        inline Object::iterator Object::end() {
            return m_pairs + m_size;
        }

        inline Object::const_iterator Object::end() const {
            return m_pairs + m_size;
        }

        inline Object::const_iterator Object::cend() const {
            return m_pairs + m_size;
        }

        inline std::pair<const Object::Pair *, bool> Object::m_search(string_view key) const {
            const Pair * endPos(cend());
            const Pair * low(m_pairs), * high(endPos);
            while (low < high) {
                const Pair * mid(low + ((high - low) >> 1));
                int delta(std::strcmp(key.data(), mid->first.c_str()));
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

        inline std::pair<Object::Pair *, bool> Object::m_search(string_view key) {
            auto [pos, found](const_cast<const Object *>(this)->m_search(key));
            return {const_cast<Pair *>(pos), found};
        }

        template <typename T, typename... Ts>
        inline Array::Array(T && val, Ts &&... vals) :
            m_type_capacity(uint32_t(Type::array) | (std::max(detail::ceil2(1 + sizeof...(Ts)), uint32_t(8)) >> 3)),
            m_size(1 + sizeof...(Ts)),
            m_values(static_cast<Value *>(::operator new(m_size * sizeof(Value))))
        {
            // Populate `m_values` using fold expression
            int index(0);
            auto f([this, &index](auto && val) {
                new (m_values + index) Value(std::forward<decltype(val)>(val));
                ++index;
            });
            f(std::forward<T>(val));
            (f(std::forward<Ts>(vals)), ...);
        }

        inline Array::Array(Array && other) :
            m_type_capacity(other.m_type_capacity),
            m_size(other.m_size),
            m_values(other.m_values)
        {
            other.m_type_capacity = uint32_t(Type::array);
            other.m_size = 0;
            other.m_values = nullptr;
        }

        inline Array & Array::operator=(Array && other) {
            m_type_capacity = other.m_type_capacity;
            m_size = other.m_size;
            m_values = other.m_values;
            other.m_type_capacity = uint32_t(Type::array);
            other.m_size = 0;
            other.m_values = nullptr;
            return *this;
        }

        inline Array::~Array() {
            if (m_size > 0) {
                clear();
                ::operator delete(m_values);
            }
        }

        inline uint32_t Array::size() const {
            return m_size;
        }

        inline uint32_t Array::capacity() const {
            return m_type_capacity << 3;
        }

        inline bool Array::empty() const {
            return m_size == 0;
        }

        inline Value & Array::add(Value && val) {
            // If this is the first value, allocate initial storage
            if (!m_values) {
                m_values = static_cast<Value *>(::operator new(8 * sizeof(Value)));
                m_type_capacity = uint32_t(Type::array) | 1u;
            }
            // If we're at capacity, expand
            else if (uint32_t capacity(capacity());  m_size >= capacity) {
                uint32_t newCapacity(capacity << 1);
                Value * newValues(static_cast<Value *>(::operator new(newCapacity * sizeof(Value))));
                std::copy(reinterpret_cast<const uint64_t *>(m_values), reinterpret_cast<const uint64_t *>(cend()), reinterpret_cast<uint64_t *>(newValues));
                // Update our current state
                ::operator delete(m_values);
                m_values = newValues;
                m_type_capacity = uint32_t(Type::array) | (newCapacity >> 3);
            }

            return *(new (m_values + m_size++) Value(std::move(val)));
        }

        inline const Value & Array::at(uint32_t i) const {
            if (i >= m_size) {
                throw std::out_of_range("Index out of bounds");
            }

            return m_values[i];
        }

        inline Value & Array::at(uint32_t i) {
            return const_cast<Value &>(const_cast<const Array &>(*this).at(i));
        }

        inline Value Array::remove(uint32_t i) {
            if (i >= m_size) {
                throw std::out_of_range("Index out of bounds");
            }

            return remove(begin() + i);
        }

        inline Value Array::remove(iterator it) {
            // Save off value and destruct
            Value val(std::move(*it));
            it->~Value();

            // Shift posterior elements forward
            std::copy(reinterpret_cast<const uint64_t *>(it + 1), reinterpret_cast<const uint64_t *>(end()), reinterpret_cast<uint64_t *>(it));

            --m_size;

            return val;
        }

        inline void Array::remove(iterator it1, iterator it2) {
            // Destruct the values
            for (iterator it(it1); it != it2; ++it) it->~Value();

            // Shift the posterior elements forward
            std::copy(reinterpret_cast<const uint64_t *>(it2), reinterpret_cast<const uint64_t *>(cend()), reinterpret_cast<uint64_t *>(it1));

            m_size -= uint32_t(it2 - it1);
        }

        inline void Array::clear() {
            remove(begin(), end());
        }

        inline Array::iterator Array::begin() {
            return m_values;
        }

        inline Array::const_iterator Array::begin() const {
            return m_values;
        }

        inline Array::const_iterator Array::cbegin() const {
            return m_values;
        }

        inline Array::iterator Array::end() {
            return m_values + m_size;
        }

        inline Array::const_iterator Array::end() const {
            return m_values + m_size;
        }

        inline Array::const_iterator Array::cend() const {
            return m_values + m_size;
        }

        inline String::String(std::string_view str) :
            m_type_size(uint32_t(Type::string) | uint32_t(str.size())),
            m_inlineChars0(),
            m_inlineChars1()
        {
            if (str.size() <= 12) {
                std::copy(str.cbegin(), str.cend(), reinterpret_cast<char *>(&m_inlineChars0));
            }
            else {
                m_dynamicChars = static_cast<char *>(::operator new(str.size()));
                std::copy(str.cbegin(), str.cend(), m_dynamicChars);
            }
        }

        inline String::String(String && other) :
            m_type_size(other.m_type_size),
            m_inlineChars0(other.m_inlineChars0),
            m_inlineChars1(other.m_inlineChars1)
        {
            other.m_type_size = uint32_t(Type::string);
            other.m_inlineChars0 = 0;
            other.m_inlineChars1 = 0;
        }

        inline String & String::operator=(String && other) {
            m_type_size = other.m_type_size;
            m_inlineChars0 = other.m_inlineChars0;
            m_inlineChars1 = other.m_inlineChars1;
            other.m_type_size = uint32_t(Type::string);
            other.m_inlineChars0 = 0;
            other.m_inlineChars1 = 0;
            return *this;
        }

        inline String::~String() {
            if (size() > 12) ::operator delete(m_dynamicChars);
        }

        inline uint32_t String::size() const {
            return m_type_size & 0b000'11111'11111111'11111111'11111111u;
        }

        inline string_view String::view() const {
            uint32_t size(size());
            return {size > 12 ? m_dynamicChars : reinterpret_cast<const char *>(&m_inlineChars0), size};
        }

        inline Value decode(string_view json) {
            Value root;
            detail::Composer composer;
            decode(json, composer, detail::Composer::State{&root, false, false});
            return root;
        }

        inline string encode(const Value & val, bool compact) {
            Encoder encoder;
            detail::encodeRecursive(encoder, val, compact);
            return encoder.finish();
        }

    }

}
