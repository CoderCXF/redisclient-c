#ifndef REDISCLIENT_REDISVALUE_H
#define REDISCLIENT_REDISVALUE_H

#include <boost/variant.hpp>
#include <string>
#include <vector>


namespace redisclient {

class RedisValue {
public:
    struct ErrorTag
    {
        /* data */
    };
    RedisValue();
    RedisValue(const RedisValue &other) = default;
    RedisValue& operator=(const RedisValue &) = default;
    RedisValue& operator=(RedisValue &&) = default;
    
    RedisValue(RedisValue &&other);
    RedisValue(int64_t i);
    RedisValue(const char *s);
    RedisValue(const std::string &s);
    RedisValue(std::vector<char> buf);
    RedisValue(std::vector<char> buf, struct ErrorTag);
    RedisValue(std::vector<RedisValue> array);

    // Return the value as a std::string if
    // type is a byte string; otherwise returns an empty std::string.
    inline std::string toString() const;      // 转换为std::string

    // Return the value as a std::vector<char> if    
    // type is a byte string; otherwise returns an empty std::vector<char>.
    inline std::vector<char> toByteArray() const;   //转换为字符数组进行显示

    // Return the value as a std::vector<RedisValue> if
    // type is an int; otherwise returns 0.
    inline int64_t toInt() const;  // 转换为整数int64_t

    // Return the value as an array if type is an array;
    // otherwise returns an empty array.
    inline std::vector<RedisValue> toArray() const;

    // Return the string representation of the value. Use
    // for dump content of the value.
    inline std::string inspect() const;


    inline bool isOk() const;
    inline bool isError() const;

    inline bool isNull() const;
    inline bool isInt() const;
    inline bool isArray() const;
    inline bool isByteArray() const;
    inline bool isString() const;

protected:
    // 定义两个函数模板
    // 类型转换
    template<typename T>
     T castTo() const;
    // 测试类型是否相等
    template<typename T>
    bool typeEq() const;

private:
    struct NullTag {
        inline bool operator == (const NullTag &) const {
            return true;
        }
    };


    bool error;
    boost::variant<NullTag, int64_t, std::vector<char>, std::vector<RedisValue> > value;
};

template<typename T>
T RedisValue::castTo() const
{
    if( value.type() == typeid(T) )
        return boost::get<T>(value);
    else
        return T();
}

template<typename T>
bool RedisValue::typeEq() const
{
    if( value.type() == typeid(T) )
        return true;
    else
        return false;
}
}

#endif