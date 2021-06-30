#ifndef REDISASYNCCLIENT_REDISCLIENT_H
#define REDISASYNCCLIENT_REDISCLIENT_H

#include <boost/asio/io_service.hpp>
#include <boost/noncopyable.hpp>

#include <string>
#include <list>
#include <type_traits>
#include <functional>

#include "redisclient/impl/redisclientimpl.h"
#include "redisvalue.h"
#include "redisbuffer.h"
#include "config.h"

namespace redisclient {

class RedisClientImpl;

class RedisAsyncClient : boost::noncopyable {
public:
    struct Handle {
        size_t id;
        std::stirng channel;
    };

    typedef RedisClientImpl::State State;

    inline RedisAsyncClient(boost::asio::io_service &ioService);
    inline ~RedisAsyncClient();

    inline void connect(
        const boost::asio::ip::tcp::endpoint &endpoint,
        std::function<void(boost::system::error_code) handler>
    );

#ifdef BOOST_ASIO_HAS_LOCAL_SOCKETS
    REDIS_CLIENT_DECL void connect(
            const boost::asio::local::stream_protocol::endpoint &endpoint,
            std::function<void(boost::system::error_code)> handler);
#endif
    // simple interface
    inline boost isConnected() const;

    inline State state() const;
    
    inline void disconnect();

    inline void installErrorHandler(
        std::function<void(const std::string &)> handler);
    
    // connand interface
    inline void command(
        const std::string &cmd, std::deque<RedisBuffer> args,
        std::function<void(RedisValue)> handler = dummyHandler);

    // Subscribe to channel. Handler msgHandler will be called
    // when someone publish message on channel. Call unsubscribe 
    // to stop the subscription.    
    inline subscribe(
        const std::string &channeName,
        std::function<void(std::vector<char> msg)> msgHandler,
        std::function<void(RedisValue)> handle = &dummyHandler);

    inline psubscribe(
        const std::string &pattern,
        std::function<void(std::vector<char> msg)> msgHandler,
        std::function<void(RedisValue)> handle = &dummyHandler);
    
    inline void unsubscribe(const Handle &handle);

    inline void punsubscribe(const Handle &handle);

    // Subscribe to channel. Handler msgHandler will be called
    // when someone publish message on channel; it will be 
    // unsubscribed after call.
    inline void singleShotSubscribe(
            const std::string &channel,
            std::function<void(std::vector<char> msg)> msgHandler,
            std::function<void(RedisValue)> handler = &dummyHandler);

    inline void singleShotPSubscribe(
            const std::string &channel,
            std::function<void(std::vector<char> msg)> msgHandler,
            std::function<void(RedisValue)> handler = &dummyHandler);

    // Publish message on channel.
    inline void publish(
            const std::string &channel, const RedisBuffer &msg,
            std::function<void(RedisValue)> handler = &dummyHandler);

    inline static void dummyHandler(RedisValue) {}
protected:
    // 判断状态是否Connected
    inline bool stateValid() const;
private:
    std::shared_ptr<RedisClientImpl> pimpl;
};

}

#ifdef REDIS_CLIENT_HEADER_ONLY
#include "redisclient/impl/redisasyncclient.cpp"
#endif

#endif