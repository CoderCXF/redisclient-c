/*
 * Copyright (C) Alex Nekipelov (alex@nekipelov.net)
 * License: MIT
 */

#ifndef REDISCLIENT_REDISCLIENTIMPL_H
#define REDISCLIENT_REDISCLIENTIMPL_H

#include <boost/array.hpp>
#include <boost/noncopyable.hpp>
#include <boost/asio/generic/stream_protocol.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/local/stream_protocol.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/io_service.hpp>

#include <string>
#include <vector>
#include <queue>
#include <map>
#include <functional>
#include <memory>

#include "redisparser.h"
#include "redisbuffer.h"

namespace redisclient {

class RedisClientImpl : public std::enable_shared_from_this<RedisClientImpl> {
public:
    enum class State {
        Unconnected,
        Connecting,
        Connected,
        Subscribed,
        Closed
    };

    inline RedisClientImpl(boost::asio::io_service &ioService);
    inline ~RedisClientImpl();

    inline void handleAsyncConnect(
            const boost::system::error_code &ec,
            std::function<void(boost::system::error_code)> handler);

    inline size_t subscribe(const std::string &command,   // 订阅
        const std::string &channel,
        std::function<void(std::vector<char> msg)> msgHandler,
        std::function<void(RedisValue)> handler);

    inline void singleShotSubscribe(const std::string &command,
        const std::string &channel,
        std::function<void(std::vector<char> msg)> msgHandler,
        std::function<void(RedisValue)> handler);

    inline void unsubscribe(const std::string &command,  // 取消订阅
        size_t handle_id, const std::string &channel,
        std::function<void(RedisValue)> handler);

    inline void close() noexcept;

    inline State getState() const;

    inline static std::vector<char> makeCommand(const std::deque<RedisBuffer> &items);
    // 同步发送命令
    inline RedisValue doSyncCommand(const std::deque<RedisBuffer> &command,
        const boost::posix_time::time_duration &timeout,
        boost::system::error_code &ec);
    // 同步发送命令，使用pipeline发送
    inline RedisValue doSyncCommand(const std::deque<std::deque<RedisBuffer>> &commands,
        const boost::posix_time::time_duration &timeout,
        boost::system::error_code &ec);
    inline RedisValue syncReadResponse(
            const boost::posix_time::time_duration &timeout,
            boost::system::error_code &ec);

    inline void doAsyncCommand(
            std::vector<char> buff,
            std::function<void(RedisValue)> handler);

    inline void sendNextCommand();
    inline void processMessage();
    inline void doProcessMessage(RedisValue v);
    inline void asyncWrite(const boost::system::error_code &ec, const size_t);
    inline void asyncRead(const boost::system::error_code &ec, const size_t);

    inline void onRedisError(const RedisValue &);
    inline static void defaulErrorHandler(const std::string &s);

    template<typename Handler>
    inline void post(const Handler &handler);

    boost::asio::io_service &ioService;   // 
    boost::asio::io_service::strand strand;
    boost::asio::generic::stream_protocol::socket socket;   // 客户端套接字
    boost::array<char, 4096> buf;
    size_t bufSize; // only for sync
    size_t subscribeSeq;
    State state;   // 状态
    RedisParser redisParser;  // redis parser

    typedef std::pair<size_t, std::function<void(const std::vector<char> &buf)> > MsgHandlerType;
    typedef std::function<void(const std::vector<char> &buf)> SingleShotHandlerType;

    typedef std::multimap<std::string, MsgHandlerType> MsgHandlersMap;
    typedef std::multimap<std::string, SingleShotHandlerType> SingleShotHandlersMap;

    std::queue<std::function<void(RedisValue)> > handlers;
    std::deque<std::vector<char>> dataWrited;
    std::deque<std::vector<char>> dataQueued;
    MsgHandlersMap msgHandlers;
    SingleShotHandlersMap singleShotMsgHandlers;

    std::function<void(const std::string &)> errorHandler;   // 错误状态处理
};

template<typename Handler>
inline void RedisClientImpl::post(const Handler &handler)
{
    strand.post(handler);
}

inline std::string to_string(RedisClientImpl::State state)
{
    switch(state)
    {
        case RedisClientImpl::State::Unconnected:
            return "Unconnected";
            break;
        case RedisClientImpl::State::Connecting:
            return "Connecting";
            break;
        case RedisClientImpl::State::Connected:
            return "Connected";
            break;
        case RedisClientImpl::State::Subscribed:
            return "Subscribed";
            break;
        case RedisClientImpl::State::Closed:
            return "Closed";
            break;
    }

    return "Invalid";
}
}


#ifdef REDIS_CLIENT_HEADER_ONLY
#include "redisclientimpl.cpp"
#endif

#endif // REDISCLIENT_REDISCLIENTIMPL_H
