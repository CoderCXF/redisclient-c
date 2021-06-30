#pragma once

#include <boost/syystem/error_code.hpp>
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