

#ifndef GUARD_REQUEST_CLIENT
#define GUARD_REQUEST_CLIENT

#include "boost/asio/ssl/context.hpp"
#include "boost/beast/core/tcp_stream.hpp"
#include "boost/beast/http/field.hpp"
#include "boost/beast/http/message_fwd.hpp"
#include "boost/beast/http/status.hpp"
#include "boost/beast/http/string_body_fwd.hpp"
#include "boost/system/detail/error_code.hpp"
#include <boost/asio/as_tuple.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <map>

#include <boost/certify/extensions.hpp>
#include <boost/certify/https_verification.hpp>

namespace beast = boost::beast;
namespace http = beast::http;
namespace ssl = boost::asio::ssl;

using boost::asio::awaitable;
using boost::system::error_code;

using boost::asio::as_tuple;

using Stream = beast::tcp_stream;
using StreamSSL = ssl::stream<Stream>;

using AdditionalHeaders = std::map<http::field, std::string>;

struct Uri {
    std::string QueryString, Path, Protocol, Host, Porl, Error;
    static Uri parse(const std::string &uri);
    std::string target() const;
    void pretty_print() const;
};

struct Response {
    error_code ec;
    std::string user_error;
    http::response<http::string_body> response;

    std::string body() const;
    http::status status() const;
};

class RequestClient {
  public:
    RequestClient();
    ~RequestClient();

    awaitable<Response> get(const std::string &endpoint,
                            AdditionalHeaders headers = {});

    awaitable<Response> post(const std::string &endpoint, std::string &&body,
                             AdditionalHeaders headers = {});

  private:
    awaitable<Response> https_get(const Uri &uri, AdditionalHeaders &&headers);
    awaitable<Response> https_post(const Uri &uri, std::string &&body,
                                   AdditionalHeaders &&headers);

    awaitable<Response> http_get(const Uri &uri, AdditionalHeaders &&headers);
    awaitable<Response> http_post(const Uri &uri, std::string &&body,
                                  AdditionalHeaders &&headers);

    template <class RequestBody>
    awaitable<void>
    https_send_and_read_response(const std::string &host, StreamSSL &stream,
                                 http::request<RequestBody> &req,
                                 Response &response);

    template <class RequestBody>
    awaitable<void> http_send_and_read_response(const std::string &host,
                                                Stream &stream,
                                                http::request<RequestBody> &req,
                                                Response &response);

    ssl::context ctx;
};

#endif // GUARD_REQUEST_CLIENT
