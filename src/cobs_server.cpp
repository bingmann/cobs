/*******************************************************************************
 * src/cobs_server.cpp
 *
 * Copyright (c) 2020 Timo Bingmann
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>

#include <boost/asio/ip/tcp.hpp>

#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/steady_timer.hpp>

#include <functional>
#include <iostream>
#include <thread>

#include <tlx/cmdline_parser.hpp>
#include <tlx/logger.hpp>

/******************************************************************************/

// Report a failure
static inline
void fail(boost::system::error_code ec, const char* what)
{
    std::cerr << what << ": " << ec.message() << "\n";
}

// Report a failure
static inline
void fail_throw(boost::system::error_code ec, const char* what)
{
    std::ostringstream os;
    os << what << ": " << ec.message();
    throw std::runtime_error(os.str());
}

/******************************************************************************/

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = boost::beast::http;
using tcp = asio::ip::tcp;

using UrlHandler = std::function<
    bool(beast::string_view target, std::string& body)>;

class WebSession;

// Accepts incoming connections and launches the sessions
class WebServer : public std::enable_shared_from_this<WebServer>
{
public:
    WebServer(asio::io_service& io_service, tcp::endpoint endpoint)
        : io_service_(io_service),
          acceptor_(io_service),
          socket_(io_service)
    {
        boost::system::error_code ec;

        // open the acceptor
        acceptor_.open(endpoint.protocol(), ec);
        if (ec) {
            fail_throw(ec, "open");
            return;
        }

        acceptor_.set_option(asio::socket_base::reuse_address(true), ec);
        if (ec) {
            fail_throw(ec, "reuse_address");
            return;
        }

        typedef asio::detail::socket_option::boolean<
                SOL_SOCKET, SO_REUSEPORT> reuse_port;
        acceptor_.set_option(reuse_port(true), ec);
        if (ec) {
            fail_throw(ec, "reuse_port");
            return;
        }

        // bind to the server address
        acceptor_.bind(endpoint, ec);
        if (ec) {
            fail_throw(ec, "bind");
            return;
        }

        // start listening for connections
        acceptor_.listen(asio::socket_base::max_listen_connections, ec);
        if (ec) {
            fail_throw(ec, "listen");
            return;
        }
    }

    //! add document root to serve files from
    void set_doc_root(std::string doc_root)
    {
        doc_root_ = std::move(doc_root);
    }

    //! add url handler function
    void set_url_handler(UrlHandler url_handler)
    {
        url_handler_ = std::move(url_handler);
    }

    //! start accepting incoming connections
    void run()
    {
        if (!acceptor_.is_open())
            return;

        do_accept();
    }

protected:
    //! method to accept a new web client connection
    void do_accept()
    {
        acceptor_.async_accept(
            socket_,
            [t = shared_from_this()](boost::system::error_code ec) {
                t->on_accept(ec);
            });
    }

    //! accept a new client connection, create context, and do_accept()
    void on_accept(boost::system::error_code ec);

private:
    asio::io_service& io_service_;
    tcp::acceptor acceptor_;
    tcp::socket socket_;
    std::string doc_root_;
    UrlHandler url_handler_;

    friend class WebSession;
};

/******************************************************************************/

//! Return a reasonable mime type based on the extension of a file.
boost::beast::string_view
mime_type(boost::beast::string_view path)
{
    using boost::beast::iequals;
    auto const ext = [&path]() {
                         auto const pos = path.rfind(".");
                         if (pos == boost::beast::string_view::npos)
                             return boost::beast::string_view();
                         return path.substr(pos);
                     } ();

    if (iequals(ext, ".htm")) return "text/html";
    if (iequals(ext, ".html")) return "text/html";
    if (iequals(ext, ".php")) return "text/html";
    if (iequals(ext, ".css")) return "text/css";
    if (iequals(ext, ".txt")) return "text/plain";
    if (iequals(ext, ".js")) return "application/javascript";
    if (iequals(ext, ".json")) return "application/json";
    if (iequals(ext, ".xml")) return "application/xml";
    if (iequals(ext, ".png")) return "image/png";
    if (iequals(ext, ".jpe")) return "image/jpeg";
    if (iequals(ext, ".jpeg")) return "image/jpeg";
    if (iequals(ext, ".jpg")) return "image/jpeg";
    if (iequals(ext, ".gif")) return "image/gif";
    if (iequals(ext, ".bmp")) return "image/bmp";
    if (iequals(ext, ".ico")) return "image/vnd.microsoft.icon";
    if (iequals(ext, ".tiff")) return "image/tiff";
    if (iequals(ext, ".tif")) return "image/tiff";
    if (iequals(ext, ".svg")) return "image/svg+xml";
    if (iequals(ext, ".svgz")) return "image/svg+xml";
    return "application/text";
}

//! Append an HTTP rel-path to a local filesystem path.
//! The returned path is normalized for the platform.
std::string path_cat(boost::beast::string_view base,
                     boost::beast::string_view path)
{
    if (base.empty())
        return path.to_string();
    std::string result = base.to_string();
    char constexpr path_separator = '/';
    if (result.back() == path_separator)
        result.resize(result.size() - 1);
    result.append(path.data(), path.size());
    return result;
}

/******************************************************************************/

//! Handles an HTTP server connection
class WebSession : public std::enable_shared_from_this<WebSession>
{
public:
    //! take ownership of the socket
    WebSession(std::shared_ptr<WebServer> server, tcp::socket socket)
        : server_(std::move(server)),
          socket_(std::move(socket))
    { }

    //! start the asynchronous operation
    void run()
    {
        do_read();
    }

protected:
    //! read new request from web connection
    void do_read()
    {
        // read a request
        http::async_read(
            socket_, buffer_, request_,
            [t = shared_from_this()](
                boost::system::error_code ec, size_t bytes_transferred)
            {
                t->on_read(ec, bytes_transferred);
            });
    }

    //! read new request
    void on_read(boost::system::error_code ec, size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        // this means they closed the connection
        if (ec == http::error::end_of_stream)
            return do_close();

        if (ec)
            return fail(ec, "read");

        // send the response
        handle_request(request_);
    }

    //! on reading a request, handle it and optionally send a response.
    void handle_request(const http::request<http::string_body>& req);

    //! write response to client
    void on_write(boost::system::error_code ec, size_t bytes_transferred,
                  bool close)
    {
        boost::ignore_unused(bytes_transferred);

        if (ec)
            return fail(ec, "write");

        if (close) {
            // this means we should close the connection, usually because the
            // response indicated the "Connection: close" semantic.
            return do_close();
        }

        // we're done with the response so delete it
        response_ = nullptr;

        // read another request
        do_read();
    }

    //! close connection gracefully
    void do_close()
    {
        // send a TCP shutdown
        boost::system::error_code ec;
        socket_.shutdown(tcp::socket::shutdown_send, ec);

        // at this point the connection is closed gracefully
    }

    template <bool isRequest, class Body, class Fields>
    void send(http::message<isRequest, Body, Fields>&& msg)
    {
        // the lifetime of the message has to extend for the duration of the
        // async operation so we use a shared_ptr to manage it.
        auto sp = std::make_shared<
            http::message<isRequest, Body, Fields> >(std::move(msg));

        // store a type-erased version of the shared pointer in the class to
        // keep it alive.
        response_ = sp;

        // write the response
        http::async_write(
            socket_, *sp,
            [t = shared_from_this(), close = sp->need_eof()](
                boost::system::error_code ec, size_t bytes_transferred)
            {
                t->on_write(ec, bytes_transferred, close);
            });
    }

    void send_error(http::status status, std::string message)
    {
        http::response<http::string_body> res(status, request_.version());
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(request_.keep_alive());
        res.body() = std::move(message);
        res.prepare_payload();
        return send(std::move(res));
    }

private:
    std::shared_ptr<WebServer> server_;
    tcp::socket socket_;
    boost::beast::flat_buffer buffer_;
    http::request<http::string_body> request_;
    std::shared_ptr<void> response_;
};

/******************************************************************************/
// WebServer definitions

void WebServer::on_accept(boost::system::error_code ec)
{
    if (ec) {
        fail(ec, "accept");
    }
    else {
        // create the WebSession and run it
        std::make_shared<WebSession>(
            shared_from_this(), std::move(socket_))->run();
    }

    // accept another connection
    do_accept();
}

/******************************************************************************/
// WebSession definitions

//! This function produces an HTTP response for the given request. The type of
//! the response object depends on the contents of the request, so the interface
//! requires the caller to pass a generic lambda for receiving the response.
void WebSession::handle_request(const http::request<http::string_body>& req)
{
    // make sure we can handle the method
    if (req.method() != http::verb::get && req.method() != http::verb::head)
        return send_error(http::status::bad_request, "Unknown HTTP-method");

    // check for generated web page
    std::string string_body;
    if (server_->url_handler_ != nullptr &&
        server_->url_handler_(req.target(), string_body)) {
        // respond to HEAD request
        if (req.method() == http::verb::head) {
            http::response<http::empty_body> res(http::status::ok, req.version());
            res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
            res.set(http::field::content_type, "text/html");
            res.content_length(string_body.size());
            res.keep_alive(req.keep_alive());
            return send(std::move(res));
        }

        // respond to GET request
        http::response<http::string_body> res(
            std::piecewise_construct,
            std::make_tuple(std::move(string_body)),
            std::make_tuple(http::status::ok, req.version()));
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.prepare_payload();
        res.keep_alive(req.keep_alive());
        return send(std::move(res));
    }

    if (!server_->doc_root_.empty()) {
        // request path must be absolute and not contain "..".
        if (req.target().empty() || req.target()[0] != '/' ||
            req.target().find("..") != boost::beast::string_view::npos)
            return send_error(http::status::bad_request,
                              "Illegal request-target");

        // build the path to the requested file
        std::string path = path_cat(server_->doc_root_, req.target());
        if (req.target().back() == '/')
            path.append("index.html");

        // attempt to open the file
        boost::beast::error_code ec;
        http::file_body::value_type body;
        body.open(path.c_str(), boost::beast::file_mode::scan, ec);

        // handle the case where the file doesn't exist
        if (ec == boost::system::errc::no_such_file_or_directory)
            return send_error(
                http::status::not_found,
                "The resource '" + req.target().to_string() + "' was not found.");

        // handle an unknown error
        if (ec) {
            return send_error(
                http::status::internal_server_error,
                "An error occurred: '" + ec.message() + "'");
        }

        // respond to HEAD request
        if (req.method() == http::verb::head) {
            http::response<http::empty_body> res(http::status::ok, req.version());
            res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
            res.set(http::field::content_type, mime_type(path));
            res.content_length(body.size());
            res.keep_alive(req.keep_alive());
            return send(std::move(res));
        }

        // respond to GET request
        http::response<http::file_body> res{
            std::piecewise_construct,
            std::make_tuple(std::move(body)),
            std::make_tuple(http::status::ok, req.version())
        };
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, mime_type(path));
        res.content_length(body.size());
        res.keep_alive(req.keep_alive());
        return send(std::move(res));
    }

    return send_error(
        http::status::not_found,
        "The resource '" + req.target().to_string() + "' was not found.");
}

/******************************************************************************/

bool make_page(beast::string_view target, std::string& body)
{
    std::cout << "URL: " << target.to_string() << std::endl;

    if (target == "/test") {
        std::ostringstream os;
        os << std::this_thread::get_id();
        body = "hello, I am " + os.str();

        return true;
    }

    return false;
}

/******************************************************************************/

int main(int argc, char** argv) {

    tlx::set_logger_to_stderr();

    tlx::CmdlineParser cp;

    std::vector<std::string> index_files;
    cp.add_stringlist(
        'i', "index", index_files, "path to index file(s)");

    std::string listen_address;
    cp.add_string(
        'l', "listen", listen_address, "IP to listen on, default: all");

    unsigned int port = 5484;
    cp.add_unsigned(
        'p', "port", port, "port to listen on, default: 5484");

    unsigned int num_threads = 0;
    cp.add_unsigned(
        't', "threads", num_threads, "number of query threads, default: 1");

    if (!cp.sort().process(argc, argv))
        return -1;

    // check result of command line parsing

    // if (index_files.size() != 1) {
    //     die("Supply one index file, TODO: add support for multiple.");
    // }

    boost::asio::ip::address address;
    if (listen_address.empty())
        address = boost::asio::ip::address_v4::any();
    else
        address = boost::asio::ip::make_address(listen_address);

    if (num_threads == 0)
        num_threads = 4;

    // Launch Server

    boost::asio::io_context io_context(num_threads);

    LOG1 << "Launching web server on address " << address << " port " << port
         << " with " << num_threads << " threads";

    // create and launch a listening port
    auto ws = std::make_shared<WebServer>(
        io_context, tcp::endpoint(address, port));
    ws->set_doc_root("./doc-root/");
    ws->set_url_handler(make_page);
    ws->run();

    // run the I/O service on the requested number of threads
    std::vector<std::thread> threads(num_threads - 1);
    for (unsigned i = 0; i + 1 < num_threads; ++i) {
        threads[i] = std::thread([&io_context] { io_context.run(); });
    }
    io_context.run();

    return EXIT_SUCCESS;
}

/******************************************************************************/
