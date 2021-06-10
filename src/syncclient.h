#ifndef REDIS_SYNC_CLIENT_H
#define REDIS_SYNC_CLIENT_H

#include <boost/asio/io_service.hpp>
#include <boost/noncopyable.hpp>
#include <boost/array.hpp>
#include <boost/noncopyable.hpp>
#include <boost/asio/generic/stream_protocol.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/local/stream_protocol.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/io_service.hpp>

#include <string>
#include <string>
#include "redisvalue.h"
#include "clientimpl.h"


namespace redisclient{

class RedisSyncClient : boost::noncopyable{  // 不许拷贝
public:
    //typedef RedisClientImpl::State State;

    RedisSyncClient(boost::asio::io_service &ioService);  // 创建一个客户端，包括客户端的信息
    RedisSyncClient(RedisSyncClient &&other);
    ~RedisSyncClient();

    // Connect to redis server
    void connect(
            const boost::asio::ip::tcp::endpoint &endpoint,                // 连接endpoint
            boost::system::error_code &ec);

    // Connect to redis server
    void connect(
            const boost::asio::ip::tcp::endpoint &endpoint);

#ifdef BOOST_ASIO_HAS_LOCAL_SOCKETS
    void connect(
            const boost::asio::local::stream_protocol::endpoint &endpoint,
            boost::system::error_code &ec);

    void connect(
            const boost::asio::local::stream_protocol::endpoint &endpoint);
#endif
    // Return true if is connected to redis.
    inline bool isConnected() const;

    // disconnect from redis
    void disconnect();
    
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
#endif