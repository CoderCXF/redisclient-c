/*
 * Copyright (C) Alex Nekipelov (alex@nekipelov.net)
 * License: MIT
 */

#ifndef REDISCLIENT_REDISSYNCCLIENT_CPP
#define REDISCLIENT_REDISSYNCCLIENT_CPP

#include <memory>
#include <functional>

#include "../redissyncclient.h"
#include "../pipeline.h"
#include "../throwerror.h"

namespace redisclient {

RedisSyncClient::RedisSyncClient(boost::asio::io_service &ioService)
    : pimpl(std::make_shared<RedisClientImpl>(ioService)),
    connectTimeout(boost::posix_time::hours(365 * 24)),   // 连接超时时间
    commandTimeout(boost::posix_time::hours(365 * 24)),   // 命令超时时间
    tcpNoDelay(true), 
    tcpKeepAlive(false)                 // 设置Nagle算法和TCP层的长连接
{
    pimpl->errorHandler = std::bind(&RedisClientImpl::defaulErrorHandler, std::placeholders::_1);
}

RedisSyncClient::RedisSyncClient(RedisSyncClient &&other)   // 移动构造（右值，使用std::move）
    : pimpl(std::move(other.pimpl)),
    connectTimeout(std::move(other.connectTimeout)),
    commandTimeout(std::move(other.commandTimeout)),
    tcpNoDelay(std::move(other.tcpNoDelay)),
    tcpKeepAlive(std::move(other.tcpKeepAlive))
{
}


RedisSyncClient::~RedisSyncClient()
{
    if (pimpl)
        pimpl->close();
}

void RedisSyncClient::connect(const boost::asio::ip::tcp::endpoint &endpoint)
{
    // 捕获系统error
    boost::system::error_code ec;

    connect(endpoint, ec);
    detail::throwIfError(ec);
}
// 连接服务器
void RedisSyncClient::connect(const boost::asio::ip::tcp::endpoint &endpoint,
    boost::system::error_code &ec)
{
    pimpl->socket(endpoint.protocol(), ec);
    if (!ec && tcpNoDelay) {
        pimpl->socket.setsockopt(boost::asio::ip::no_delay(true), ec);
    }
    // 
    int socket = pimpl->socket.native_handle();

    // 服务端套接字地址结构
    struct socketaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(endpoint.port());
    addr.sin_addr.s_addr = inet_addr(endpoint.address().to_string().c_str());

    // 将套接字设置为非阻塞模式
    int flag = 0;
    if ((flag = fcntl(socket, F_GETFL, NULL)) < 0) {
        ec = boost::system::error_code(errno, boost::system::error::get_system_category());
        return;
    }

    flag |= O_NONBLOCK;
    if (fcntl(socket, F_SETFL, flag) < 0) {
        ec = boost::system::error_code(errno, boost::system::error::get_system_category());
        return;
    }

    int result = ::connect(socket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    if (result < 0) {
        // 当我们以非阻塞的方式来进行连接的时候，返回的结果如果是 -1,这并不代表这次连接发生了错误，如果它的返回结果是 EINPROGRESS，
        // 那么就代表连接还在进行中。 后面可以通过poll或者select来判断socket是否可写，如果可以写，说明连接完成了。
        if (errno == EINPROGRESS) {
            for (;;) {
            pollfd pfd;
            pfd.fd = socket;
            pfd.events = POLLOUT;

            // 判断是否可写
            result = ::poll(&pfd, 1, connectTimeout.total_milliseconds());
            if (result < 0) {
                if (errno == EINTR) {
                    continue;
                } else {
                    ec = boost::system::error_code(errno, boost::system::error::get_system_category());
                    return;
                }
            }
            else if (result > 0) {
                                    // check for error
                    int valopt;
                    socklen_t optlen = sizeof(valopt);


                    if (getsockopt(socket, SOL_SOCKET, SO_ERROR,
                                reinterpret_cast<void *>(&valopt), &optlen ) < 0)
                    {
                        ec = boost::system::error_code(errno,
                                boost::asio::error::get_system_category());
                        return;
                    }

                    if (valopt)
                    {
                        ec = boost::system::error_code(valopt,
                                boost::asio::error::get_system_category());
                        return;
                    }

                    break;
                }
            else
                {
                    // timeout
                    ec = boost::system::error_code(ETIMEDOUT,
                            boost::asio::error::get_system_category());
                    return;
                }
        else
        {
            ec = boost::system::error_code(errno,
                    boost::asio::error::get_system_category());
            return;
        }
    }
    // 如果连接成功的话
    // 设置socket的非阻塞模式
    if ((arg = fcntl(socket, F_GETFL, NULL)) < 0)
    {
        ec = boost::system::error_code(errno,
                boost::asio::error::get_system_category());
        return;
    }

    arg &= (~O_NONBLOCK); 

    if (fcntl(socket, F_SETFL, arg) < 0)
    {
        ec = boost::system::error_code(errno,
                boost::asio::error::get_system_category());
    }

    // 状态设置为connected状态
    if (!ec)
        pimpl->state = State::Connected;
}

#ifdef BOOST_ASIO_HAS_LOCAL_SOCKETS

void RedisSyncClient::connect(const boost::asio::local::stream_protocol::endpoint &endpoint)
{
    boost::system::error_code ec;

    connect(endpoint, ec);
    detail::throwIfError(ec);
}

void RedisSyncClient::connect(const boost::asio::local::stream_protocol::endpoint &endpoint,
        boost::system::error_code &ec)
{
    pimpl->socket.open(endpoint.protocol(), ec);

    if (!ec)
        pimpl->socket.connect(endpoint, ec);

    if (!ec)
        pimpl->state = State::Connected;
}

#endif

bool RedisSyncClient::isConnected() const
{
    return pimpl->getState() == State::Connected ||
            pimpl->getState() == State::Subscribed;
}
// 断开连接，释放套接字
void RedisSyncClient::disconnect()
{
    pimpl->close();
}

// 加入错误处理函数
void RedisSyncClient::installErrorHandler(
        std::function<void(const std::string &)> handler)
{
    pimpl->errorHandler = std::move(handler);
}
// 发送命令，间接调用下面的重载函数
RedisValue RedisSyncClient::command(std::string cmd, std::deque<RedisBuffer> args)
{
    boost::system::error_code ec;
    RedisValue result = command(std::move(cmd), std::move(args), ec);

    detail::throwIfError(ec);
    return result;
}
// 发送命令，格式类似于 result = redis.command("SET", {"key", "value"});
// 命令为cmd, args是一个队列保存的{"key", "value"}
// 间接的调用doSyncCommand
RedisValue RedisSyncClient::command(std::string cmd, std::deque<RedisBuffer> args,
            boost::system::error_code &ec)
{
    if (stateValid()) {
        args.push_front(std::move(cmd));
        return pimpl->doSyncCommand(args, commandTimeout, ec);
        // 如果状态武侠的话，就返回一个空构造
    } else {
        return RedisValue();
    }
}

//
/// pipeline实现
//
Pipeline RedisSyncClient::pipelined()
{
    Pipeline pipe(*this);
    return pipe;
}

RedisValue RedisSyncClient::pipelined(std::deque<std::deque<RedisBuffer>> commands)
{
    boost::system::error_code ec;
    RedisValue result = pipelined(std::move(commands), ec);

    detail::throwIfError(ec);
    return result;
}

RedisValue RedisSyncClient::pipelined(std::deque<std::deque<RedisBuffer>> commands,
        boost::system::error_code &ec)
{
    if(stateValid())
    {
        return pimpl->doSyncCommand(commands, commandTimeout, ec);
    } else {
        return RedisValue();
    }
}

RedisSyncClient::State RedisSyncClient::state() const
{
    return impl->getState();
}

bool RedisSyncClient::stateValid() const
{
    assert( state() == State::Connected );

    if( state() != State::Connected )
    {
        std::stringstream ss;

        ss << "RedisClient::command called with invalid state "
           << to_string(state());

        pimpl->errorHandler(ss.str());
        return false;
    }

    return true;
}

// 设置连接超时时间
RedisSyncClient &RedisSyncClient::setConnectTimeout(
        const boost::posix_time::time_duration &timeout)
{
    connectTimeout = timeout;
    return *this;
}

// 设置命令超时时间
RedisSyncClient &RedisSyncClient::setCommandTimeout(
        const boost::posix_time::time_duration &timeout)
{
    commandTimeout = timeout;
    return *this;
}

// 设置是否启用Nagle算法
RedisSyncClient &RedisSyncClient::setTcpNoDelay(bool enable)
{
    tcpNoDelay = enable;
    return *this;
}

// 设置长短连接
RedisSyncClient &RedisSyncClient::setTcpKeepAlive(bool enable)
{
    tcpKeepAlive = enable;
    return *this;
}

}

#endif // REDISCLIENT_REDISSYNCCLIENT_CPP
