/*
 * @Author: your name
 * @Date: 2021-06-30 20:23:52
 * @LastEditTime: 2021-06-30 20:40:01
 * @LastEditors: your name
 * @Description: In User Settings Edit
 * @FilePath: /redisclient-c/src/impl/pipeline.cpp
 */
/*
 * Copyright (C) Alex Nekipelov (alex@nekipelov.net)
 * License: MIT
 */

#ifndef REDISCLIENT_PIPELINE_CPP
#define REDISCLIENT_PIPELINE_CPP

#include "../pipeline.h"
#include "../redisvalue.h"
#include "../redissyncclient.h"

namespace redisclient
{

Pipeline::Pipeline(RedisSyncClient &client)
    : client(client)
{
}

Pipeline &Pipeline::command(std::string cmd, std::deque<RedisBuffer> args)
{
    args.push_front(std::move(cmd));
    commands.push_back(std::move(args));
    return *this;
}

// finish
// 调用redissyncclient-->doSyncCommand
RedisValue Pipeline::finish()
{
    return client.pipelined(std::move(commands));
}

RedisValue Pipeline::finish(boost::system::error_code &ec)
{
    return client.pipelined(std::move(commands), ec);
}

}

#endif // REDISCLIENT_PIPELINE_CPP
