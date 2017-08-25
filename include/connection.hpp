#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include "credentials.hpp"
#include "messagehandler.hpp"
#include "network.hpp"

#include <iostream>
#include <memory>
#include <mutex>

class Connection : public std::enable_shared_from_this<Connection>
{
public:
    Connection(boost::asio::io_service &ioService,
               const BoostConnection::endpoint &_endpoint,
               const Credentials &_credentials, const std::string &_channelName,
               MessageHandler *_handler);
    virtual ~Connection();

    void writeMessage(const std::string &message);
    void pong();

    bool established = false;
    void reconnect();

private:
    // Start connecting to the stored endpoint
    void startConnect();
    void startConnect(std::shared_ptr<BoostConnection::socket> sock);

    void writeRawMessage(const std::string &message);

    void doRead();
    void doWrite();

    void handleConnect(const boost::system::error_code &ec);
    void handleRead(std::shared_ptr<BoostConnection::socket> sock, const boost::system::error_code &ec,
                    std::size_t bytesTransferred);
    void handleError(const boost::system::error_code &ec);
    void handleWrite(std::shared_ptr<BoostConnection::socket> sock, const boost::system::error_code &ec);

    void onConnectionEstablished();

    boost::asio::io_service &ioService;
    std::shared_ptr<BoostConnection::socket> socket;
    BoostConnection::endpoint endpoint;

    boost::asio::streambuf inputBuffer;

    Credentials credentials;
    std::string channelName;

    MessageHandler *handler;
    bool duplicateMsg = false;
    
    std::mutex connM;
};

#endif  // CONNECTION_HPP
