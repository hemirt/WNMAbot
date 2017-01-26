#include "connection.hpp"
#include "parser.hpp"

#include <boost/bind.hpp>

Connection::Connection(boost::asio::io_service &ioService,
                       const BoostConnection::endpoint &_endpoint,
                       const Credentials &_credentials,
                       const std::string &_channelName,
                       MessageHandler *_handler)
    : socket(ioService)
    , endpoint(_endpoint)
    , credentials(_credentials)
    , channelName(_channelName)
    , handler(_handler)
{
    // When the connection object is started, we start connecting
    std::cout << "connection: " << this->channelName << std::endl;
    this->startConnect();
}

Connection::~Connection()
{
    this->socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
    this->socket.close();
}

void
Connection::writeRawMessage(const std::string &message)
{
    const auto asioBuffer = boost::asio::buffer(message);

    boost::asio::async_write(this->socket, asioBuffer,
                             boost::bind(&Connection::handleWrite, this, _1));
}

void
Connection::writeMessage(const std::string &message)
{
    if(!this->established) {
        return;
    }
    this->writeRawMessage(message + " \r\n");
}

void
Connection::startConnect()
{
    this->socket.async_connect(
        this->endpoint, boost::bind(&Connection::handleConnect, this, _1));
}

void
Connection::doRead()
{
    boost::asio::async_read_until(
        this->socket, this->inputBuffer, "\r\n",
        boost::bind(&Connection::handleRead, this, _1, _2));
}

void
Connection::handleRead(const boost::system::error_code &ec,
                       std::size_t bytesTransferred)
{
    if (ec) {
        this->handleError(ec);
        return;
    }

    auto bufs = this->inputBuffer.data();
    std::string rawMessage(boost::asio::buffers_begin(bufs),
                           boost::asio::buffers_begin(bufs) + bytesTransferred);
    this->inputBuffer.consume(bytesTransferred);

    auto ircMessage = Parser::parseMessage(rawMessage);

    this->handler->handleMessage(ircMessage);

    this->doRead();
}

void
Connection::handleWrite(const boost::system::error_code &ec)
{
    if (ec) {
        this->handleError(ec);
        return;
    }
}

void
Connection::doWrite()
{
}

void
Connection::handleConnect(const boost::system::error_code &ec)
{
    if (!this->socket.is_open()) {
        // Connection timed out, machine unreachable?

        this->startConnect();
    } else if (ec) {
        this->socket.close();

        this->handleError(ec);

        this->startConnect();
    } else {
        this->onConnectionEstablished();
    }
}

void
Connection::handleError(const boost::system::error_code &ec)
{
    std::cerr << "Handle error: " << ec << std::endl;
    if(ec == boost::asio::error::eof)
    {
        this->socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
        this->socket.close();
        this->startConnect();
    }
}

void
Connection::onConnectionEstablished()
{
    this->doRead();

    this->writeRawMessage("PASS " + this->credentials.password + "\r\n");
    this->writeRawMessage("NICK " + this->credentials.username + "\r\n");
    this->writeRawMessage("CAP REQ :twitch.tv/commands\r\n");
    this->writeRawMessage("JOIN #" + this->channelName + "\r\n");
    
    this->established = true;
}
