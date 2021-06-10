#include "syncclient.h"

namespace redisclient{
// 构造函数
RedisSyncClient::RedisSyncClient(boost::asio::io_service &ioService)
    :pimpl(std::make_shared<RedisClientImpl>(ioService)),
    connectTimeout(boost::posix_time::hours(365 * 24)),   // 连接超时时间
    commandTimeout(boost::posix_time::hours(365 * 24)),   // 命令超时时间
    tcpNoDelay(true), tcpKeepAlive(false)                 // 设置Nagle算法和TCP层的长连接
{

}

// 移动构造
RedisSyncClient::RedisSyncClient(RedisSyncClient &&other) 
    : pimpl(std::move(other.pimpl)),
    connectTimeout(std::move(other.connectTimeout)),
    commandTimeout(std::move(other.commandTimeout)),
    tcpNoDelay(std::move(other.tcpNoDelay)),
    tcpKeepAlive(std::move(other.tcpKeepAlive)) 
{

}
RedisSyncClient::~RedisSyncClient() {
    pimpl.close();
}

void RedisSyncClient::connect(const boost::asio::ip::tcp::endpoint &endpoint)
{
    boost::system::error_code ec;

    connect(endpoint, ec);
    detail::throwIfError(ec);
}
// 连接服务器
void RedisSyncClient::connect(const boost::asio::ip::tcp::endpoint &endpoint,
    boost::system::error_code &ec)
{
    pimpl->socket.open(endpoint.protocol(), ec);

    if (!ec && tcpNoDelay)   // 禁用Nagle算法，因为每次发送的命令，例如GET、SET都是简短的字符串。
        pimpl->socket.set_option(boost::asio::ip::tcp::no_delay(true), ec);

    // TODO keep alive option

    // boost asio does not support `connect` with timeout
    int socket = pimpl->socket.native_handle();
    struct sockaddr_in addr;

    addr.sin_family = AF_INET;                    // 现在写的是客户端，需要的是对端（服务器）的地址结构
    addr.sin_port = htons(endpoint.port());
    addr.sin_addr.s_addr = inet_addr(endpoint.address().to_string().c_str());

    // Set non-blocking
    // 标准的设置非阻塞模式
    int arg = 0;
    if ((arg = fcntl(socket, F_GETFL, NULL)) < 0)
    {
        ec = boost::system::error_code(errno,
                boost::asio::error::get_system_category());
        return;
    }

    arg |= O_NONBLOCK;

    if (fcntl(socket, F_SETFL, arg) < 0)
    {
        ec = boost::system::error_code(errno,
                boost::asio::error::get_system_category());
        return;
    }

    // connecting
    // Unix的connect函数
    int result = ::connect(socket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    // 阻塞监听套接字，接受新的连接
    if (result < 0)
    {
        if (errno == EINPROGRESS)
        {
            for(;;)  // 非阻塞模式，需要死循环进行监听
            {
                //selecting
                pollfd pfd;
                pfd.fd = socket;
                pfd.events = POLLOUT;

                result = ::poll(&pfd, 1, connectTimeout.total_milliseconds());

                if (result < 0)
                {
                    if (errno == EINTR)
                    {
                        // try again
                        continue;
                    }
                    else
                    {
                        ec = boost::system::error_code(errno,
                                boost::asio::error::get_system_category());
                        return;
                    }
                }
                else if (result > 0)
                {
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
            }
        }
        else
        {
            ec = boost::system::error_code(errno,
                    boost::asio::error::get_system_category());
            return;
        }
    }
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

    if (!ec)
        pimpl->state = State::Connected;
}


}
