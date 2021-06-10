#include "src/clientimpl.h"


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

}