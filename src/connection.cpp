#include "connection.hpp"
#include "parser.hpp"
#include "utilities.hpp"
#include "channel.hpp"
#include "connectionhandler.hpp"

#include <iomanip>
#include <thread>

#include <boost/bind.hpp>

Connection::Connection(boost::asio::io_service &ioService,
                       const BoostConnection::endpoint &_endpoint,
                       const Credentials &_credentials,
                       const std::string &_channelName)
    : ioService(ioService)
    , socket(new BoostConnection::socket(ioService))
    , endpoint(_endpoint)
    , credentials(_credentials)
    , channelName(_channelName)
{
    // When the connection object is started, we start connecting
    std::cout << "connection: " << this->channelName << std::endl;
    //this->startConnect();
}

Connection::~Connection()
{
    this->shutdown();
    std::cout << "Connection::~Connection(): " << this->channelName << std::endl;
}

void
Connection::shutdown()
{
    std::scoped_lock lock(this->connM, this->handlerM);
    this->quit = true;
    this->handler.reset();
    if (this->socket != nullptr) {
        this->socket->shutdown(boost::asio::ip::tcp::socket::shutdown_both);
        this->socket->close();
        this->socket.reset();
    }
    std::cout << "connection was shut down " << this->channelName << std::endl;
}

void
Connection::writeRawMessage(const std::string &message)
{
    const auto asioBuffer = boost::asio::buffer(message);

    boost::asio::async_write(*this->socket, asioBuffer,
                             boost::bind(&Connection::handleWrite, this, this->socket, _1));
}

void
Connection::pong()
{
    if (!this->established) {
        this->socket->close();

        this->startConnect(this->socket);

        return;
    }
    this->writeRawMessage("PONG :tmi.twitch.tv\r\n");
}

void
Connection::writeMessage(const std::string &message)
{
    if (!this->established || this->quit) {
        return;
    }
    this->duplicateMsg = !this->duplicateMsg;
    if (this->duplicateMsg) {
        this->writeRawMessage(message + " ⁭\r\n");
    } else {
        this->writeRawMessage(message + " \r\n");
    }
}

void
Connection::startConnect(std::shared_ptr<Channel> chn)
{
    {
        std::scoped_lock lock(this->connM, this->handlerM);
        this->handler = chn;
    }
    this->startConnect();
}

void
Connection::startConnect()
{
    this->socket->async_connect(
        this->endpoint, boost::bind(&Connection::handleConnect, this, _1));
}

void
Connection::startConnect([[maybe_unused]] std::shared_ptr<BoostConnection::socket> sock)
{
    this->socket->async_connect(
        this->endpoint, boost::bind(&Connection::handleConnect, this->shared_from_this(), _1));
}

void
Connection::doRead()
{
    boost::asio::async_read_until(
        *this->socket, this->inputBuffer, "\r\n",
        boost::bind(&Connection::handleRead, this->shared_from_this(), this->socket, _1, _2));
}

void
Connection::handleRead([[maybe_unused]] std::shared_ptr<BoostConnection::socket> sock,
                       const boost::system::error_code &ec,
                       std::size_t bytesTransferred)
{
    if (ec) {
        std::cout << "from handleRead" << std::endl;
        this->handleError(ec);
        return;
    }

    auto bufs = this->inputBuffer.data();
    std::string rawMessage(boost::asio::buffers_begin(bufs),
                           boost::asio::buffers_begin(bufs) + bytesTransferred);
    this->inputBuffer.consume(bytesTransferred);

    auto ircMessage = Parser::parseMessage(rawMessage);

    {
        std::unique_lock lk1(this->connM, std::defer_lock);
        std::unique_lock lk2(this->handlerM, std::defer_lock);
        std::lock(lk1, lk2);

        if (this->handler == nullptr) {
            return;
        }
        if (this->socket == nullptr) {
            return;
        }
        
        auto copy = this->handler;
        lk1.unlock();
        lk2.unlock();
        
        if(!copy->handleMessage(ircMessage)) {
            return;
        }
    }

    this->doRead();
}

void
Connection::handleWrite([[maybe_unused]] std::shared_ptr<BoostConnection::socket> sock, const boost::system::error_code &ec)
{
    if (ec) {
        std::cout << "from handleWrite" << std::endl;
        this->handleError(ec);
        return;
    }
}

void
Connection::doWrite()
{
}

void
Connection::reconnect()
{
    std::scoped_lock lock(this->connM, this->handlerM);
    if (this->quit) {
        std::cerr << this->channelName << ": connection is quitting, cant reconnect" << std::endl;
        return;
    }
    boost::system::error_code ec;
    if (this->socket == nullptr) {
        std::cerr << this->channelName << ": connection socket is nullptr, cant reconnect" << std::endl;
        return;
    }
    this->socket->shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
    if (ec) {
        std::cerr << "Reconnect: shutdown: ec: " << ec << " msg: " << ec.message() << std::endl;
        return;
    }
    this->socket->close();
    if (ec) {
        std::cerr << "Reconnect: close: ec: " << ec << " msg: " << ec.message() << std::endl;
        return;
    }
    this->socket.reset(new BoostConnection::socket(ioService));
    this->inputBuffer.consume(this->inputBuffer.size());
    this->startConnect(this->socket);
    std::cout << "Connection " << channelName << " reconnected." << std::endl;
}

void
Connection::handleConnect(const boost::system::error_code &ec)
{
    std::unique_lock lockconn(this->connM, std::defer_lock);
    std::unique_lock lockhandler(this->handlerM, std::defer_lock);
    std::lock(lockconn, lockhandler);
    std::cout << "HANDLECONNECTING " << this->channelName << std::endl;
    if (this->socket == nullptr) {
        std::cerr << "CNN NULLPTR" << std::endl;
        return;
    }
    
    if (!this->socket->is_open()) {
        // Connection timed out, machine unreachable?
        std::cerr << "socket is not open startcon" << this->channelName << std::endl;
        this->startConnect(this->socket);
    } else if (ec) {
        lockconn.unlock();
        lockhandler.unlock();
        std::cerr << "from handleConnect" << std::endl;
        this->handleError(ec);
        // this->reconnect(); // maybe this fixes it?
    } else {
        this->onConnectionEstablished();
    }
}

void
Connection::handleError(const boost::system::error_code &ec)
{
    std::unique_lock lk1(this->connM, std::defer_lock);
    std::unique_lock lk2(this->handlerM, std::defer_lock);
    std::lock(lk1, lk2);
    std::cerr << "Handle error " << this->channelName << ": " << utcDateTime() << ": " << ec << " msg: " << ec.message() << std::endl;
    if (ec == boost::asio::error::eof || ec == boost::system::errc::timed_out || ec == boost::system::errc::bad_file_descriptor) {
        //this->socket->shutdown(boost::asio::ip::tcp::socket::shutdown_both);
        //this->socket->close();
        //this->startConnect();
        std::cerr << "attempting to reconnect" << std::endl;
        if (this->handler == nullptr) {
            std::cerr << "this->handler is nullptr" << std::endl;
            return;
        }
        auto ec = this->handler->owner->resetEndpoint();
        if (ec) {
            std::cerr << "resetEndpoint() ec: " << ec << " msg: " << ec.message() << std::endl;
            return;
        }
        if (this->handler == nullptr) {
            std::cerr << "this->handler 2 is nullptr" << std::endl;
            return;
        }
        this->endpoint = this->handler->owner->getEndpoint();
        lk1.unlock();
        lk2.unlock();
        this->reconnect();
    }
}

void
Connection::onConnectionEstablished()
{
    this->doRead();

    this->writeRawMessage("PASS " + this->credentials.password + "\r\n");
    this->writeRawMessage("NICK " + this->credentials.username + "\r\n");
    this->writeRawMessage("CAP REQ :twitch.tv/commands\r\n");
    this->writeRawMessage("CAP REQ :twitch.tv/tags\r\n");
    this->writeRawMessage("JOIN #" + this->channelName + "\r\n");

    this->established = true;
}
