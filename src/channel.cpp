#include "channel.hpp"

void handler(const asio::error_code& error,std::size_t bytes_transferred){}

Channel::Channel(const std::string &_channelName, BotEventQueue &evq, const asio::io_service &io_s, ConnHandler *_owner)
	:	channelName{_channelName},
		eventQueue{evq},
		pingReplied(false),
		quit(false),
        sock(io_s),
        owner(_owner),
        readThread(std::thread([this](){this->read();}))
{
    asio::connect(*sock, owner->twitch_it);
	
	std::string passx = "PASS " + owner->pass + "\r\n";
	std::string nickx = "NICK " + owner->nick + "\r\n";
	std::string cmds = "CAP REQ :twitch.tv/commands\r\n";
	std::string join = "JOIN #" + channelName + "\r\n";
	
	sock.send(asio::buffer(passx));
	sock.send(asio::buffer(nickx));
	sock.send(asio::buffer(cmds));	
	sock.send(asio::buffer(join));
    
    //thread reading the socket
    //readThread = std::thread([this]()
	//{
	//	this->read();
	//});
    //readThread.detach();
}

Channel::~Channel()
{
    quit = true;    
    sock.shutdown(asio::ip::tcp::socket::shutdown_both);
    sock.close();
    readThread.join();
}

bool Channel::sendMsg(const std::string &msg)
{
	auto timeNow = std::chrono::high_resolution_clock::now();
	if(messageCount < 19 && std::chrono::duration_cast<std::chrono::milliseconds>(timeNow - lastMessageTime).count() > 1500)
	{
		std::string sendirc = "PRIVMSG #" + channelName + " :";
		sendirc += msg + ' ';
		if(sendirc.length() > 387)
			sendirc = sendirc.substr(0, 387) + "\r\n";
		else sendirc += "\r\n";
		sock->async_send(asio::buffer(sendirc), handler); // define handler
		messageCount++;
		lastMessageTime = timeNow;
		return true;
	}
	else 
	{
		return false;
	}
}

void Channel::read()
{
	try
	{
		while(!(this->quit))
		{	
			std::unique_ptr<asio::streambuf> b(new asio::streambuf);
			asio::error_code ec;
			asio::read_until(*(sock), *b, "\r\n", ec);				
			if(!(ec && !(this->quit)))
			{
				eventQueue.push(std::pair<std::unique_ptr<asio::streambuf>, std::string>(std::move(b), channelName));
			}
			else
			{
				//else reconnect
				/*
				if(this->channelBools.count(channelName) == 1)
				{
					this->joinChannel(channelName);
				}
				return;
				*/
			}
		}
	}
	catch (std::exception& e)
	{
		std::cout << "exception running Channel" << channelName << " " << e.what() << std::endl;
	}
}








