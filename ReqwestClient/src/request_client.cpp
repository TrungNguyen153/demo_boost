#include "request_client.h"
#include "boost/asio/as_tuple.hpp"
#include "boost/asio/ssl/stream.hpp"
#include "boost/asio/this_coro.hpp"
#include "boost/beast/core/tcp_stream.hpp"
#include <cstdio>
#include <sstream>

namespace net = boost::asio;
using boost::asio::as_tuple;

RequestClient::RequestClient() : ctx(ssl::context::tlsv12_client) {
    // Verify the remote server's certificate
    ctx.set_verify_mode(ssl::verify_peer);

    // Cert loads
    ctx.set_default_verify_paths();
    boost::certify::enable_native_https_server_verification(ctx);
}
RequestClient::~RequestClient() {}

awaitable<Response>
RequestClient::get(const std::string &endpoint,
                   std::map<http::field, std::string> headers) {

    Response res;

    auto url_parsed = Uri::parse(endpoint);

    if (!url_parsed.Error.empty()) {
        res.user_error = url_parsed.Error;
        // log it out
        co_return res;
    }

    if (url_parsed.Protocol == "https") {
        co_return co_await https_get(url_parsed, std::move(headers));
    }

    // assume http

    co_return co_await http_get(url_parsed, std::move(headers));
}

awaitable<Response>
RequestClient::post(const std::string &endpoint, std::string &&body,
                    std::map<http::field, std::string> headers) {
    Response res;

    auto url_parsed = Uri::parse(endpoint);

    if (!url_parsed.Error.empty()) {
        res.user_error = url_parsed.Error;
        // log it out
        co_return res;
    }

    if (url_parsed.Protocol == "https") {
        co_return co_await https_post(url_parsed, std::move(body),
                                      std::move(headers));
    }

    // assume http

    co_return co_await http_post(url_parsed, std::move(body),
                                 std::move(headers));
}

awaitable<Response> RequestClient::https_get(const Uri &uri,
                                             AdditionalHeaders &&headers) {

    auto executor = co_await net::this_coro::executor;
    auto resolver = net::ip::tcp::resolver{executor};
    auto stream = ssl::stream<beast::tcp_stream>{executor, ctx};

    Response res;

    const auto &host = uri.Host;
    const auto &target = uri.target();
    const auto &protocol = uri.Protocol;

    // Look up the domain name
    auto const [ec, results] =
        co_await resolver.async_resolve(host, protocol, as_tuple);

    if (ec) {
        res.ec = ec;
        co_return res;
    }

    // Set the timeout.
    beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(30));

    // Make the connection on the IP address we get from a lookup
    auto [ec2, connected] =
        co_await beast::get_lowest_layer(stream).async_connect(results,
                                                               as_tuple);

    if (ec2) {
        res.ec = ec2;
        co_return res;
    }

    // Set up an HTTP GET request message
    http::request<http::string_body> req{http::verb::get, target, 11};
    req.set(http::field::host, host);
    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    req.keep_alive(false);

    for (const auto &[k, p] : headers) {
        req.set(k, p);
    }

    co_await https_send_and_read_response(host, stream, req, res);

    co_return res;
}
awaitable<Response> RequestClient::https_post(const Uri &uri,
                                              std::string &&body,
                                              AdditionalHeaders &&headers) {

    auto executor = co_await net::this_coro::executor;
    auto resolver = net::ip::tcp::resolver{executor};
    auto stream = ssl::stream<beast::tcp_stream>{executor, ctx};

    Response res;

    const auto &host = uri.Host;
    const auto &target = uri.target();
    const auto &protocol = uri.Protocol;

    // Look up the domain name
    auto const [ec, results] =
        co_await resolver.async_resolve(host, protocol, as_tuple);

    if (ec) {
        res.ec = ec;
        co_return res;
    }

    // Set the timeout.
    beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(30));

    // Make the connection on the IP address we get from a lookup
    auto [ec2, connected] =
        co_await beast::get_lowest_layer(stream).async_connect(results,
                                                               as_tuple);

    if (ec2) {
        res.ec = ec2;
        co_return res;
    }

    // Set up an HTTP GET request message
    http::request<http::string_body> req{http::verb::post, target, 11};
    req.set(http::field::host, host);
    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    req.keep_alive(false);

    for (const auto &[k, p] : headers) {
        req.set(k, p);
    }

    req.body() = body;

    req.prepare_payload();

    co_await https_send_and_read_response(host, stream, req, res);

    co_return res;
}
awaitable<Response> RequestClient::http_get(const Uri &uri,
                                            AdditionalHeaders &&headers) {

    auto executor = co_await net::this_coro::executor;
    auto resolver = net::ip::tcp::resolver{executor};
    auto stream = beast::tcp_stream{executor};

    Response res;

    const auto &host = uri.Host;
    const auto &target = uri.target();
    const auto &protocol = uri.Protocol;

    // Look up the domain name
    auto const [ec, results] =
        co_await resolver.async_resolve(host, protocol, as_tuple);

    if (ec) {
        res.ec = ec;
        co_return res;
    }

    // Set the timeout.
    beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(30));

    // Make the connection on the IP address we get from a lookup
    auto [ec2, connected] =
        co_await beast::get_lowest_layer(stream).async_connect(results,
                                                               as_tuple);

    if (ec2) {
        res.ec = ec2;
        co_return res;
    }

    // Set up an HTTP GET request message
    http::request<http::string_body> req{http::verb::get, target, 11};
    req.set(http::field::host, host);
    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    req.keep_alive(false);

    for (const auto &[k, p] : headers) {
        req.set(k, p);
    }

    co_await http_send_and_read_response(host, stream, req, res);

    co_return res;
}
awaitable<Response> RequestClient::http_post(const Uri &uri, std::string &&body,
                                             AdditionalHeaders &&headers) {

    auto executor = co_await net::this_coro::executor;
    auto resolver = net::ip::tcp::resolver{executor};
    auto stream = beast::tcp_stream{executor};

    Response res;

    const auto &host = uri.Host;
    const auto &target = uri.target();
    const auto &protocol = uri.Protocol;

    // Look up the domain name
    auto const [ec, results] =
        co_await resolver.async_resolve(host, protocol, as_tuple);

    if (ec) {
        res.ec = ec;
        co_return res;
    }

    // Set the timeout.
    beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(30));

    // Make the connection on the IP address we get from a lookup
    auto [ec2, connected] =
        co_await beast::get_lowest_layer(stream).async_connect(results,
                                                               as_tuple);

    if (ec2) {
        res.ec = ec2;
        co_return res;
    }

    // Set up an HTTP GET request message
    http::request<http::string_body> req{http::verb::post, target, 11};
    req.set(http::field::host, host);
    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    req.keep_alive(false);

    for (const auto &[k, p] : headers) {
        req.set(k, p);
    }

    req.body() = body;

    req.prepare_payload();

    co_await http_send_and_read_response(host, stream, req, res);

    co_return res;
}

template <class RequestBody>
awaitable<void> RequestClient::https_send_and_read_response(
    const std::string &host, StreamSSL &stream, http::request<RequestBody> &req,
    Response &response) {

    std::ostringstream os;
    os << req;
    std::string re_str = os.str();
    printf("%s\n", re_str.c_str());

    boost::certify::set_server_hostname(stream, host);
    boost::certify::sni_hostname(stream, host);

    // Set the timeout.
    beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(30));

    // Perform the SSL handshake
    auto [ec_handshake] =
        co_await stream.async_handshake(ssl::stream_base::client, as_tuple);

    if (ec_handshake) {
        response.ec = ec_handshake;
        co_return;
    }

    // Set the timeout.
    beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(30));

    // Send the HTTP request to the remote host
    co_await http::async_write(stream, req);

    // This buffer is used for reading and must be persisted
    beast::flat_buffer buffer;

    auto [ec, bytes_count] =
        co_await http::async_read(stream, buffer, response.response, as_tuple);

    if (ec) {
        response.ec = ec;
        co_return;
    }

    // Set the timeout.
    beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(30));

    // Gracefully close the stream - do not threat every error as an
    // exception!
    std::tie(ec) = co_await stream.async_shutdown(as_tuple);

    if (ec && ec != net::ssl::error::stream_truncated) {
        response.ec = ec;
    }
}

template <class RequestBody>
awaitable<void> RequestClient::http_send_and_read_response(
    const std::string &host, Stream &stream, http::request<RequestBody> &req,
    Response &response) {
    std::ostringstream os;
    os << req;
    std::string re_str = os.str();
    printf("%s\n", re_str.c_str());

    // Set the timeout.
    beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(30));

    // Send the HTTP request to the remote host
    co_await http::async_write(stream, req);

    // This buffer is used for reading and must be persisted
    beast::flat_buffer buffer;

    auto [ec, bytes_count] =
        co_await http::async_read(stream, buffer, response.response, as_tuple);

    if (ec) {
        response.ec = ec;
        co_return;
    }

    // Set the timeout.
    // Gracefully close the socket
    stream.socket().shutdown(net::ip::tcp::socket::shutdown_both, ec);

    // not_connected happens sometimes
    // so don't bother reporting it.
    //
    if (ec && ec != beast::errc::not_connected) {
        response.ec = ec;
    }
}

Uri Uri::parse(const std::string &uri) {
    // default constructor
    Uri res{};
    if (uri.empty()) {
        res.Error = "provided empty Uri";
        return res;
    }

    auto query_start = std::find(uri.begin(), uri.end(), '?');

    auto protocol_start = uri.begin();
    auto protocol_end = std::find(protocol_start, uri.end(), ':'); // "://"

    if (protocol_end == uri.end()) {
        res.Error = std::string("provided Url not have Protocal") + uri;
        return res;
    }

    // validate ://
    if (*protocol_end != ':' || (protocol_end + 1) == uri.end() ||
        *(protocol_end + 1) != '/' || (protocol_end + 2) == uri.end() ||
        *(protocol_end + 2) != '/') {
        res.Error = "provide Url invalid token :// for Protocol";
        return res;
    }

    res.Protocol = std::string(protocol_start, protocol_end);

    auto host_start = protocol_end + 3;
    auto path_start = std::find(host_start, uri.end(), '/');

    auto host_end = std::find(
        host_start, (path_start != uri.end()) ? path_start : query_start, ':');

    res.Host = std::string(host_start, host_end);

    // port
    if (host_end != uri.end() && *host_end == ':') {
        res.Porl = std::string(
            host_end + 1, (path_start != uri.end()) ? path_start : query_start);
    }

    // path
    if (path_start != uri.end()) {
        res.Path = std::string(path_start, query_start);
    }

    // query
    if (query_start != uri.end()) {
        res.QueryString = std::string(query_start, uri.end());
    }

    return res;
}

std::string Uri::target() const { return Path + QueryString; }

std::string Response::body() const { return response.body(); }

http::status Response::status() const { return response.result(); }
