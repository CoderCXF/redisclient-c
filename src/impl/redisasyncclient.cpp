#ifndef REDISASYNCCLIENT_REDISASYNCCLIENT_CPP
#define REDISASYNCCLIENT_REDISASYNCCLIENT_CPP

#include <memory>
#include <functional>

#include "redisclient/impl/throwerror.h"
#include "redisclient/redisasyncclient.h"

namespace redisclientP{

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

}




}