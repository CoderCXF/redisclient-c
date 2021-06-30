/*
 * Copyright (C) Alex Nekipelov (alex@nekipelov.net)
 * License: MIT
 */

#ifndef REDISSYNCCLIENT_REDISCLIENT_H
#define REDISSYNCCLIENT_REDISCLIENT_H

#include <boost/asio/io_service.hpp>
#include <boost/noncopyable.hpp>

#include <string>
#include <list>
#include <functional>

#include "redisclientimpl.h"
#include "redisbuffer.h"
#include "redisvalue.h"


namespace redisclient {

class RedisClientImpl;
class Pipeline;

class RedisSyncClient : boost::noncopyable {
public:
    typedef RedisClientImpl::State State;

    inline RedisSyncClient(boost::asio::io_service &ioService);  // 创建一个客户端socket
    inline RedisSyncClient(RedisSyncClient &&other);
    inline ~RedisSyncClient();

    // Connect to redis server
    inline void connect(
            const boost::asio::ip::tcp::endpoint &endpoint,                // 连接endpoint,带有error_code
            boost::system::error_code &ec);

    // Connect to redis server
    inline void connect(
            const boost::asio::ip::tcp::endpoint &endpoint);

#ifdef BOOST_ASIO_HAS_LOCAL_SOCKETS
    inline void connect(
            const boost::asio::local::stream_protocol::endpoint &endpoint,
            boost::system::error_code &ec);

    inline void connect(
            const boost::asio::local::stream_protocol::endpoint &endpoint);
#endif

    // Return true if is connected to redis.
    inline bool isConnected() const;

    // disconnect from redis
    inline void disconnect();

    // Set custom error handler.
    inline void installErrorHandler(
        std::function<void(const std::string &)> handler);

    // Execute command on Redis server with the list of arguments.
    inline RedisValue command(
            std::string cmd, std::deque<RedisBuffer> args);

    // Execute command on Redis server with the list of arguments.
    inline RedisValue command(
            std::string cmd, std::deque<RedisBuffer> args,
            boost::system::error_code &ec);

    // Create pipeline (see Pipeline)
    inline Pipeline pipelined();

    inline RedisValue pipelined(
            std::deque<std::deque<RedisBuffer>> commands,
            boost::system::error_code &ec);

    inline RedisValue pipelined(
            std::deque<std::deque<RedisBuffer>> commands);

    // Return connection state. See RedisClientImpl::State.
    inline State state() const;

    inline RedisSyncClient &setConnectTimeout(
            const boost::posix_time::time_duration &timeout);
    inline RedisSyncClient &setCommandTimeout(
            const boost::posix_time::time_duration &timeout);

    inline RedisSyncClient &setTcpNoDelay(bool enable);
    inline RedisSyncClient &setTcpKeepAlive(bool enable);

protected:
    inline bool stateValid() const;

private:
    std::shared_ptr<RedisClientImpl> pimpl;
    boost::posix_time::time_duration connectTimeout;
    boost::posix_time::time_duration commandTimeout;
    bool tcpNoDelay;
    bool tcpKeepAlive;
};

}

#ifdef REDIS_CLIENT_HEADER_ONLY
#include "redisclient/impl/redissyncclient.cpp"
#endif

#endif // REDISSYNCCLIENT_REDISCLIENT_H
