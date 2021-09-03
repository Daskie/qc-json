#pragma once

///
/// QC JSON 1.4.7
///
/// Austin Quick : 2019 - 2021
///
/// https://github.com/Daskie/qc-json
///
/// Provides an interface for decoding a JSON strings to JSON objects, creating/manipulating JSON objects, and encoding
///   JSON objects to a JSON string
///
/// Uses `qc-json-encode.hpp` to do the encoding and `qc-json-decode.hpp` to do the decoding
///
/// See the GitHub link above for more info and examples
///

#include <cstring>

#include <algorithm>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>

#include <qc-json/qc-json-decode.hpp>
#include <qc-json/qc-json-encode.hpp>

namespace qc::json
{
    ///
    /// This will be thrown when attempting to access a value as the wrong type
    ///
    struct TypeError : Error {};

    ///
    /// An essentially cosmetic means of making certain unsafe methods more clearly so
    ///
    /// The user may choose to `using enum qc::json::Safety` to reduce code verbosity
    ///
    enum Safety { safe, unsafe };

    ///
    /// The type of the JSON value
    ///
    enum class Type : uint32_t
    {
        null,
        object,
        array,
        string,
        number,
        boolean
    };

    enum class _NumberType : uint32_t
    {
        nan,
        signedInteger,
        unsignedInteger,
        floater
    };

    class Object;
    class Array;
    class String;

    ///
    /// Represents one JSON value, which can be an object, array, string, number, boolean, or null
    ///
    class Value
    {
        friend Object;
        friend Array;
        friend String;

        public: //--------------------------------------------------------------

        ///
        /// Constructs a null value
        ///
        Value(std::nullptr_t = nullptr) noexcept;

        ///
        /// @param val the value whith which to be constructed
        ///
        Value(Object && val) noexcept;
        Value(Array && val) noexcept;
        Value(String && val) noexcept;
        Value(string_view val) noexcept;
        Value(const string & val) noexcept;
        Value(const char * val);
        Value(char * val);
        Value(char val) noexcept;
        Value(int64_t val) noexcept;
        Value(int32_t val) noexcept;
        Value(int16_t val) noexcept;
        Value(int8_t val) noexcept;
        Value(uint64_t val) noexcept;
        Value(uint32_t val) noexcept;
        Value(uint16_t val) noexcept;
        Value(uint8_t val) noexcept;
        Value(double val) noexcept;
        Value(float val) noexcept;
        Value(bool val) noexcept;

        ///
        /// Attempts to construct a value from a custom type `T` using a specialized `qc::json::valueFrom` function,
        ///   details of which can be found below
        ///
        /// @tparam T the custom type
        /// @param val the custom type value
        ///
        template <typename T> Value(const T & val);

        Value(const Value &) = delete;
        Value(Value && other) noexcept;

        Value & operator=(const Value &) = delete;
        Value & operator=(Value && other) noexcept;

        ~Value() noexcept;

        ///
        /// @return the type of the value
        ///
        Type type() const noexcept;

        ///
        /// @return whether the value is an object
        ///
        bool isObject() const noexcept;

        ///
        /// @return whether the value is an array
        ///
        bool isArray() const noexcept;

        ///
        /// @return whether the value is a string
        ///
        bool isString() const noexcept;

        ///
        /// @return whether the value is a number
        ///
        bool isNumber() const noexcept;

        ///
        /// @return whether the value is a boolean
        ///
        bool isBoolean() const noexcept;

        ///
        /// @return whether the value is null
        ///
        bool isNull() const noexcept;

        ///
        /// Determines if the value type is compatible with `T`, which is to say calling `as<T>()` would be valid. See
        ///   the `as` method docs below for more details
        ///
        /// @tparam T the type in question, e.g. `int` or `std::string_view`
        /// @return whether the value type is compatible with type `T`
        ///
        template <typename T> bool is() const noexcept;

        ///
        /// @tparam isSafe whether to check if this value is actually an object
        /// @return this value as an object
        /// @throw `TypeError` if this value is not an object and `isSafe` is true
        ///
        template <Safety isSafe = safe> Object & asObject() noexcept(isSafe == unsafe);
        template <Safety isSafe = safe> const Object & asObject() const noexcept(isSafe == unsafe);

        ///
        /// @tparam isSafe whether to check if this value is actually an array
        /// @return this value as an array
        /// @throw `TypeError` if this value is not an array and `isSafe` is true
        ///
        template <Safety isSafe = safe> Array & asArray() noexcept(isSafe == unsafe);
        template <Safety isSafe = safe> const Array & asArray() const noexcept(isSafe == unsafe);

        ///
        /// @tparam isSafe whether to check if this value is actually a string
        /// @return this value as a string
        /// @throw `TypeError` if this value is not a string and `isSafe` is true
        ///
        template <Safety isSafe = safe> string_view asString() const noexcept(isSafe == unsafe);

        ///
        /// @tparam isSafe whether to check if this value is actually a number
        /// @return this value as a number
        /// @throw `TypeError` if this value is not a number and `isSafe` is true
        ///
        template <Safety isSafe = safe> std::variant<int64_t, uint64_t, double> asNumber() const noexcept(isSafe == unsafe);

        ///
        /// @tparam isSafe whether to check if this value is actually a boolean
        /// @return this value as a boolean
        /// @throw `TypeError` if this value is not a boolean and `isSafe` is true
        ///
        template <Safety isSafe = safe> bool asBoolean() const noexcept(isSafe == unsafe);

        ///
        /// Retrieves the value as the given type
        ///
        /// If the actual type does not match the requested type and `isSafe` is true, a `TypeError` is thrown
        ///
        /// If `T` is `std::string_view`, this call is equivalent to `asString`. `std::string` and `char *` are
        ///   incompatible and may not be used
        ///
        /// If `T` is `bool`, this call is equivalent to `asBoolean`
        ///
        /// If `T` is `char`, a single character string will try to be fetched. Note that in c++ `char`,
        /// `signed char`, and `unsigned char` are distinct types. Asking for a `signed char` or `unsigned char` will
        ///   instead try to fetch a number as type `int8_t` or `uint8_t` respectively
        ///
        /// If `T` is a numeric type...
        ///   ...and the value is a positive integer, it may be accessed as:
        ///     - any floater type (`double`, `float`)
        ///     - any signed integer type (`int64_t`, `int32_t`, `int16_t`, `int8_t`), but only if it can fit
        ///     - any unsigned integer type (`uint64_t`, `uint32_t`, `uint16_t`, `uint8_t`), but only if it can fit
        ///   ...and the value is a negative integer, it may be accessed as:
        ///     - any floater type (`double`, `float`)
        ///     - any signed integer type (`int64_t`, `int32_t`, `int16_t`, `int8_t`), but only if it can fit
        ///   ...and the value is not an integer, it may only be accessed as a floater (`double`, `float`)
        ///
        /// If `T` is an unrecognized type, then we attempt to use the specialized `qc::json::valueTo` struct, details
        ///   of which can be found below
        ///
        template <typename T, Safety isSafe = safe> T as() const;

        private: //-------------------------------------------------------------

        struct
        {
            uint32_t type_and_data;
            _NumberType numberType;
        } _data0{};
        union
        {
            int64_t signedInteger;
            uint64_t unsignedInteger;
            double floater;
            bool boolean;
        } _data1{};

    };

    ///
    /// Represents a JSON object
    ///
    class Object
    {
        public: //--------------------------------------------------------------

        using Pair = std::pair<string, Value>;

        using iterator = Pair *;
        using const_iterator = const Pair *;

        ///
        /// Construct an empty object. No memory is allocated until the first element is added
        ///
        Object() noexcept = default;

        Object(const Object &) = delete;
        Object(Object && other) noexcept;

        Object & operator=(const Object &) = delete;
        Object & operator=(Object && other) noexcept;

        ~Object() noexcept;

        ///
        /// @return the current number of elements in the object
        ///
        uint32_t size() const noexcept;

        ///
        /// @return whether the object has no elements
        ///
        bool empty() const noexcept;

        ///
        /// @return the size of the backing array. Always a power of two
        ///
        uint32_t capacity() const noexcept;

        ///
        /// Add a new key/value element to the object, growing to the next power of two if at full capacity
        ///
        /// @param key the key to add or replace
        /// @param val the value to add or replace
        /// @return the new pair entry in the object
        ///
        Pair & add(string && key, Value && val);

        ///
        /// @param key the key in question
        /// @return whether the object has an element with `key`
        ///
        bool contains(string_view key) const;

        ///
        /// @param key the key of the element to fetch
        /// @return the pair entry matching `key` if it exists
        /// @throw `std::out_of_range` if the key does not exist in the object
        ///
        Value & at(string_view key);
        const Value & at(string_view key) const;

        ///
        /// @param key the key of the element to find
        /// @return an iterator to the pair entry matching `key`, or the end iterator if the key does not exist
        ///
        iterator find(string_view key);
        const_iterator find(string_view key) const;

        ///
        /// @param it a valid iterator to an element present in the object
        /// @return the removed element
        ///
        Pair remove(iterator it) noexcept;

        ///
        /// Deletes all elements
        ///
        void clear() noexcept;

        ///
        /// @return an iterator to the first element, or the end iterator if the object is empty
        ///
        iterator begin() noexcept;
        const_iterator begin() const noexcept;
        const_iterator cbegin() const noexcept;

        ///
        /// @return the end iterator, which is one-past the last element
        ///
        iterator end() noexcept;
        const_iterator end() const noexcept;
        const_iterator cend() const noexcept;

        private: //-------------------------------------------------------------

        uint32_t _type_and_capacity{uint32_t(Type::object) << 29};
        uint32_t _size{0u};
        alignas(8) Pair * _pairs{nullptr};

        std::pair<const Pair *, bool> _search(string_view key) const;
        std::pair<Pair *, bool> _search(string_view key);
    };

    ///
    /// Represents a JSON array
    ///
    class Array
    {
        public: //--------------------------------------------------------------

        using iterator = Value *;
        using const_iterator = const Value *;

        ///
        /// Constructs an empty array. No memory is allocated until the first element is added
        ///
        Array() noexcept = default;

        ///
        /// Constructs an array from the given arguments, which may be differnt types. Memory is allocated to match the
        ///   number of elements
        ///
        /// @tparam T the type of the first value
        /// @tparam Ts the remaining value types, if any
        /// @param val the first value
        /// @param vals the remaining values, if any
        ///
        template <typename T, typename... Ts> explicit Array(T && val, Ts &&... vals);

        Array(const Array & other) = delete;
        Array(Array && other) noexcept;

        Array & operator=(const Array &) = delete;
        Array & operator=(Array && other) noexcept;

        ~Array() noexcept;

        ///
        /// @return the current number of elements in the array
        ///
        uint32_t size() const noexcept;

        ///
        /// @return whether the array has no elements
        ///
        bool empty() const noexcept;

        ///
        /// @return the backing capacity of the array
        ///
        uint32_t capacity() const noexcept;

        ///
        /// Adds the value to the end of the array, growing to the next power of two if at full capacity
        ///
        /// @param val the value to add
        /// @return the new element in the array
        ///
        Value & add(Value && val) noexcept;

        ///
        /// @param i the index of the element to fetch
        /// @return the element at index `i`
        /// @throw `std::out_of_range` if `i` >= the current size of the array
        Value & at(uint32_t i);
        const Value & at(uint32_t i) const;

        ///
        /// Removes the element at the given index and shifts all posterior elements forward
        ///
        /// @param i the index of the element to remove
        /// @return the removed element
        /// @throw `std::out_of_range` if `i` >= the current size of the array
        ///
        Value remove(uint32_t i);

        ///
        /// Removes the element at the given iterator and shifts all posterior elements forward
        ///
        /// @param it a valid iterator to an element to remove
        /// @return the removed element
        ///
        Value remove(iterator it) noexcept;

        ///
        /// Removes the elements in the given iterator range and shifts all posterior elements forward
        ///
        /// @param it1 a valid iterator to the first element to remove
        /// @param it2 an iterator to one-past the last element to remove
        ///
        void remove(iterator it1, iterator it2) noexcept;

        ///
        /// Deletes all elements in the array
        ///
        void clear() noexcept;

        ///
        /// @return an iterator to the first element or the end iterator if the array is empty
        ///
        iterator begin() noexcept;
        const_iterator begin() const noexcept;
        const_iterator cbegin() const noexcept;

        ///
        /// @return the end iterator, which is one-past the last element in the array
        ///
        iterator end() noexcept;
        const_iterator end() const noexcept;
        const_iterator cend() const noexcept;

        private: //-------------------------------------------------------------

        uint32_t _type_and_capacity{uint32_t(Type::array) << 29};
        uint32_t _size{0u};
        alignas(8) Value * _values{nullptr};
    };

    ///
    /// Represents a JSON string
    ///
    class String
    {
        public: //--------------------------------------------------------------

        ///
        /// Constructs a string copied from the given string view
        ///
        /// @param str the string to copy
        ///
        explicit String(string_view str) noexcept;

        String(const String &) = delete;
        String(String && other) noexcept;

        String & operator=(const String &) = delete;
        String & operator=(String && other) noexcept;

        ~String() noexcept;

        ///
        /// @return the number of characters in the string
        ///
        uint32_t size() const noexcept;

        ///
        /// @return a string view of the string
        ///
        string_view view() const noexcept;

        private: //-------------------------------------------------------------

        uint32_t _type_and_size{uint32_t(Type::string) << 29};
        uint32_t _inlineChars0{0u};
        union
        {
            uint64_t _inlineChars1{0u};
            char * _dynamicChars;
        };
    };

    ///
    /// @param json the JSON string to decode
    /// @return the decoded value of the JSON
    /// @throw `DecodeError` if the JSON string is invalid or could otherwise not be parsed
    ///
    Value decode(string_view json);

    ///
    /// @param val the JSON value to encode
    /// @param density the base density of the encoded JSON string
    /// @return an encoded JSON string of the given JSON value
    /// @throw `EncodeError` if there was an issue encoding the JSON
    ///
    string encode(const Value & val, Density density = multiline);

    ///
    /// Specialization of the encoder's `operator<<` for `Value`
    /// @param encoder the encoder
    /// @param val the JSON value to encode
    /// @return `encoder`
    /// @throw `EncodeError` if there was an issue encoding the JSON value
    ///
    Encoder & operator<<(Encoder & encoder, const Value & val);

    ///
    /// Specialize `qc::json::valueTo` to enable `Value::as` for custom types
    ///
    /// Example:
    ///     template <qc::json::Safety isSafe>
    ///     struct qc::json::valueTo<std::pair<int, int>, isSafe> {
    ///         std::pair<int, int> operator()(const qc::json::Value & v) {
    ///             const qc::json::Array & arr{v.asArray<isSafe>()};
    ///             return {arr.at(0u)->asInteger<isSafe>(), arr.at(1u)->asInteger<isSafe>()};
    ///         }
    ///     };
    ///
    template <typename T, Safety isSafe> struct valueTo;

    ///
    /// Specialize `qc::json::valueFrom` to enable `Value` construction from custom types
    ///
    /// Example:
    ///     template <>
    ///     struct qc::json::valueFrom<std::pair<int, int>> {
    ///         qc::json::Value operator()(const std::pair<int, int> & v) {
    ///             return qc::json::Array{v.first, f.second};
    ///         }
    ///     };
    ///
    template <typename T> struct valueFrom;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace qc::json
{
    class _Composer
    {
        public: //--------------------------------------------------------------

        struct State
        {
            Value * node;
            bool isObject;
            bool isArray;
        };

        State object(State & outerState)
        {
            Value * innerNode;
            if (outerState.isObject) {
                innerNode = &outerState.node->asObject<unsafe>().add(std::move(_key), Object{}).second;
            }
            else if (outerState.isArray) {
                innerNode = &outerState.node->asArray<unsafe>().add(Object{});
            }
            else {
                *outerState.node = Object{};
                innerNode = outerState.node;
            }
            return {innerNode, true, false};
        }

        State array(State & outerState)
        {
            Value * innerNode;
            if (outerState.isObject) {
                innerNode = &outerState.node->asObject<unsafe>().add(std::move(_key), Array{}).second;
            }
            else if (outerState.isArray) {
                innerNode = &outerState.node->asArray<unsafe>().add(Array{});
            }
            else {
                *outerState.node = Array{};
                innerNode = outerState.node;
            }
            return {innerNode, false, true};
        }

        void key(string && k, State &)
        {
            _key = std::move(k);
        }

        void end(State &&, State &) {}

        template <typename T>
        void val(const T v, State & state)
        {
            if (state.isObject) {
                state.node->asObject<unsafe>().add(std::move(_key), v);
            }
            else if (state.isArray) {
                state.node->asArray<unsafe>().add(v);
            }
            else {
                *state.node = v;
            }
        }

        private: //-------------------------------------------------------------

        string _key;
    };

    inline Value::Value(const std::nullptr_t) noexcept {}

    inline Value::Value(Object && val) noexcept :
        Value{std::move(reinterpret_cast<Value &>(val))}
    {}

    inline Value::Value(Array && val) noexcept :
        Value{std::move(reinterpret_cast<Value &>(val))}
    {}

    inline Value::Value(String && val) noexcept :
        Value{std::move(reinterpret_cast<Value &>(val))}
    {}

    inline Value::Value(const string_view val) noexcept :
        Value{String{val}}
    {}

    inline Value::Value(const string & val) noexcept :
        Value{string_view{val}}
    {}

    inline Value::Value(const char * const val) :
        Value{string_view{val}}
    {}

    inline Value::Value(char * const val) :
        Value{string_view{val}}
    {}

    inline Value::Value(const char val) noexcept :
        Value{string_view{&val, 1u}}
    {}

    inline Value::Value(const int64_t val) noexcept :
        _data0{uint32_t(Type::number) << 29, _NumberType::signedInteger},
        _data1{.signedInteger = val}
    {}

    inline Value::Value(const int32_t val) noexcept :
        Value{int64_t{val}}
    {}

    inline Value::Value(const int16_t val) noexcept :
        Value{int64_t{val}}
    {}

    inline Value::Value(const int8_t val) noexcept :
        Value{int64_t{val}}
    {}

    inline Value::Value(const uint64_t val) noexcept :
        _data0{uint32_t(Type::number) << 29, _NumberType::unsignedInteger},
        _data1{.unsignedInteger = val}
    {}

    inline Value::Value(const uint32_t val) noexcept :
        Value{uint64_t{val}}
    {}

    inline Value::Value(const uint16_t val) noexcept :
        Value{uint64_t{val}}
    {}

    inline Value::Value(const uint8_t val) noexcept :
        Value{uint64_t{val}}
    {}

    inline Value::Value(const double val) noexcept :
        _data0{uint32_t(Type::number) << 29, _NumberType::floater},
        _data1{.floater = val}
    {}

    inline Value::Value(const float val) noexcept :
        Value{double{val}}
    {}

    inline Value::Value(const bool val) noexcept :
        _data0{uint32_t(Type::boolean) << 29, _NumberType::nan},
        _data1{.boolean = val}
    {}

    template <typename T>
    inline Value::Value(const T & val) :
        Value{::qc::json::valueFrom<T>()(val)}
    {}

    inline Value::Value(Value && other) noexcept :
        _data0{std::exchange(other._data0, {})},
        _data1{std::exchange(other._data1, {})}
    {}

    inline Value & Value::operator=(Value && other) noexcept
    {
        switch (type()) {
            case Type::object:
                reinterpret_cast<Object &>(*this) = std::move(reinterpret_cast<Object &>(other));
                break;
            case Type::array:
                reinterpret_cast<Array &>(*this) = std::move(reinterpret_cast<Array &>(other));
                break;
            case Type::string:
                reinterpret_cast<String &>(*this) = std::move(reinterpret_cast<String &>(other));
                break;
            default:
                _data0 = std::exchange(other._data0, {});
                _data1 = std::exchange(other._data1, {});
        }

        return *this;
    }

    inline Value::~Value() noexcept
    {
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
            default:;
        }
    }

    inline Type Value::type() const noexcept
    {
        return Type{_data0.type_and_data >> 29};
    }

    inline bool Value::isObject() const noexcept
    {
        return type() == Type::object;
    }

    inline bool Value::isArray() const noexcept
    {
        return type() == Type::array;
    }

    inline bool Value::isString() const noexcept
    {
        return type() == Type::string;
    }

    inline bool Value::isNumber() const noexcept
    {
        return type() == Type::number;
    }

    inline bool Value::isBoolean() const noexcept
    {
        return type() == Type::boolean;
    }

    inline bool Value::isNull() const noexcept
    {
        return type() == Type::null;
    }

    template <typename T>
    inline bool Value::is() const noexcept
    {
        using U = std::decay_t<T>;

        // Type should not be `std::string`, `const char *`, or `char *`
        static_assert(
            !(std::is_same_v<U, string> || std::is_same_v<U, const char *> || std::is_same_v<U, char *>),
            "Use `qc::json::Value::isString` or `qc::json::Value::is<std::string_view>` instead"
        );

        // Object
        if constexpr (std::is_same_v<U, Object>) {
            return isObject();
        }
        // Array
        else if constexpr (std::is_same_v<U, Array>) {
            return isArray();
        }
        // String
        else if constexpr (std::is_same_v<U, String> || std::is_same_v<U, string_view>) {
            return isString();
        }
        // Character
        else if constexpr (std::is_same_v<U, char>) {
            return isString() && reinterpret_cast<const String &>(*this).size() == 1u;
        }
        // Boolean
        else if constexpr (std::is_same_v<U, bool>) {
            return isBoolean();
        }
        // Signed integer
        else if constexpr (std::is_integral_v<U> && std::is_signed_v<U>) {
            return isNumber() && (
                                     ((_data0.numberType == _NumberType::signedInteger) && (_data1.signedInteger <= std::numeric_limits<U>::max()) && (_data1.signedInteger >= std::numeric_limits<U>::min())) ||
                                     ((_data0.numberType == _NumberType::unsignedInteger) && (_data1.unsignedInteger <= uint64_t{std::numeric_limits<U>::max()})) ||
                                     ((_data0.numberType == _NumberType::floater) && (U(_data1.floater) == _data1.floater))
            );
        }
        // Unsigned integer
        else if constexpr (std::is_integral_v<U> && std::is_unsigned_v<U>) {
            return isNumber() && (
                                     ((_data0.numberType == _NumberType::unsignedInteger) && (_data1.unsignedInteger <= std::numeric_limits<U>::max())) ||
                                     ((_data0.numberType == _NumberType::signedInteger) && (_data1.signedInteger >= 0) && (uint64_t(_data1.signedInteger) <= std::numeric_limits<U>::max())) ||
                                     ((_data0.numberType == _NumberType::floater) && (U(_data1.floater) == _data1.floater))
            );
        }
        // Floater
        else if constexpr (std::is_floating_point_v<U>) {
            return isNumber();
        }
        // Other
        else {
            return false;
        }
    }

    template <Safety isSafe>
    inline Object & Value::asObject() noexcept(isSafe == unsafe)
    {
        return const_cast<Object &>(static_cast<const Value &>(*this).asObject<isSafe>());
    }

    template <Safety isSafe>
    inline const Object & Value::asObject() const noexcept(isSafe == unsafe)
    {
        if constexpr (isSafe == safe) if (!isObject()) throw TypeError{};
        return reinterpret_cast<const Object &>(*this);
    }

    template <Safety isSafe>
    inline Array & Value::asArray() noexcept(isSafe == unsafe)
    {
        return const_cast<Array &>(static_cast<const Value &>(*this).asArray<isSafe>());
    }

    template <Safety isSafe>
    inline const Array & Value::asArray() const noexcept(isSafe == unsafe)
    {
        if constexpr (isSafe == safe) if (!isArray()) throw TypeError{};
        return reinterpret_cast<const Array &>(*this);
    }

    template <Safety isSafe>
    inline string_view Value::asString() const noexcept(isSafe == unsafe)
    {
        if constexpr (isSafe == safe) if (!isString()) throw TypeError{};
        return reinterpret_cast<const String &>(*this).view();
    }

    template <Safety isSafe>
    inline std::variant<int64_t, uint64_t, double> Value::asNumber() const noexcept(isSafe == unsafe)
    {
        if constexpr (isSafe == safe) if (!isNumber()) throw TypeError{};
        switch (_data0.numberType) {
            case _NumberType::signedInteger: return _data1.signedInteger;
            case _NumberType::unsignedInteger: return _data1.unsignedInteger;
            case _NumberType::floater: return _data1.floater;
            default: if constexpr (isSafe == safe) throw TypeError{}; else return _data1.signedInteger;
        }
    }

    template <Safety isSafe>
    inline bool Value::asBoolean() const noexcept(isSafe == unsafe)
    {
        if constexpr (isSafe == safe) if (!isBoolean()) throw TypeError{};
        return _data1.boolean;
    }

    template <typename T, Safety isSafe>
    inline T Value::as() const
    {
        using U = std::decay_t<T>;

        // Type should not be `qc::json::Object`
        static_assert(!std::is_same_v<U, Object>, "Use `qc::json::Value::asObject` instead, as this function must return by value");

        // Type should not be `qc::json::Array`
        static_assert(!std::is_same_v<U, Object>, "Use `qc::json::Value::asArray` instead, as this function must return by value");

        // Type should not be `std::string`, `const char *`, `char *`, or `qc::json::String`
        static_assert(
            !(std::is_same_v<U, string> || std::is_same_v<U, const char *> || std::is_same_v<U, char *> || std::is_same_v<U, String>),
            "Use `qc::json::Value::asString` or `qc::json::Value::as<std::string_view>` instead"
        );

        // String
        if constexpr (std::is_same_v<U, string_view>) {
            return asString<isSafe>();
        }
        // Character
        else if constexpr (std::is_same_v<U, char>) {
            if constexpr (isSafe == safe) if (!is<char>()) throw TypeError{};
            return asString<unsafe>().front();
        }
        // Boolean
        else if constexpr (std::is_same_v<U, bool>) {
            return asBoolean<isSafe>();
        }
        // Number
        else if constexpr (std::is_arithmetic_v<U>) {
            if constexpr (isSafe == safe) if (!is<U>()) throw TypeError{};
            switch (_data0.numberType) {
                case _NumberType::signedInteger: return U(_data1.signedInteger);
                case _NumberType::unsignedInteger: return U(_data1.unsignedInteger);
                case _NumberType::floater: return U(_data1.floater);
                default: if constexpr (isSafe == safe) throw TypeError{}; else return U(_data1.signedInteger);
            }
        }
        // Other
        else {
            return ::qc::json::valueTo<U, isSafe>()(*this);
        }
    }

    inline Object::Object(Object && other) noexcept :
        _type_and_capacity{std::exchange(other._type_and_capacity, uint32_t(Type::object) << 29)},
        _size{std::exchange(other._size, 0u)},
        _pairs{std::exchange(other._pairs, nullptr)}
    {}

    inline Object & Object::operator=(Object && other) noexcept
    {
        this->~Object();

        _type_and_capacity = std::exchange(other._type_and_capacity, uint32_t(Type::object) << 29);
        _size = std::exchange(other._size, 0u);
        _pairs = std::exchange(other._pairs, nullptr);

        return *this;
    }

    inline Object::~Object() noexcept
    {
        if (_size) {
            clear();
            ::operator delete(_pairs);
        }
    }

    inline uint32_t Object::size() const noexcept
    {
        return _size;
    }

    inline bool Object::empty() const noexcept
    {
        return !_size;
    }

    inline uint32_t Object::capacity() const noexcept
    {
        return _type_and_capacity << 3;
    }

    inline Object::Pair & Object::add(string && key, Value && val)
    {
        // If this is the first pair, allocate backing array
        if (!_pairs) {
            _pairs = static_cast<Pair *>(::operator new(8u * sizeof(Pair)));
            _type_and_capacity = (uint32_t(Type::object) << 29) | 1u;
        }

        // Find the position in the backing array where this pair should go
        auto [pos, found]{_search(key)};

        // If key already exists, replace it
        if (found) {
            pos->second = std::move(val);
            return *pos;
        }

        // If we're at capacity, expand
        if (const uint32_t capacity{Object::capacity()}; _size >= capacity) {
            const uint32_t newCapacity{capacity << 1};
            Pair * const newPairs{static_cast<Pair *>(::operator new(newCapacity * sizeof(Pair)))};
            Pair * const newPos{newPairs + (pos - _pairs)};

            // Move over the pairs before the one we're inserting
            for (Pair * src{_pairs}, * dst{newPairs}; src < pos; ++src, ++dst) {
                new (&dst->first) string{std::move(src->first)};
                src->first.~string();

                dst->second._data0 = src->second._data0;
                dst->second._data1 = src->second._data1;
            }

            // Move over the pairs after the one we're inserting, leaving a gap
            for (Pair * src{pos}, * dst{newPos + 1}, * end{this->end()}; src < end; ++src, ++dst) {
                new (&dst->first) string{std::move(src->first)};
                src->first.~string();

                dst->second._data0 = src->second._data0;
                dst->second._data1 = src->second._data1;
            }

            // Update our state
            ::operator delete(_pairs);
            _pairs = newPairs;
            _type_and_capacity = (uint32_t(Type::object) << 29) | (newCapacity >> 3);
            pos = newPos;
        }
        // Otherwise, we've still got space
        else {
            // Shift back the pairs after the one we're inserting, leaving a gap
            Pair * const end{this->end()};
            new (&end->first) string{};
            for (Pair * src{end - 1}, * dst{end}; src >= pos; --src, --dst) {
                dst->first = std::move(src->first);

                dst->second._data0 = src->second._data0;
                dst->second._data1 = src->second._data1;
            }
            pos->first.~string();
        }

        // Add new entry
        new (&pos->first) string{std::move(key)};
        new (&pos->second) Value{std::move(val)};
        ++_size;

        return *pos;
    }

    inline bool Object::contains(const string_view key) const
    {
        return _search(key).second;
    }

    inline Value & Object::at(const string_view key)
    {
        return const_cast<Value &>(static_cast<const Object &>(*this).at(key));
    }

    inline const Value & Object::at(const string_view key) const
    {
        const auto [pos, found]{_search(key)};
        if (!found) {
            throw std::out_of_range{"Key not found"};
        }
        return pos->second;
    }

    inline Object::iterator Object::find(const string_view key)
    {
        return const_cast<iterator>(static_cast<const Object &>(*this).find(key));
    }

    inline Object::const_iterator Object::find(const string_view key) const
    {
        const auto [pos, found]{_search(key)};
        return found ? pos : cend();
    }

    inline Object::Pair Object::remove(const iterator it) noexcept
    {
        // Save off pair
        Pair pair{std::move(*it)};

        // Shift forward posterior pairs
        Pair * const end{this->end()};
        for (Pair * src{it + 1}, * dst{it}; src < end; ++src, ++dst) {
            dst->first = std::move(src->first);

            dst->second._data0 = src->second._data0;
            dst->second._data1 = src->second._data1;
        }
        (end - 1)->first.~string();

        --_size;

        return pair;
    }

    inline void Object::clear() noexcept
    {
        // Destruct the pairs
        for (Pair & pair : *this) pair.~pair();
        _size = 0u;
    }

    inline Object::iterator Object::begin() noexcept
    {
        return _pairs;
    }

    inline Object::const_iterator Object::begin() const noexcept
    {
        return _pairs;
    }

    inline Object::const_iterator Object::cbegin() const noexcept
    {
        return begin();
    }

    inline Object::iterator Object::end() noexcept
    {
        return _pairs + _size;
    }

    inline Object::const_iterator Object::end() const noexcept
    {
        return _pairs + _size;
    }

    inline Object::const_iterator Object::cend() const noexcept
    {
        return end();
    }

    inline std::pair<const Object::Pair *, bool> Object::_search(const string_view key) const
    {
        const Pair * const endPos{cend()};
        const Pair * low{_pairs}, * high{endPos};
        while (low < high) {
            const Pair * const mid{low + ((high - low) >> 1)};
            const int delta{std::strcmp(key.data(), mid->first.c_str())};
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

    inline std::pair<Object::Pair *, bool> Object::_search(const string_view key)
    {
        const auto [pos, found]{static_cast<const Object *>(this)->_search(key)};
        return {const_cast<Pair *>(pos), found};
    }

    template <typename T, typename... Ts>
    inline Array::Array(T && val, Ts &&... vals) :
        _type_and_capacity{(uint32_t(Type::array) << 29) | (std::max(uint32_t(std::bit_ceil(1u + sizeof...(Ts))), 8u) >> 3)},
        _size{1u + sizeof...(Ts)},
        _values{static_cast<Value *>(::operator new(_size * sizeof(Value)))}
    {
        // Populate `_values` using fold expression
        int index{0};
        const auto populate{[this, &index](auto && val) {
            new (_values + index) Value{std::forward<decltype(val)>(val)};
            ++index;
        }};
        populate(std::forward<T>(val));
        (populate(std::forward<Ts>(vals)), ...);
    }

    inline Array::Array(Array && other) noexcept :
        _type_and_capacity{std::exchange(other._type_and_capacity, uint32_t(Type::array) << 29)},
        _size{std::exchange(other._size, 0u)},
        _values{std::exchange(other._values, nullptr)}
    {}

    inline Array & Array::operator=(Array && other) noexcept
    {
        this->~Array();

        _type_and_capacity = std::exchange(other._type_and_capacity, uint32_t(Type::array) << 29);
        _size = std::exchange(other._size, 0u);
        _values = std::exchange(other._values, nullptr);

        return *this;
    }

    inline Array::~Array() noexcept
    {
        if (_size) {
            clear();
            ::operator delete(_values);
        }
    }

    inline uint32_t Array::size() const noexcept
    {
        return _size;
    }

    inline bool Array::empty() const noexcept
    {
        return !_size;
    }

    inline uint32_t Array::capacity() const noexcept
    {
        return _type_and_capacity << 3;
    }

    inline Value & Array::add(Value && val) noexcept
    {
        // If this is the first value, allocate initial storage
        if (!_values) {
            _values = static_cast<Value *>(::operator new(8u * sizeof(Value)));
            _type_and_capacity = (uint32_t(Type::array) << 29) | 1u;
        }
        // If we're at capacity, expand
        else if (const uint32_t capacity{this->capacity()}; _size >= capacity) {
            const uint32_t newCapacity{capacity << 1};
            Value * const newValues{static_cast<Value *>(::operator new(newCapacity * sizeof(Value)))};

            // Copy over values
            std::copy(reinterpret_cast<const uint64_t *>(_values), reinterpret_cast<const uint64_t *>(cend()), reinterpret_cast<uint64_t *>(newValues));

            // Update our current state
            ::operator delete(_values);
            _values = newValues;
            _type_and_capacity = (uint32_t(Type::array) << 29) | (newCapacity >> 3);
        }

        return *new (_values + _size++) Value{std::move(val)};
    }

    inline Value & Array::at(const uint32_t i)
    {
        return const_cast<Value &>(static_cast<const Array &>(*this).at(i));
    }

    inline const Value & Array::at(const uint32_t i) const
    {
        if (i >= _size) {
            throw std::out_of_range{"Index out of bounds"};
        }

        return _values[i];
    }

    inline Value Array::remove(const uint32_t i)
    {
        if (i >= _size) {
            throw std::out_of_range{"Index out of bounds"};
        }

        return remove(begin() + i);
    }

    inline Value Array::remove(iterator it) noexcept
    {
        // Save off value and destruct
        Value val{std::move(*it)};

        // Shift posterior elements forward
        std::copy(reinterpret_cast<const uint64_t *>(it + 1), reinterpret_cast<const uint64_t *>(cend()), reinterpret_cast<uint64_t *>(it));

        --_size;

        return val;
    }

    inline void Array::remove(const iterator it1, const iterator it2) noexcept
    {
        // Destruct the values
        for (iterator it{it1}; it != it2; ++it) it->~Value();

        // Shift the posterior elements forward
        std::copy(reinterpret_cast<const uint64_t *>(it2), reinterpret_cast<const uint64_t *>(cend()), reinterpret_cast<uint64_t *>(it1));

        _size -= uint32_t(it2 - it1);
    }

    inline void Array::clear() noexcept
    {
        // Destruct the values
        for (Value & val : *this) val.~Value();
        _size = 0u;
    }

    inline Array::iterator Array::begin() noexcept
    {
        return _values;
    }

    inline Array::const_iterator Array::begin() const noexcept
    {
        return _values;
    }

    inline Array::const_iterator Array::cbegin() const noexcept
    {
        return begin();
    }

    inline Array::iterator Array::end() noexcept
    {
        return _values + _size;
    }

    inline Array::const_iterator Array::end() const noexcept
    {
        return _values + _size;
    }

    inline Array::const_iterator Array::cend() const noexcept
    {
        return end();
    }

    inline String::String(const string_view str) noexcept :
        _type_and_size{(uint32_t(Type::string) << 29) | uint32_t(str.size())}
    {
        if (str.size() <= 12u) {
            std::copy(str.cbegin(), str.cend(), reinterpret_cast<char *>(&_inlineChars0));
        }
        else {
            _dynamicChars = static_cast<char *>(::operator new(str.size()));
            std::copy(str.cbegin(), str.cend(), _dynamicChars);
        }
    }

    inline String::String(String && other) noexcept :
        _type_and_size{std::exchange(other._type_and_size, uint32_t(Type::string) << 29)},
        _inlineChars0{std::exchange(other._inlineChars0, 0u)},
        _inlineChars1{std::exchange(other._inlineChars1, 0u)}
    {}

    inline String & String::operator=(String && other) noexcept
    {
        this->~String();

        _type_and_size = std::exchange(other._type_and_size, uint32_t(Type::string) << 29);
        _inlineChars0 = std::exchange(other._inlineChars0, 0u);
        _inlineChars1 = std::exchange(other._inlineChars1, 0u);

        return *this;
    }

    inline String::~String() noexcept
    {
        if (size() > 12u) ::operator delete(_dynamicChars);
    }

    inline uint32_t String::size() const noexcept
    {
        return _type_and_size & 0b000'11111'11111111'11111111'11111111u;
    }

    inline string_view String::view() const noexcept
    {
        const uint32_t size{this->size()};
        return {size > 12u ? _dynamicChars : reinterpret_cast<const char *>(&_inlineChars0), size};
    }

    inline Value decode(const string_view json)
    {
        Value root;
        _Composer composer;
        decode(json, composer, _Composer::State{&root, false, false});
        return root;
    }

    inline string encode(const Value & val, const Density density)
    {
        Encoder encoder{density};
        encoder << val;
        return encoder.finish();
    }

    inline Encoder & operator<<(Encoder & encoder, const Value & val)
    {
        switch (val.type()) {
            case Type::null: {
                encoder << nullptr;
                break;
            }
            case Type::object: {
                encoder << object;
                for (const auto & [key, v] : val.asObject<unsafe>()) {
                    encoder << key << v;
                }
                encoder << end;
                break;
            }
            case Type::array: {
                encoder << array;
                for (const auto & v : val.asArray<unsafe>()) {
                    encoder << v;
                }
                encoder << end;
                break;
            }
            case Type::string: {
                encoder << val.asString<unsafe>();
                break;
            }
            case Type::number: {
                std::visit([&encoder](auto v) { encoder << v; }, val.asNumber<unsafe>());
                break;
            }
            case Type::boolean: {
                encoder << val.asBoolean<unsafe>();
                break;
            }
        }

        return encoder;
    }
}
