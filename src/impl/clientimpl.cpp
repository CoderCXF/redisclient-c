#include "src/clientimpl.h"

// 公共函数
// buffer append
// socketread/socketwrite
namespace {
    // append to buffer
    static const char crlf[] = {'\r', '\n'};
    inline void bufferAppend(std::vector<char> &vec, const std::string &s);      // 将s添加至字符数组中
    inline void bufferAppend(std::vector<char> &vec, const std::vector<char> &s);
    inline void bufferAppend(std::vector<char> &vec, const char *s);
    inline void bufferAppend(std::vector<char> &vec, char c);

    template<size_t size>
    inline void bufferAppend(std::vector<char> &vec, const char (&s)[size]);

    inline void bufferAppend(std::vector<char> &vec, const redisclient::RedisBuffer &buf)
    {
        if (buf.data.type() == typeid(std::string))
            bufferAppend(vec, boost::get<std::string>(buf.data));
        else
            bufferAppend(vec, boost::get<std::vector<char>>(buf.data));
    }

    inline void bufferAppend(std::vector<char> &vec, const std::string &s)
    {
        vec.insert(vec.end(), s.begin(), s.end());
    }

    inline void bufferAppend(std::vector<char> &vec, const std::vector<char> &s)
    {
        vec.insert(vec.end(), s.begin(), s.end());
    }

    inline void bufferAppend(std::vector<char> &vec, const char *s)
    {
        vec.insert(vec.end(), s, s + strlen(s));
    }

    inline void bufferAppend(std::vector<char> &vec, char c)
    {
        vec.resize(vec.size() + 1);
        vec[vec.size() - 1] = c;
    }

    template<size_t size>
    inline void bufferAppend(std::vector<char> &vec, const char (&s)[size])
    {
        vec.insert(vec.end(), s, s + size);
    }

    // socket就是服务端套接字
    ssize_t socketReadSomeImpl(int socket, char *buffer, size_t size,   // 读取socket至buffer中(这是客户端？？？)
            size_t timeoutMsec)   
    {
        struct timeval tv = {static_cast<time_t>(timeoutMsec / 1000),
            static_cast<__suseconds_t>((timeoutMsec % 1000) * 1000)};
        // 设置读取超时时间
        int result = setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

        // 如果设置失败的话
        if (result != 0)
        {
            return result;
        }

        pollfd pfd;

        pfd.fd = socket;     // 监听服务器的读事件（意味着服务器有信息要到）
        pfd.events = POLLIN; 

        result = ::poll(&pfd, 1, timeoutMsec);  // 使用poll
        if (result > 0)
        {
            return recv(socket, buffer, size, MSG_DONTWAIT);  // 阻塞0.5s，设置为临时非阻塞
        }
        else
        {
            return result;
        }
    }

    size_t socketReadSome(int socket, boost::asio::mutable_buffer buffer,
            const boost::posix_time::time_duration &timeout,
            boost::system::error_code &ec)
    {
        size_t bytesRecv = 0;
        size_t timeoutMsec = timeout.total_milliseconds();

        for(;;)   //非阻塞模式
        {
            ssize_t result = socketReadSomeImpl(socket,
                    boost::asio::buffer_cast<char *>(buffer) + bytesRecv,
                    boost::asio::buffer_size(buffer) - bytesRecv, timeoutMsec);

            if (result < 0)   // 非阻塞模式，读至EINTR系统调用
            {
                if (errno == EINTR)
                {
                    continue;
                }
                else
                {
                    ec = boost::system::error_code(errno,
                            boost::asio::error::get_system_category());
                    break;
                }
            }
            else if (result == 0)   // 服务器关闭
            {
                // boost::asio::error::connection_reset();
                // boost::asio::error::eof
                ec = boost::asio::error::eof;
                break;
            }
            else
            {
                bytesRecv += result;
                break;
            }
        }

        return bytesRecv;
    }

    /**
     * 向服务器端写保存命令的buffer数据 （也就是将客户端的命令发送给服务端）
    */
    ssize_t socketWriteImpl(int socket, const char *buffer, size_t size,
            size_t timeoutMsec)
    {
        struct timeval tv = {static_cast<time_t>(timeoutMsec / 1000),
            static_cast<__suseconds_t>((timeoutMsec % 1000) * 1000)};
        int result = setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)); // 设置了发送超时时间

        if (result != 0)
        {
            return result;
        }

        pollfd pfd;

        // 监听客户端的写事件？？？
        pfd.fd = socket;
        pfd.events = POLLOUT; 

        result = ::poll(&pfd, 1, timeoutMsec);
        if (result > 0)
        {
            return send(socket, buffer, size, 0);
        }
        else
        {
            return result;
        }
    }

    size_t socketWrite(int socket, boost::asio::const_buffer buffer,
            const boost::posix_time::time_duration &timeout,
            boost::system::error_code &ec)
    {
        size_t bytesSend = 0;
        size_t timeoutMsec = timeout.total_milliseconds();

        while(bytesSend < boost::asio::buffer_size(buffer))
        {
            ssize_t result = socketWriteImpl(socket,
                    boost::asio::buffer_cast<const char *>(buffer) + bytesSend,
                    boost::asio::buffer_size(buffer) - bytesSend, timeoutMsec);

            if (result < 0)
            {
                if (errno == EINTR)
                {
                    continue;
                }
                else
                {
                    ec = boost::system::error_code(errno,
                            boost::asio::error::get_system_category());
                    break;
                }
            }
            else if (result == 0)
            {
                    // boost::asio::error::connection_reset();
                    // boost::asio::error::eof
                    ec = boost::asio::error::eof;
                    break;
            }
            else
            {
                bytesSend += result;
            }
        }

        return bytesSend;
    }

    size_t socketWrite(int socket, const std::vector<boost::asio::const_buffer> &buffers,
            const boost::posix_time::time_duration &timeout,
            boost::system::error_code &ec)
    {
        size_t bytesSend = 0;
        for(const auto &buffer: buffers)
        {
            bytesSend += socketWrite(socket, buffer, timeout, ec);

            if (ec)
                break;
        }

        return bytesSend;
    }
}

namespace redisclient{
RedisClientImpl::RedisClientImpl(boost::asio::io_service &ioService_) 
: ioService(ioService_), strand(ioService), socket(ioService),
    bufSize(0),subscribeSeq(0), state(State::Unconnected)
{

}

RedisClientImpl::~RedisClientImpl(){
    close();
}   
void RedisClientImpl::close() {
    
    boost::system::error_code ignored_ec;

    msgHandlers.clear();
    decltype(handlers)().swap(handlers);

    socket.cancel(ignored_ec);
    socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
    socket.close(ignored_ec);

    state = State::Closed;
}

RedisClientImpl::State RedisClientImpl::getState() const {
    return state;
}

std::vector<char> RedisClientImpl::makeCommand(const std::deque<RedisBuffer> &items)
{
    std::vector<char> result;

    bufferAppend(result, '*');
    bufferAppend(result, std::to_string(items.size()));
    bufferAppend<>(result, crlf);

    for(const auto &item: items)
    {
        bufferAppend(result, '$');
        bufferAppend(result, std::to_string(item.size()));
        bufferAppend<>(result, crlf);
        bufferAppend(result, item);
        bufferAppend<>(result, crlf);
    }

    return result;
}

RedisValue RedisClientImpl::doSyncCommand(const std::deque<RedisBuffer> &command,
        const boost::posix_time::time_duration &timeout,
        boost::system::error_code &ec)
{
    std::vector<char> data = makeCommand(command);
    socketWrite(socket.native_handle(), boost::asio::buffer(data), timeout, ec);

    if( ec )
    {
        return RedisValue();
    }
    // 返回的是Redis读取结果
    return syncReadResponse(timeout, ec);
}

RedisValue RedisClientImpl::doSyncCommand(const std::deque<std::deque<RedisBuffer>> &commands,
        const boost::posix_time::time_duration &timeout,
        boost::system::error_code &ec)
{
    std::vector<std::vector<char>> data;
    std::vector<boost::asio::const_buffer> buffers;

    data.reserve(commands.size());
    buffers.reserve(commands.size());

    for(const auto &command: commands)
    {
        data.push_back(makeCommand(command));
        buffers.push_back(boost::asio::buffer(data.back()));
    }

    socketWrite(socket.native_handle(), buffers, timeout, ec);

    if( ec )
    {
        return RedisValue();
    }

    std::vector<RedisValue> responses;

    for(size_t i = 0; i < commands.size(); ++i)
    {
        responses.push_back(syncReadResponse(timeout, ec));

        if (ec)
        {
            return RedisValue();
        }
    }

    return RedisValue(std::move(responses));
}

// 从Redis服务器读取命令执行结果
RedisValue RedisClientImpl::syncReadResponse(
        const boost::posix_time::time_duration &timeout,
        boost::system::error_code &ec)
{
    for(;;)
    {
        if (bufSize == 0)
        {
            // bufSize就是
            bufSize = socketReadSome(socket.native_handle(),
                    boost::asio::buffer(buf), timeout, ec);

            if (ec)
                return RedisValue();
        }

        for(size_t pos = 0; pos < bufSize;)
        {
            std::pair<size_t, RedisParser::ParseResult> result =
                redisParser.parse(buf.data() + pos, bufSize - pos);

            pos += result.first;

            ::memmove(buf.data(), buf.data() + pos, bufSize - pos);
            bufSize -= pos;

            if( result.second == RedisParser::Completed )
            {
                return redisParser.result();
            }
            else if( result.second == RedisParser::Incompleted )
            {
                continue;
            }
            else
            {
                errorHandler("[RedisClient] Parser error");
                return RedisValue();
            }
        }
    }
}


}