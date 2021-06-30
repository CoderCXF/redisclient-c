#ifndef REDIS_CLIENT_IMPL
#define REDIS_CLIENT_IMPL


#include <queue>
#include <deque>

#include <boost/array.hpp>
#include <boost/noncopyable.hpp>
#include <boost/asio/generic/stream_protocol.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/local/stream_protocol.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/io_service.hpp>

#include "redisvalue.h"
#include "redisbuffer.h"

namespace redisclient{
class RedisClientImpl{
    
public:
    enum class State {
            Unconnected,
            Connecting,
            Connected,
            Subscribed,
            Closed
        };
    RedisClientImpl(boost::asio::io_service &ioService);
    ~RedisClientImpl();
    void close() noexcept;
    inline State getState() const;

    static std::vector<char> makeCommand(const std::deque<RedisBuffer> &items);

    RedisValue doSyncCommand(const std::deque<RedisBuffer> &command,
        const boost::posix_time::time_duration &timeout,
        boost::system::error_code &ec);
    
    // 同步发送命令，使用pipeline发送
    RedisValue doSyncCommand(const std::deque<std::deque<RedisBuffer>> &commands,
        const boost::posix_time::time_duration &timeout,
        boost::system::error_code &ec);
    
    RedisValue syncReadResponse(
            const boost::posix_time::time_duration &timeout,
            boost::system::error_code &ec);

    inline void doAsyncCommand(
            std::vector<char> buff,
            std::function<void(RedisValue)> handler);
private:
    boost::asio::io_service &ioService;  
    boost::asio::io_service::strand strand;
    boost::asio::generic::stream_protocol::socket socket;   // 客户端套接字
    //RedisParser redisParser;  // redis parser
    boost::array<char, 4096> buf;
    size_t bufSize; // only for sync
    size_t subscribeSeq;
    State state;   // 状态

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

#endif