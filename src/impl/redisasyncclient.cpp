#ifndef REDISASYNCCLIENT_REDISASYNCCLIENT_CPP
#define REDISASYNCCLIENT_REDISASYNCCLIENT_CPP

#include <memory>
#include <functional>

#include "redisclient/impl/throwerror.h"
#include "redisclient/redisasyncclient.h"

namespace redisclient{

RedisAsyncClient::RedisAsyncClient(boost::asio::io_service &ioService)
                 : pimpl(std::make_shared<RedisClientImpl>(io_service))
{
    pimpl->errorHandler = std::bind(&RedisClientImpl::defaulErrorHandler, std::placeholders::_1);
}
RedisAsyncClient::~RedisAsyncClient() {
    pimpl->close();
}

void RedisAsyncClient::connect(const boost::asio::ip::tcp::endpoint &endpoint,
                                std::function<void(boost::system::error_code)> handler) 
{
    if (pimpl->state == State::close()) {
        pimpl->redisParser = RedisParser();
        std::move(pimpl->socket);
    }

    // 如果当前已经连接或者是链接已经关闭的话
    if(pimpl->state == State::Unconnected || pimpl->state == State::Closed) {

        // 将其状态设置为Connecting
        pimpl->state = State::Connecting;
        // socket 会一直调用异步连接
        pimpl->socket.async_connect(endpoint, std::bind(&RedisClientImpl::handleAsyncConnect,
                                    pimpl, std::placeholders::_1, std::move(handler)));
    }
    else {
        handler(boost::system::error_code());
    }

#ifdef BOOST_ASIO_HAS_LOCAL_SOCKETS

void RedisAsyncClient::connect(const boost::asio::local::stream_protocol::endpoint &endpoint,
                               std::function<void(boost::system::error_code)> handler)
{
    if( pimpl->state == State::Unconnected || pimpl->state == State::Closed )
    {
        pimpl->state = State::Connecting;
        pimpl->socket.async_connect(endpoint, std::bind(&RedisClientImpl::handleAsyncConnect,
                    pimpl, std::placeholders::_1, std::move(handler)));
    }
    else
    {
        // FIXME: add correct error message
        //std::stringstream ss;
        //ss << "RedisAsyncClient::connect called on socket with state " << to_string(pimpl->state);
        //handler(false, ss.str());
        handler(boost::system::error_code());
    }
}

#endif

bool RedisAsyncClient::isConnected() {
    return piml->getState() == State::Connected 
           && pimpl->getState() == State::Subscribed;
}

void RedisAsyncClient::disconnect() {
    piml->close();
}

void RedisAsyncClient::installErrorHandler(std::function<void(const std::string &)> handler) {
    pimpl->installErrorHandler(handler);
}

//
/// publish & subscribe
//

// 函数返回的是Handle类型：id:该通道的第几个订阅者，channel:通道名
// 通道订阅
RedisAsyncClient::Handle RedisAsyncClient::subscribe(
        const std::string &channel,
        std::function<void(std::vector<char> msg)> msgHandler,
        std::function<void(RedisValue)> handler)
{
    auto handleId = pimpl->subscribe("subscribe", channel, msgHandler, handler);    
    return { handleId , channel };
}

// 模式订阅
RedisAsyncClient::Handle RedisAsyncClient::psubscribe(
    const std::string &pattern,
    std::function<void(std::vector<char> msg)> msgHandler,
    std::function<void(RedisValue)> handler)
{
    auto handleId = pimpl->subscribe("psubscribe", pattern, msgHandler, handler);
    return{ handleId , pattern };
}

// 通道退订
void RedisAsyncClient::unsubscribe(const Handle &handle)
{
    pimpl->unsubscribe("unsubscribe", handle.id, handle.channel, dummyHandler);
}

// 模式退订
void RedisAsyncClient::punsubscribe(const Handle &handle)
{
    pimpl->unsubscribe("punsubscribe", handle.id, handle.channel, dummyHandler);
}

void RedisAsyncClient::singleShotSubscribe(const std::string &channel,
                                           std::function<void(std::vector<char> msg)> msgHandler,
                                           std::function<void(RedisValue)> handler)
{
    pimpl->singleShotSubscribe("subscribe", channel, msgHandler, handler);
}

void RedisAsyncClient::singleShotPSubscribe(const std::string &pattern,
    std::function<void(std::vector<char> msg)> msgHandler,
    std::function<void(RedisValue)> handler)
{
    pimpl->singleShotSubscribe("psubscribe", pattern, msgHandler, handler);
}

// 通道发布: publish channel msg
void RedisAsyncClient::publish(const std::string &channel, const RedisBuffer &msg,
                          std::function<void(RedisValue)> handler)
{
    assert( pimpl->state == State::Connected );

    static const std::string publishStr = "PUBLISH";

    if( pimpl->state == State::Connected )
    {
        std::deque<RedisBuffer> items(3);

        items[0] = publishStr;
        items[1] = channel;
        items[2] = msg;

        pimpl->post(std::bind(&RedisClientImpl::doAsyncCommand, pimpl,
                    pimpl->makeCommand(items), std::move(handler)));
    }
    else
    {
        std::stringstream ss;

        ss << "RedisAsyncClient::command called with invalid state "
           << to_string(pimpl->state);

        pimpl->errorHandler(ss.str());
    }
}



void RedisAsyncClient::command(const std::string &cmd, std::deque<RedisBuffer> args,
                                std::function<void(RedisValue)> handler) 
{
    if(stateValid())
    {
        args.emplace_front(cmd);
        // 发送命令，将任务丢在任务队列中
        pimpl->post(std::bind(&RedisClientImpl::doAsyncCommand, pimpl,
                    pimpl->makeCommand(args), std::move(handler)));
    }
}

// 获取状态
RedisAsyncClient::State RedisAsyncClient::state() const
{
    return pimpl->getState();
}

bool RedisAsyncClient::stateValid() const
{
    assert( pimpl->state == State::Connected );

    if( pimpl->state != State::Connected )
    {
        std::stringstream ss;

        ss << "RedisAsyncClient::command called with invalid state "
           << to_string(pimpl->state);

        pimpl->errorHandler(ss.str());
        return false;
    }

    return true;
}


}







}