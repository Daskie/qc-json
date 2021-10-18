#pragma once

///
/// QC JSON 1.4.8
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
/// See the README for more info and examples
///

#include <cstring>

#include <algorithm>
#include <memory>
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
    /// Required for low-order bit packing
    ///
    static_assert(__STDCPP_DEFAULT_NEW_ALIGNMENT__ >= 8);

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
    enum class Type : uint8_t
    {
        null     = 0b000u,
        object   = 0b001u,
        array    = 0b010u,
        string   = 0b011u,
        integer  = 0b100u,
        unsigner = 0b101u,
        floater  = 0b110u,
        boolean  = 0b111u
    };

    class Value;

    using Object = std::map<string, Value>;
    using Array = std::vector<Value>;

    ///
    /// Represents one JSON value, which can be an object, array, string, number, boolean, or null
    ///
    class Value
    {
        public: //--------------------------------------------------------------

        ///
        /// Constructs a null value
        ///
        Value(std::nullptr_t = nullptr) noexcept {}

        ///
        /// @param val the value whith which to be constructed
        ///
        Value(Object && val, Density density = Density::unspecified) noexcept;
        Value(Array && val, Density density = Density::unspecified) noexcept;
        Value(string && val) noexcept;
        Value(string_view val) noexcept;
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
        /// details of which can be found below
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
        /// @return the density of the object or array, or `unspecified` if not an object or array
        ///
        Density density() const noexcept;

        ///
        /// Sets the density of the object or array
        ///
        /// @param density the new density
        ///
        void setDensity(Density density) noexcept;

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
        /// @return whether the value is a signed integer
        ///
        bool isInteger() const noexcept;

        ///
        /// @return whether the value is an unsigned integer
        ///
        bool isUnsigner() const noexcept;

        ///
        /// @return whether the value is a floater
        ///
        bool isFloater() const noexcept;

        ///
        /// @return whether the value is a boolean
        ///
        bool isBoolean() const noexcept;

        ///
        /// @return whether the value is null
        ///
        bool isNull() const noexcept;

        ///
        /// Determines if the value type is compatible with `T`, which is to say calling `to<T>()` would be valid. See
        /// the `to` method docs below for more details
        ///
        /// @tparam T the type in question, e.g. `int` or `std::string`
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
        template <Safety isSafe = safe> string & asString() noexcept(isSafe == unsafe);
        template <Safety isSafe = safe> const string & asString() const noexcept(isSafe == unsafe);

        ///
        /// @tparam isSafe whether to check if this value is actually a signed integer
        /// @return this value as a signed integer
        /// @throw `TypeError` if this value is not a signed integer and `isSafe` is true
        ///
        template <Safety isSafe = safe> int64_t & asInteger() noexcept(isSafe == unsafe);
        template <Safety isSafe = safe> const int64_t & asInteger() const noexcept(isSafe == unsafe);

        ///
        /// @tparam isSafe whether to check if this value is actually an unsigned integer
        /// @return this value as an unsigned integer
        /// @throw `TypeError` if this value is not an unsigned integer and `isSafe` is true
        ///
        template <Safety isSafe = safe> uint64_t & asUnsigner() noexcept(isSafe == unsafe);
        template <Safety isSafe = safe> const uint64_t & asUnsigner() const noexcept(isSafe == unsafe);

        ///
        /// @tparam isSafe whether to check if this value is actually a floater
        /// @return this value as a floater
        /// @throw `TypeError` if this value is not a floater and `isSafe` is true
        ///
        template <Safety isSafe = safe> double & asFloater() noexcept(isSafe == unsafe);
        template <Safety isSafe = safe> const double & asFloater() const noexcept(isSafe == unsafe);

        ///
        /// @tparam isSafe whether to check if this value is actually a boolean
        /// @return this value as a boolean
        /// @throw `TypeError` if this value is not a boolean and `isSafe` is true
        ///
        template <Safety isSafe = safe> bool & asBoolean() noexcept(isSafe == unsafe);
        template <Safety isSafe = safe> const bool & asBoolean() const noexcept(isSafe == unsafe);

        ///
        /// Retrieves the value as the given type
        ///
        /// If the actual type does not match the requested type and `isSafe` is true, a `TypeError` is thrown
        ///
        /// If `T` is `std::string`, this call is equivalent to `asString`, except a copy of the string is returnsd
        ///
        /// If `T` is `std::string_view`, `const char *`, or `char *`, a view/pointer to the current string is returned
        ///
        /// If `T` is `char`, the first/only character of the current string is returned. If the current string has more
        /// then one character `TypeError` is thrown. Note that in c++ `char`, `signed char`, and `unsigned char` are
        /// distinct types. Asking for a `signed char` or `unsigned char` will instead try to fetch a number of type
        /// `int8_t` or `uint8_t` respectively
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
        /// If `T` is `bool`, this call is equivalent to `asBoolean` by value
        ///
        /// If `T` is `nullptr_t` simply returns `nullptr`
        ///
        /// If `T` is an unrecognized type, then we attempt to use the specialized `qc::json::valueTo` struct, details
        /// of which can be found below
        ///
        template <typename T, Safety isSafe = safe> T to() const;

        ///
        /// @return whether the value has a comment
        ///
        bool hasComment() const noexcept;

        ///
        /// @return the value's comment, or `nullptr` if it has no comment
        ///
        string * comment() noexcept;
        const string * comment() const noexcept;

        ///
        /// @param str the new comment
        ///
        void setComment(string && str);
        void setComment(string_view str);
        void setComment(const char * str);

        ///
        /// Removes the value's comment
        ///
        /// @return ownership of the value's comment
        ///
        std::unique_ptr<string> removeComment() noexcept;

        private: //-------------------------------------------------------------

        union {
            uintptr_t _ptrAndDensity;
            string * _string;
            int64_t _integer;
            uint64_t _unsigner;
            double _floater;
            bool _boolean;
            nullptr_t _null{};
        };
        uintptr_t _typeAndComment{};

        template <typename T> void _setComment(T && str);
    };

    ///
    /// Efficiently creates an object from the given key and value arguments
    ///
    /// @param key the first key, forwarded to `std::string` constructor
    /// @param val the first value, forwarded to `qc::json::Value` constructor
    /// @param more any number of additional key and value arguments
    /// @return the created object
    ///
    template <typename K, typename V, typename... MoreKVs> Object makeObject(K && key, V && val, MoreKVs &&... moreKVs);
    Object makeObject();

    ///
    /// Efficiently creates an array from the given value arguments
    ///
    /// @param vals the values, each forwarded to `qc::json::Value` constructor
    /// @return the created array
    ///
    template <typename... Vs> Array makeArray(Vs &&... vals);

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
    string encode(const Value & val, Density density = Density::multiline);

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

        struct State { Value * node; Container container; };

        State object(State & outerState)
        {
            Value * innerNode;

            switch (outerState.container) {
                case Container::object:
                    innerNode = &outerState.node->asObject<unsafe>().emplace(std::move(_key), Object{}).first->second;
                    break;
                case Container::array:
                    innerNode = &outerState.node->asArray<unsafe>().emplace_back(Object{});
                    break;
                default:
                    *outerState.node = Object{};
                    innerNode = outerState.node;
            }

            if (!_comment.empty()) {
                innerNode->setComment(std::move(_comment));
            }

            return {innerNode, Container::object};
        }

        State array(State & outerState)
        {
            Value * innerNode;

            switch (outerState.container) {
                case Container::object:
                    innerNode = &outerState.node->asObject<unsafe>().emplace(std::move(_key), Array{}).first->second;
                    break;
                case Container::array:
                    innerNode = &outerState.node->asArray<unsafe>().emplace_back(Array{});
                    break;
                default:
                    *outerState.node = Array{};
                    innerNode = outerState.node;
            }

            if (!_comment.empty()) {
                innerNode->setComment(std::move(_comment));
            }

            return {innerNode, Container::array};
        }

        void key(const string_view k, State & /*state*/)
        {
            _key = k;
        }

        void end(const Density density, State && innerState, State & /*outerState*/) {
            switch (innerState.container) {
                case Container::object:
                    innerState.node->setDensity(density);
                    break;
                case Container::array:
                    innerState.node->setDensity(density);
                    break;
                default:
                    break;
            }

            _comment.clear();
        }

        template <typename T>
        void val(const T v, State & state)
        {
            Value * composedVal;

            switch (state.container) {
                case Container::object:
                    composedVal = &state.node->asObject<unsafe>().emplace(std::move(_key), v).first->second;
                    break;
                case Container::array:
                    composedVal = &state.node->asArray<unsafe>().emplace_back(v);
                    break;
                default:
                    *state.node = v;
                    composedVal = state.node;
            }

            if (!_comment.empty()) {
                composedVal->setComment(std::move(_comment));
            }
        }

        void comment(const string_view comment, State & /*state*/)
        {
            if (_comment.empty()) {
                _comment = comment;
            }
        }

        private: //-------------------------------------------------------------

        string _key{};
        string _comment{};
    };

    inline Value::Value(Object && val, const Density density) noexcept :
        _ptrAndDensity{reinterpret_cast<uintptr_t>(new Object{std::move(val)}) | uintptr_t(density)},
        _typeAndComment{uintptr_t(Type::object)}
    {}

    inline Value::Value(Array && val, const Density density) noexcept :
        _ptrAndDensity{reinterpret_cast<uintptr_t>(new Array{std::move(val)}) | uintptr_t(density)},
        _typeAndComment{uintptr_t(Type::array)}
    {}

    inline Value::Value(string && val) noexcept :
        _ptrAndDensity{reinterpret_cast<uintptr_t>(new string{std::move(val)})},
        _typeAndComment{uintptr_t(Type::string)}
    {}

    inline Value::Value(const string_view val) noexcept :
        _ptrAndDensity{reinterpret_cast<uintptr_t>(new string{val})},
        _typeAndComment{uintptr_t(Type::string)}
    {}

    inline Value::Value(const char * const val) :
        Value(string_view{val})
    {}

    inline Value::Value(char * const val) :
        Value(string_view{val})
    {}

    inline Value::Value(const char val) noexcept :
        Value{string_view{&val, 1u}}
    {}

    inline Value::Value(const int64_t val) noexcept :
        _integer{val},
        _typeAndComment{uintptr_t(Type::integer)}
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
        _unsigner{val},
        _typeAndComment{uintptr_t(Type::unsigner)}
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
        _floater{val},
        _typeAndComment{uintptr_t(Type::floater)}
    {}

    inline Value::Value(const float val) noexcept :
        Value{double{val}}
    {}

    inline Value::Value(const bool val) noexcept :
        _boolean{val},
        _typeAndComment{uintptr_t(Type::boolean)}
    {}

    template <typename T>
    inline Value::Value(const T & val) :
        Value{::qc::json::valueFrom<T>()(val)}
    {}

    inline Value::Value(Value && other) noexcept :
        _ptrAndDensity{std::exchange(other._ptrAndDensity, 0u)},
        _typeAndComment{std::exchange(other._typeAndComment, 0u)}
    {}

    inline Value & Value::operator=(Value && other) noexcept
    {
        _ptrAndDensity = std::exchange(other._ptrAndDensity, 0u);
        _typeAndComment = std::exchange(other._typeAndComment, 0u);
        return *this;
    }

    inline Value::~Value() noexcept
    {
        switch (type()) {
            case Type::object: delete &asObject<unsafe>(); break;
            case Type::array: delete &asArray<unsafe>(); break;
            case Type::string: delete _string; break;
            default: break;
        }

        string * const comment{this->comment()};
        if (comment) {
            delete comment;
        }
    }

    inline Type Value::type() const noexcept
    {
        return Type(_typeAndComment & 0b111u);
    }

    inline Density Value::density() const noexcept
    {
        const Type type{this->type()};
        if (type == Type::object || type == Type::array) {
            return Density(_ptrAndDensity & 0b111u);
        }
        else {
            return Density::unspecified;
        }
    }

    inline void Value::setDensity(const Density density) noexcept
    {
        const Type type{this->type()};
        if (type == Type::object || type == Type::array) {
            _ptrAndDensity &= ~uintptr_t{0b111u};
            _ptrAndDensity |= uintptr_t(density);
        }
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
        const Type type{this->type()};
        return type == Type::integer || type == Type::unsigner || type == Type::floater;
    }

    inline bool Value::isInteger() const noexcept
    {
        return type() == Type::integer;
    }

    inline bool Value::isUnsigner() const noexcept
    {
        return type() == Type::unsigner;
    }

    inline bool Value::isFloater() const noexcept
    {
        return type() == Type::floater;
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

        // Object
        if constexpr (std::is_same_v<U, Object>) {
            return isObject();
        }
        // Array
        else if constexpr (std::is_same_v<U, Array>) {
            return isArray();
        }
        // String
        else if constexpr (std::is_same_v<U, string> || std::is_same_v<U, string_view> || std::is_same_v<U, const char *> || std::is_same_v<U, char *>) {
            return isString();
        }
        // Character
        else if constexpr (std::is_same_v<U, char>) {
            return isString() && asString<unsafe>().size() == 1u;
        }
        // Boolean
        else if constexpr (std::is_same_v<U, bool>) {
            return isBoolean();
        }
        // Signed integer
        else if constexpr (std::is_integral_v<U> && std::is_signed_v<U>) {
            switch (type()) {
                case Type::integer: {
                    if constexpr (std::is_same_v<U, int64_t>) {
                        return true;
                    }
                    else {
                        const int64_t val{asInteger<unsafe>()};
                        return val <= std::numeric_limits<U>::max() && val >= std::numeric_limits<U>::min();
                    }
                }
                case Type::unsigner: {
                    return asUnsigner<unsafe>() <= uint64_t(std::numeric_limits<U>::max());
                }
                case Type::floater: {
                    const double val{asFloater<unsafe>()};
                    return double(U(val)) == val;
                }
                default: {
                    return false;
                }
            }
        }
        // Unsigned integer
        else if constexpr (std::is_integral_v<U> && std::is_unsigned_v<U>) {
            switch (type()) {
                case Type::integer: {
                    const int64_t val{asInteger<unsafe>()};
                    return val >= 0 && uint64_t(val) <= std::numeric_limits<U>::max();
                }
                case Type::unsigner: {
                    if constexpr (std::is_same_v<U, uint64_t>) {
                        return true;
                    }
                    else {
                        return asUnsigner<unsafe>() <= std::numeric_limits<U>::max();
                    }
                }
                case Type::floater: {
                    const double val{asFloater<unsafe>()};
                    return val >= 0.0 && double(U(val)) == val;
                }
                default: {
                    return false;
                }
            }
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
        return const_cast<Object &>(static_cast<const Value *>(this)->asObject<isSafe>());
    }

    template <Safety isSafe>
    inline const Object & Value::asObject() const noexcept(isSafe == unsafe)
    {
        if constexpr (isSafe == safe) if (!isObject()) throw TypeError{};
        return *reinterpret_cast<const Object *>(_ptrAndDensity & ~uintptr_t{0b111u});
    }

    template <Safety isSafe>
    inline Array & Value::asArray() noexcept(isSafe == unsafe)
    {
        return const_cast<Array &>(static_cast<const Value *>(this)->asArray<isSafe>());
    }

    template <Safety isSafe>
    inline const Array & Value::asArray() const noexcept(isSafe == unsafe)
    {
        if constexpr (isSafe == safe) if (!isArray()) throw TypeError{};
        return *reinterpret_cast<const Array *>(_ptrAndDensity & ~uintptr_t{0b111u});
    }

    template <Safety isSafe>
    inline string & Value::asString() noexcept(isSafe == unsafe)
    {
        return const_cast<string &>(static_cast<const Value *>(this)->asString<isSafe>());
    }

    template <Safety isSafe>
    inline const string & Value::asString() const noexcept(isSafe == unsafe)
    {
        if constexpr (isSafe == safe) if (!isString()) throw TypeError{};
        return *_string;
    }

    template <Safety isSafe>
    inline int64_t & Value::asInteger() noexcept(isSafe == unsafe)
    {
        return const_cast<int64_t &>(static_cast<const Value *>(this)->asInteger<isSafe>());
    }

    template <Safety isSafe>
    inline const int64_t & Value::asInteger() const noexcept(isSafe == unsafe)
    {
        if constexpr (isSafe == safe) if (!isInteger()) throw TypeError{};
        return _integer;
    }

    template <Safety isSafe>
    inline uint64_t & Value::asUnsigner() noexcept(isSafe == unsafe)
    {
        return const_cast<uint64_t &>(static_cast<const Value *>(this)->asUnsigner<isSafe>());
    }

    template <Safety isSafe>
    inline const uint64_t & Value::asUnsigner() const noexcept(isSafe == unsafe)
    {
        if constexpr (isSafe == safe) if (!isUnsigner()) throw TypeError{};
        return _unsigner;
    }

    template <Safety isSafe>
    inline double & Value::asFloater() noexcept(isSafe == unsafe)
    {
        return const_cast<double &>(static_cast<const Value *>(this)->asFloater<isSafe>());
    }

    template <Safety isSafe>
    inline const double & Value::asFloater() const noexcept(isSafe == unsafe)
    {
        if constexpr (isSafe == safe) if (!isFloater()) throw TypeError{};
        return _floater;
    }

    template <Safety isSafe>
    inline bool & Value::asBoolean() noexcept(isSafe == unsafe)
    {
        return const_cast<bool &>(static_cast<const Value *>(this)->asBoolean<isSafe>());
    }

    template <Safety isSafe>
    inline const bool & Value::asBoolean() const noexcept(isSafe == unsafe)
    {
        if constexpr (isSafe == safe) if (!isBoolean()) throw TypeError{};
        return _boolean;
    }

    template <typename T, Safety isSafe>
    inline T Value::to() const
    {
        using U = std::decay_t<T>;

        // Type must not be `qc::json::Object`
        static_assert(!std::is_same_v<U, Object>, "This function would have to make a copy of the object, use `qc::json::Value::asObject` instead");

        // Type must not be `qc::json::Array`
        static_assert(!std::is_same_v<U, Array>, "This function would have to make a copy of the array. Use `qc::json::Value::asArray` instead");

        // Type must not be `char *`
        static_assert(!std::is_same_v<U, char *>, "Mutable char pointer may not be accessed by const function. Use `qc::json::Value::asString` or `qc::json::Value::to<const char *>` instead");

        // String
        if constexpr (std::is_same_v<U, string> || std::is_same_v<U, string_view>) {
            return asString<isSafe>();
        }
        else if constexpr (std::is_same_v<U, const char *>) {
            return asString<isSafe>().c_str();
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
            switch (type()) {
                case Type::integer: return U(asInteger<unsafe>());
                case Type::unsigner: return U(asUnsigner<unsafe>());
                case Type::floater: return U(asFloater<unsafe>());
                default: return {};
            }
        }
        else if constexpr (std::is_same_v<U, nullptr_t>) {
            if constexpr (isSafe == safe) if (!isNull()) throw TypeError{};
            return nullptr;
        }
        // Other
        else {
            return ::qc::json::valueTo<U, isSafe>()(*this);
        }
    }

    inline bool Value::hasComment() const noexcept
    {
        return comment();
    }

    inline string * Value::comment() noexcept
    {
        return const_cast<string *>(static_cast<const Value *>(this)->comment());
    }

    inline const string * Value::comment() const noexcept
    {
        return reinterpret_cast<string *>(_typeAndComment & ~uintptr_t{0b111u});
    }

    inline void Value::setComment(string && str)
    {
        _setComment(std::move(str));
    }

    inline void Value::setComment(const string_view str)
    {
        _setComment(str);
    }

    inline void Value::setComment(const char * str)
    {
        _setComment(str);
    }

    template <typename T>
    inline void Value::_setComment(T && str)
    {
        string * const comment{this->comment()};
        if (comment) {
            *comment = std::forward<T>(str);
        }
        else {
            _typeAndComment &= 0b111u;
            _typeAndComment |= reinterpret_cast<uintptr_t>(new string{std::move(str)});
        }
    }

    inline std::unique_ptr<string> Value::removeComment() noexcept
    {
        string * const comment{this->comment()};
        _typeAndComment &= 0b111u;
        return std::unique_ptr<string>{comment};
    }

    template <typename K, typename V, typename... MoreKVs>
    inline void _makeObjectHelper(Object & obj, K && key, V && val, MoreKVs &&... moreKVs)
    {
        obj.emplace(std::forward<K>(key), std::forward<V>(val));
        if constexpr (sizeof...(moreKVs) > 0) {
            _makeObjectHelper(obj, std::forward<MoreKVs>(moreKVs)...);
        }
    }

    template <typename K, typename V, typename... MoreKVs>
    inline Object makeObject(K && key, V && val, MoreKVs &&... moreKVs)
    {
        static_assert(sizeof...(moreKVs) % 2 == 0, "Must provide an even number of arguments alternating between key and value");
        Object obj{};
        _makeObjectHelper(obj, std::forward<K>(key), std::forward<V>(val), std::forward<MoreKVs>(moreKVs)...);
        return obj;
    }

    inline Object makeObject()
    {
        return Object{};
    }

    template <typename... Vs>
    inline Array makeArray(Vs &&... vals)
    {
        Array arr{};
        arr.reserve(sizeof...(vals));
        (arr.emplace_back(std::forward<Vs>(vals)), ...);
        return arr;
    }

    inline Value decode(const string_view json)
    {
        Value root{};
        _Composer::State rootState{&root, Container::none};
        _Composer composer{};
        decode(json, composer, rootState);
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
        if (val.hasComment() && encoder.container() != Container::object) {
            encoder << tokens::comment(*val.comment());
        }

        switch (val.type()) {
            case Type::null: {
                encoder << nullptr;
                break;
            }
            case Type::object: {
                const Object & object{val.asObject<unsafe>()};
                encoder << tokens::object(val.density());
                for (const auto & [key, v] : object) {
                    if (v.hasComment()) {
                        encoder << tokens::comment(*v.comment());
                    }
                    encoder << key << v;
                }
                encoder << tokens::end;
                break;
            }
            case Type::array: {
                const Array & array{val.asArray<unsafe>()};
                encoder << tokens::array(val.density());
                for (const auto & v : array) {
                    encoder << v;
                }
                encoder << tokens::end;
                break;
            }
            case Type::string: {
                encoder << val.asString<unsafe>();
                break;
            }
            case Type::integer: {
                encoder << val.asInteger<unsafe>();
                break;
            }
            case Type::unsigner: {
                encoder << val.asUnsigner<unsafe>();
                break;
            }
            case Type::floater: {
                encoder << val.asFloater<unsafe>();
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
