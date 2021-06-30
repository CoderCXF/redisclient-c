/*
 * @Author: your name
 * @Date: 2021-06-30 20:23:52
 * @LastEditTime: 2021-06-30 20:46:21
 * @LastEditors: your name
 * @Description: In User Settings Edit
 * @FilePath: /redisclient-c/src/throwerror.h
 */
#pragma once

#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>

namespace redisclient {
    
namespace detail {

    inline void throwErrror(const boost::system::error_code &ec) {
        boost::system::system_error error(ec);
        throw error;
    }

    inline void throwIfError(const boost::system::error_code &ec) {
        // 如果ec不为空的话就立即抛出异常
        if (!ec) {
            throwErrror(ec);
        }
    }
}

}