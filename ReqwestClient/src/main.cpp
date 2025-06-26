#include <SDKDDKVer.h>

#include "boost/asio/awaitable.hpp"
#include "boost/asio/detached.hpp"
#include "boost/asio/io_context.hpp"
#include "boost/beast/http/field.hpp"
#include "request_client.h"
#include <cstdio>

boost::asio::awaitable<void> main_coroutine() {
    printf("Main Coroutine\n");
    RequestClient client{};
    // auto ret = co_await client.get("https://www.google.com");
    // auto ret = co_await client.get("https://hsts.badssl.com/");

    // auto ret = co_await client.post(
    //     "http://echo.free.beeceptor.com/sample-request?author=beeceptor",
    //     R"({"name": "John Doe", "age": 30, "city": "New York"})",
    //     {{http::field::content_type, "application/json"}});

    auto ret = co_await client.post(
        "https://www.postb.in/1750865969522-5679690015967",
        R"({"name": "John Doe", "age": 30, "city": "New York"})",
        {{http::field::content_type, "application/json"}});

    printf("Content: %s\n", ret.body().c_str());
    printf("Error: %s\n", ret.ec.message().c_str());
}

int main(int argc, char *argv[]) {
    boost::asio::io_context ctx(4);

    boost::asio::co_spawn(ctx, main_coroutine(), boost::asio::detached);

    ctx.run();
    // std::getchar();
    //
    return 0;
}
