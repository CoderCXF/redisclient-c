/*
 * @Author: your name
 * @Date: 2021-06-30 20:23:52
 * @LastEditTime: 2021-06-30 20:43:16
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: /redisclient-c/src/pipeline.h
 */
/*
 * Copyright (C) Alex Nekipelov (alex@nekipelov.net)
 * License: MIT
 */

#pragma once

#include <deque>
#include <boost/system/error_code.hpp>

#include "redisbuffer.h"

namespace redisclient
{

class RedisSyncClient;
class RedisValue;

// See https://redis.io/topics/pipelining.
class Pipeline
{
public:
    inline Pipeline(RedisSyncClient &client);

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
    RedisSyncClient &client;     // pipeline只用于同步方式
};

}

#ifdef REDIS_CLIENT_HEADER_ONLY
#include "redisclient/impl/pipeline.cpp"
#endif

