/*
 * Copyright (C) Alex Nekipelov (alex@nekipelov.net)
 * License: MIT
 */

#pragma once

#include <deque>
#include <boost/system/error_code.hpp>

#include "redisbuffer.h"
#include "config.h"

namespace redisclient
{

class RedisSyncClient;
class RedisValue;

// See https://redis.io/topics/pipelining.
class Pipeline
{
public:
    REDIS_CLIENT_DECL Pipeline(RedisSyncClient &client);

    // add command to pipe
    inline Pipeline &command(std::string cmd, std::deque<RedisBuffer> args);

    // Sends all commands to the redis server.
    // For every request command will get response value.
    // Example:
    //
    //  Pipeline pipe(redis);
    //
    //  pipe.command("GET", {"foo"})
    //      .command("GET", {"bar"})
    //      .command("GET", {"more"});
    //
    //  std::vector<RedisValue> result = pipe.finish().toArray();
    //
    //  result[0];  // value of the key "foo"
    //  result[1];  // value of the key "bar"
    //  result[2];  // value of the key "more"
    //
    inline RedisValue finish();
    inline RedisValue finish(boost::system::error_code &ec);

private:
    std::deque<std::deque<RedisBuffer>> commands;
    RedisSyncClient &client;     // pipeline只用于同步客户端
};

}

