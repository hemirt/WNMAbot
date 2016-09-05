#include <iostream>
#include "asio.hpp"
#include <chrono>
#include <thread>
#include <map>
#include <mutex>
#include <ctime>
#include <set>
#include <fstream>
#include <random>
#include <iomanip>
#include <locale>
#include <condition_variable>
#include <list>
#include "curl/curl.h"
#include <limits>
#include "channels.hpp"
#include "channelitems.hpp"
#include "itemincrements.hpp"

struct Command
{
	std::string who;
	std::string action;
	std::string data;
};

void changeToLower(std::string& str)
{
	static std::vector<std::string> vekcharup{"Á", "Č", "Ď", "É", "Ě", "Í", "Ň", "Ó", "Ř", "Š", "Ť", "Ú", "Ů", "Ý", "Ž"};
	static std::vector<std::string> vekchardown{"á", "č", "ď", "é", "ě", "í", "ň", "ó", "ř", "š", "ť", "ú", "ů", "ý", "ž"};

	for(int i = 0; i < vekcharup.size(); ++i)
	{
		size_t pos = 0;
		while((pos = str.find(vekcharup[i], 0)) != std::string::npos)
		{
			str.replace(pos, vekcharup[i].size(), vekchardown[i], 0, vekchardown[i].size());
		}
	}
	std::transform(str.begin(), str.end(), str.begin(),[](char c){return std::tolower(c, std::locale("cs_CZ.utf8"));});
}

std::string timenow()
{
	std::time_t result = std::time(nullptr);
	std::stringstream ss;
	ss.imbue(std::locale("cs_CZ.utf8"));
	ss << std::put_time(std::localtime(&result), "%c");
	return ss.str();
}

//provided by old_forsen
template <typename T>
class EventQueue {
    std::list<T> _queue;
    std::condition_variable cv;
    std::mutex m;
    std::unique_lock<std::mutex> lk;
 
public:
    void push(T t) {
        std::unique_lock<std::mutex> lk(m);
        _queue.push_back(move(t));
        lk.unlock();
        cv.notify_all();
    }
	
	void notify() {
        cv.notify_all();
    }
 
    T pop() {
        std::unique_lock<std::mutex> lk(m);
        T value = std::move(_queue.front());
        _queue.pop_front();
        lk.unlock();
        return value;
    }
 
    void wait() {
        std::unique_lock<std::mutex> lk(m);
        cv.wait(lk);
    }
 
    template<typename Lambda>
    void wait(Lambda f) {
        std::unique_lock<std::mutex> lk(m);
        cv.wait(lk);
        lk.unlock();
        f();
    }
 
    bool empty() {
        std::unique_lock<std::mutex> lk(m);
        bool e = _queue.empty();
        lk.unlock();
        return e;
    }
 
    size_t size() {
        std::unique_lock<std::mutex> lk(m);
        size_t s = _queue.size();
        lk.unlock();
        return s;
    }
};

class IrcConnection
{
public:
	IrcConnection();
	~IrcConnection();
	asio::io_service m_io_service;
	std::map<std::string, std::shared_ptr<asio::ip::tcp::socket>> channelSockets;
	void stop();
	void start(const std::string& pass, const std::string& nick);
	std::map<std::string, std::chrono::high_resolution_clock::time_point> channelTimes;
	void joinChannel(const std::string&);
	void leaveChannel(const std::string&);
	bool sendMsg(const std::string& channel, const std::string& message);
	bool isAdmin(const std::string& user);
	void addAdmin(const std::string& admin);
	void addCmd(const std::string& cmd, std::string msg);
	std::multimap<std::string, Command> commands;
	int dice()
	{
		return this->dist(this->mt);
	}
	void listenAndHandle(const std::string& channel);
	std::map<std::string, bool> channelBools;
	std::map<std::string, unsigned> channelMsgs;
	std::mutex irc_m;
	bool quit();
	bool quit_m = false;
	std::vector<std::thread> threads;
	void waitEnd()
	{
		std::unique_lock<std::mutex> lock(irc_m);
		this->quit_cv.wait(lock, [this](){return this->quit_m;});
		lock.unlock();
		for(auto& th : this->threads) 
		{
			if(th.joinable())
				th.join();
		}
	}
	EventQueue<std::pair<std::unique_ptr<asio::streambuf>, std::string>> eventQueue;
	void handleCommands(std::string&, const std::string&, std::string&);
	Channels channels;
	struct Pings
	{
		std::condition_variable cv;
		std::mutex mtx;
		bool pinged = false;
	};
	std::map<std::string, std::unique_ptr<Pings>> pingMap;
	void IncrementLoop();
	ItemIncrements itemincr;
private:
	std::unique_ptr<asio::io_service::work> wrk;
	void run();
	void msgCount();
	void processEventQueue();
	std::fstream commandsFile;
	std::fstream adminsFile;
	std::set<std::string> admins;
	std::random_device rd;
	std::mt19937 mt{this->rd()};
	std::uniform_int_distribution<int> dist{0, 100};
	std::string pass;
	std::string nick;
	asio::ip::tcp::resolver::iterator twitch_it;
	std::condition_variable quit_cv;
	std::chrono::high_resolution_clock::time_point currentTrigger = std::chrono::high_resolution_clock::now();
};

size_t writeFn(void *contents, size_t size, size_t nmemb, void *userp)
{
	((std::string*)userp)->append((char*)contents, size * nmemb);
	return size * nmemb;
}

void IrcConnection::IncrementLoop()
{
	int current = 0;
	while(!(this->quit()))
	{
		std::unique_lock<std::mutex> lock(irc_m);
		if(this->quit_cv.wait_for(lock, std::chrono::minutes(1),[this](){return this->quit_m;}))			
		{
			std::cout << "quiting IncrementLoop" << std::endl;
			return;
		}
		else // 1 minute passed
		{	
			++current;
			CURL *curl;
			CURLcode res;
			curl = curl_easy_init();
			struct curl_slist *chunk = NULL;
			chunk = curl_slist_append(chunk, "Accept: application/json");
			
			for(auto& nm : this->channels.channelsItemsMap)
			{
				
				std::string tmi = "http://tmi.twitch.tv/group/user/" + nm.first + "/chatters";
				std::string readBuffer;
				std::vector<std::string> readVector;
				
				res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
				curl_easy_setopt(curl, CURLOPT_URL, tmi.c_str());
				curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeFn);
				curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
				res = curl_easy_perform(curl);
				curl_easy_reset(curl);
				size_t	pos;
				size_t endpos;
				std::vector<std::string> chatters;
				std::string fullchatters;
				pos = readBuffer.find("\"moderators\": [");
				if(pos == std::string::npos) continue;
				pos += strlen("\"moderators\": [");
				endpos = readBuffer.find("]", pos);
				fullchatters.append(readBuffer.substr(pos, endpos-pos));
				pos = readBuffer.find("\"viewers\": [");
				if(pos == std::string::npos) continue;
				pos += strlen("\"viewers\": [");
				endpos = readBuffer.find("]", pos);
				fullchatters.append(readBuffer.substr(pos, endpos-pos));
				pos = 0;
				while((pos = fullchatters.find("\"", pos + 1)) != std::string::npos)
				{
					endpos = fullchatters.find("\"", pos + 1);
					chatters.push_back(fullchatters.substr(pos + 1, endpos-pos - 1));
					pos = endpos;
				}
				std::vector<ItemIncrements::Increments> currentOnes;
				for(auto i : this->itemincr.allIncrements)
				{
					if(current % i.trigger == 0)
					{
						currentOnes.push_back(i);
					}
				}
				auto start = std::chrono::high_resolution_clock::now();
				struct incr
				{
					std::string name;
					std::string what;
					unsigned long long current;
				};
				std::unique_lock<std::mutex> lk(*(nm.second.mtx));
				std::vector<incr> vek;
				for(auto name : chatters)
				{
					std::map<std::string, unsigned long long> increases;
					for(auto i : currentOnes)
					{
						auto it = increases.find(i.what);
						if(it == increases.end()) increases[i.what] = 0;
						unsigned long long current = 0;
						unsigned long long adding = 0;
						
						if(i.per == "default")
						{
							increases[i.what] += i.howmuch;
						}
						else
						{
							auto pair = nm.second.get(name, i.per);
							if(pair.first == false) continue;
							increases[i.what] += i.howmuch * pair.second;
						}
					}
					for(auto i : increases)
					{
						auto pair = nm.second.get(name, i.first);
						unsigned long long current;
						if(pair.first == false) current = 0;
						else current = pair.second;
						current += i.second;
						vek.push_back({name, i.first, current});
					}
					
				}
				for(const auto& i :vek)
				{
					nm.second.insert(i.name, i.what, i.current);
				}
				nm.second.writeFile();
				lk.unlock();
				auto end = std::chrono::high_resolution_clock::now();
				std::cout << nm.first << " took: " << std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() << " ns" << std::endl;
			}
		}
	}
}

IrcConnection::IrcConnection()
{
	this->adminsFile.open("admins.txt", std::ios::in);
	if(this->adminsFile.is_open() == false)
	{
		this->adminsFile.open("admins.txt", std::ios::out);
		this->adminsFile << "hemirt" << std::endl;
		this->adminsFile.close();
		this->adminsFile.open("admins.txt", std::ios::in);
	}
	{
		std::string line;
		while(std::getline(this->adminsFile, line))
		{
			this->admins.insert(line);
		}
		this->adminsFile.close();
		this->adminsFile.open("admins.txt", std::ios::out | std::ios::app);
	}
	
	this->commandsFile.open("commands.txt", std::ios::in);
	if(this->commandsFile.is_open() == false)
	{
		this->commandsFile.open("commands.txt", std::ios::out);
		this->commandsFile << "!addcmd admins Done" << std::endl;
		this->commandsFile.close();
		this->commandsFile.open("commands.txt", std::ios::in);
	}
	{
		std::string line;
		while(std::getline(this->commandsFile, line))
		{
			std::string delimiter = "#";
			std::vector<std::string> vek;
			size_t pos = 0;
			int i = 1;
			while((pos = line.find(delimiter)) != std::string::npos && i < 4)
			{
				vek.push_back(line.substr(0, pos));
				line.erase(0, pos + delimiter.length());
				i++;
			}
			vek.push_back(line);
			if(vek.size() == 4)
			{
				changeToLower(vek[0]);
				changeToLower(vek[1]);
				Command cmd = {vek[1], vek[2], vek[3]};
				this->commands.insert(std::pair<std::string, Command>(vek[0], cmd));
			}
		}
		this->commandsFile.close();
		this->commandsFile.open("commands.txt", std::ios::out | std::ios::app);
	}

	std::cout << "lulopxened\n";
}
IrcConnection::~IrcConnection()
{
	this->adminsFile.close();
	this->commandsFile.close();
}

bool IrcConnection::isAdmin(const std::string& user)
{
	if(this->admins.count(user) == 1)
		return true;
	else return false;
}

void IrcConnection::addAdmin(const std::string& admin)
{
	if(this->admins.count(admin) == 0)
	{
		this->admins.insert(admin);
		this->adminsFile.seekp(0, std::ios::end);
		this->adminsFile << admin << std::endl;
	}
}

void IrcConnection::addCmd(const std::string& cmd, std::string message)
{
	std::pair<std::multimap<std::string, Command>::const_iterator, std::multimap<std::string, Command>::const_iterator> ret = this->commands.equal_range(cmd);
		
	Command cmds;
		
	std::string delimiter = "#";
	std::vector<std::string> vek;
	size_t pos = 0;
	int i = 1;
	while((pos = message.find(delimiter)) != std::string::npos && i < 3)
	{
		vek.push_back(message.substr(0, pos));
		message.erase(0, pos + delimiter.length());
		i++;
	}
	vek.push_back(message);
	if(vek.size() == 3)
	{
		cmds = {vek[0], vek[1], vek[2]};
	
		for(auto& itr = ret.first; itr != ret.second; ++itr)
		{
			if(itr->second.who == cmds.who)
			{
				return;
			}
		}
		this->commands.insert(std::pair<std::string, Command>(cmd, cmds));
		this->commandsFile.seekp(0, std::ios::end);
		this->commandsFile << cmd << "#" << cmds.who << "#" << cmds.action << "#" << cmds.data << std::endl;
	}

}

void IrcConnection::joinChannel(const std::string& chn)
{
	std::shared_ptr<asio::ip::tcp::socket> sock(new asio::ip::tcp::socket(this->m_io_service));
	this->channelSockets.insert(std::pair<std::string, std::shared_ptr<asio::ip::tcp::socket>>(chn, sock));
	asio::connect(*sock, this->twitch_it);
	
	std::string passx = "PASS " + this->pass + "\r\n";
	std::string nickx = "NICK " + this->nick + "\r\n";
	std::string cmds = "CAP REQ :twitch.tv/commands\r\n";
	
	sock->send(asio::buffer(passx));
	sock->send(asio::buffer(nickx));
	sock->send(asio::buffer(cmds));	
	
	std::string join = "JOIN #" + chn + "\r\n";
	sock->send(asio::buffer(join));
	this->channelTimes.insert(std::pair<std::string, std::chrono::high_resolution_clock::time_point>(chn, std::chrono::high_resolution_clock::now()));
	this->channelMsgs.insert(std::pair<std::string, unsigned>(chn, 0));
	this->channelBools.insert(std::pair<std::string, bool>(chn, false));
	std::cout << "joined" << chn << std::endl;
	
	/*
	asio::error_code ec;
	asio::socket_base::keep_alive option(true);
	sock->set_option(option, ec);
	std::cout << "keepalive set ec: " << ec << " option.value() " << option.value();
	*/
	std::unique_ptr<Pings> ptr(new Pings);
	this->pingMap.insert(std::pair<std::string, std::unique_ptr<Pings>>(chn, std::move(ptr)));
	this->threads.push_back(std::thread(&IrcConnection::listenAndHandle, this, chn));
	std::cout << "started thread: " << chn << std::endl;
	
}

void IrcConnection::leaveChannel(const std::string& chn)
{
	std::string part = "PART #" + chn + "\r\n";
	this->channelSockets[chn]->send(asio::buffer(part));
	this->channelSockets[chn]->close();
	this->channelSockets.erase(chn);
	this->channelBools.erase(chn);
	this->channelMsgs.erase(chn);
	this->channelTimes.erase(chn);
	this->pingMap.erase(chn);
}

bool IrcConnection::quit()
{
	std::lock_guard<std::mutex> lck(irc_m);
	return this->quit_m;
}

void IrcConnection::stop()
{
	{
		std::lock_guard<std::mutex> lck(irc_m);
		this->wrk.reset();
		this->quit_m = true;
		for(auto &i : this->channelSockets)
		{
			this->leaveChannel(i.first);
			i.second->close();
			std::cout << "left: " << i.first << std::endl;
		}
		this->channelSockets.clear();
		this->channelBools.clear();
		this->channelMsgs.clear();
		this->channelTimes.clear();
		this->pingMap.clear();
	}
	this->quit_cv.notify_all();
	this->eventQueue.notify();
}

void handler(const asio::error_code& error,std::size_t bytes_transferred)
{
	std::cout << "sent msg: " << bytes_transferred << std::endl;
}

void IrcConnection::start(const std::string& pass, const std::string& nick)
{
	std::lock_guard<std::mutex> lck(irc_m);
	this->quit_m = false;
	this->pass = pass;
	this->nick = nick;
	
	asio::ip::tcp::resolver resolver(m_io_service);
	asio::ip::tcp::resolver::query query("irc.chat.twitch.tv", "6667");
	this->twitch_it = resolver.resolve(query);
		
	this->threads.push_back(std::thread(&IrcConnection::run, this));
	this->threads.push_back(std::thread(&IrcConnection::msgCount, this));
	this->threads.push_back(std::thread(&IrcConnection::processEventQueue, this));
	
	auto lambda = [this]()
	{
		while(!(this->quit()))
		{
			std::unique_lock<std::mutex> lock(irc_m);
			if(this->quit_cv.wait_for(lock, std::chrono::seconds(15),[this](){return this->quit_m;}))			
			{
				std::cout << "quiting pingmap" << std::endl;
				return;
			}
			else // 15 seconds passed
			{	
				lock.unlock();
				for(const auto& i : this->channelSockets)
				{
					std::string sendirc = "PING :tmi.twitch.tv\r\n";
					
					this->channelSockets[i.first]->send(asio::buffer(sendirc));
					std::cout << "pinging" << i.first << std::endl;
					std::unique_lock<std::mutex> lk(this->pingMap[i.first]->mtx);
					if(this->pingMap[i.first]->cv.wait_for(lk, std::chrono::seconds(15), [this, &i](){return this->pingMap[i.first]->pinged;}))
					{
						std::cout << "received the ping back " << i.first << std::endl;
						this->pingMap[i.first]->pinged = false;
					}
					else // didnt get back;
					{
						std::cout << "didnt receive ping back " << i.first << std::endl;
						this->leaveChannel(i.first);
						this->joinChannel(i.first);
						std::cout << "didnt receive, rejoined " << i.first << std::endl;
					}
				}
			}
		}
	};
	
	this->threads.push_back(std::thread(lambda));
	this->threads.push_back(std::thread(&IrcConnection::IncrementLoop, this));
}



void IrcConnection::run()
{
	this->wrk = std::make_unique<asio::io_service::work>(this->m_io_service);
	this->m_io_service.run();
	this->m_io_service.reset();
	std::cout << "end wrk\n";
}

void IrcConnection::msgCount()
{
	while(!(this->quit()))
	{
		std::this_thread::sleep_for(std::chrono::seconds(2));
		for(auto & i : this->channelMsgs)
		{
			if(i.second > 0)
			{
				i.second--;
			}
		}
	}
	std::cout << "end msg\n";
}

bool IrcConnection::sendMsg(const std::string& channel, const std::string& msg)
{
	if(this->channelMsgs[channel] < 19 && std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - this->channelTimes[channel]).count() > 1500)
	{
		std::cout << "Sending msg okay\n";
		std::string sendirc = "PRIVMSG #" + channel + " :" + msg; // + " \r\n";
		if(sendirc.length() > 387)
			sendirc = sendirc.substr(0, 387) + " \r\n";
		else sendirc += " \r\n";
		std::cout << "sendirc: " << sendirc << std::endl;
		this->channelSockets[channel]->async_send(asio::buffer(sendirc), handler);
		this->channelMsgs[channel]++;
		this->channelTimes[channel] = std::chrono::high_resolution_clock::now();
		return true;
	}
	else 
	{
		std::cout << "sending msg not okay\n";
		return false;
	}
}

void IrcConnection::handleCommands(std::string& user, const std::string& channel, std::string& msg)
{
	try{
	//std::cout << "HANDLING COMMAND\n";

	//tolower the user name
	changeToLower(user);
	
	if(msg.compare(0, strlen("!quit"), "!quit") == 0 && user == "hemirt")
	{
		this->stop();
		return;
	}
	
	if(msg.compare(0, strlen("!rcn"), "!rcn") == 0 && user == "hemirt")
	{
		this->leaveChannel(channel);
		this->joinChannel(channel);
		return;
	}
	/*
	if(msg.compare(0, strlen("!addchannel"), "!addchannel") == 0 && user == "hemirt")
	{
		std::string delimiter = " ";
		std::vector<std::string> vek;
		size_t pos = 0;
		while((pos = msg.find(delimiter)) != std::string::npos)
		{
			vek.push_back(msg.substr(0, pos));
			msg.erase(0, pos + delimiter.length());
		}
		vek.push_back(msg);
		if(vek.size() == 2)
		this->items.createChannelTable(vek[1]);
		return;
	}
	*/
	if(msg.compare(0, strlen("!deleteincrement"), "!deleteincrement") == 0 && user == "hemirt")
	{
		
		std::string delimiter = " ";
		std::vector<std::string> vek;
		size_t pos = 0;
		while((pos = msg.find(delimiter)) != std::string::npos)
		{
			vek.push_back(msg.substr(0, pos));
			msg.erase(0, pos + delimiter.length());
		}
		vek.push_back(msg);
		if(vek.size() == 5)
			this->itemincr.remove(stoi(vek[1]), vek[2], vek[3], stod(vek[4]));
		return;
	}
	
	if(msg.compare(0, strlen("!additemincrement"), "!additemincrement") == 0 && user == "hemirt")
	{
		std::string delimiter = " ";
		std::vector<std::string> vek;
		size_t pos = 0;
		while((pos = msg.find(delimiter)) != std::string::npos)
		{
			vek.push_back(msg.substr(0, pos));
			msg.erase(0, pos + delimiter.length());
		}
		vek.push_back(msg);
		if(vek.size() == 5)
			this->itemincr.add(stoi(vek[1]), vek[2], vek[3], stod(vek[4]));
		return;
	}
	
	if(msg.compare(0, strlen("!addnewitem"), "!addnewitem") == 0 && user == "hemirt")
	{
		std::string delimiter = " ";
		std::vector<std::string> vek;
		size_t pos = 0;
		while((pos = msg.find(delimiter)) != std::string::npos)
		{
			vek.push_back(msg.substr(0, pos));
			msg.erase(0, pos + delimiter.length());
		}
		vek.push_back(msg);
		if(vek.size() == 3)
			this->itemincr.addNewItem(vek[1], stoull(vek[2]));
		return;
	}
	
	if(msg.compare(0, strlen("!allitems"), "!allitems") == 0)
	{
		std::string msgback = user + ", the existing items are ";
		for(auto i : this->itemincr.allItems)
		{
			msgback += i.first + "s, ";
		}
		msgback.pop_back();
		msgback.pop_back();
		msgback += ".";
		this->sendMsg(channel, msgback);
		return;
	}
	
	if(msg.compare(0, strlen("!price"), "!price") == 0)
	{
		std::string delimiter = " ";
		std::vector<std::string> vek;
		size_t pos = 0;
		while((pos = msg.find(delimiter)) != std::string::npos)
		{
			vek.push_back(msg.substr(0, pos));
			msg.erase(0, pos + delimiter.length());
		}
		vek.push_back(msg);
		if(vek.size() != 2)
			return;
		auto it = this->itemincr.allItems.find(vek[1]);
		if(it != this->itemincr.allItems.end())
		{
			std::stringstream ss;
			if(it->second == 0)
			{
				ss << user << ", you can't buy " << it->first << "s.";
				this->sendMsg(channel, ss.str());
				return;
			}
			ss << user << ", " << it->first << "s cost " << it->second << " coin";
			if(it->second != 1) ss << "s";
			ss << " each.";
			this->sendMsg(channel, ss.str());
		}
		else
		{
			vek[1].pop_back();
			it = this->itemincr.allItems.find(vek[1]);
			if(it != this->itemincr.allItems.end())
			{
				std::stringstream ss;
				if(it->second == 0)
				{
					ss << user << ", you can't buy " << it->first << "s.";
					this->sendMsg(channel, ss.str());
					return;
				}
				ss << user << ", " << it->first << "s cost " << it->second << " coin";
				if(it->second != 1) ss << "s";
				ss << " each.";
				this->sendMsg(channel, ss.str());
			}
		}
		return;
	}
	
	if(msg.compare(0, strlen("!addcmd"), "!addcmd") == 0 && isAdmin(user))
	{
		std::string delimiter = "#";
		std::vector<std::string> vek;
		size_t pos = 0;
		int i = 1;
		while((pos = msg.find(delimiter)) != std::string::npos && i < 5)
		{
			vek.push_back(msg.substr(0, pos));
			msg.erase(0, pos + delimiter.length());
			i++;
		}
		vek.push_back(msg);
		
		for(auto i : vek)
		{
			std::cout << "VKX:" << i << std::endl;
		}
		
		if(vek.size() == 5)
		{
			changeToLower(vek[1]);
			changeToLower(vek[2]);
			std::string cmd = vek[2] + "#" + vek[3] + "#" + vek[4];
			std::cout << "adding: " << vek[1] << "#" << cmd << std::endl;
			this->addCmd(vek[1], cmd);
		}
		return;
	}
	
	if(msg.compare(0, strlen("!mycount"), "!mycount") == 0)
	{
		std::string delimiter = " ";
		std::vector<std::string> vek;
		size_t pos = 0;
		while((pos = msg.find(delimiter)) != std::string::npos)
		{
			vek.push_back(msg.substr(0, pos));
			msg.erase(0, pos + delimiter.length());
		}
		vek.push_back(msg);
		std::unique_lock<std::mutex> lk;
		std::pair<bool, unsigned long long> pair;
		try
		{
			lk = std::unique_lock<std::mutex>(*(this->channels.channelsItemsMap.at(channel).mtx));
			pair = this->channels.channelsItemsMap.at(channel).get(user, vek[1]);
		}
		catch(std::exception &e)
		{
			std::cout << "exception get mycount: " << e.what() << std::endl;
			return;
		}
		if(pair.first == false) 
		{
			vek[1].pop_back();
			try
			{
				pair = this->channels.channelsItemsMap.at(channel).get(user, vek[1]);
			}
			catch(std::exception &e)
			{
				std::cout << "exception get mycount: " << e.what() << std::endl;
				return;
			}
			if(pair.first == false) return;
			unsigned long long count = pair.second;
			std::stringstream ss;
			ss << user << ", you have " << count << " " << vek[1];
			if(count != 1) ss << "s";
			ss << ".";
			this->sendMsg(channel, ss.str());
			return;
		}
		unsigned long long count = pair.second;
		std::stringstream ss;
		ss << user << ", you have " << count << " " << vek[1];
		if(count != 1) ss << "s";
		ss << ".";
		this->sendMsg(channel, ss.str());
		return;
	}
	
	if(msg.compare(0, strlen("!peng"), "!peng") == 0)
	{
		std::string delimiter = " ";
		
		std::stringstream ss;
		ss << user << ", weneedmoreautisticbots is running for " <<  std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - this->currentTrigger).count()  << " nanoseconds EleGiggle";
		this->sendMsg(channel, ss.str());
		return;
	}
	
	if(msg.compare(0, strlen("!size"), "!size") == 0)
	{
		try
		{
			std::cout << channel << " size: " << this->channels.channelsItemsMap.at(channel).size() << std::endl;
		}
		catch(std::exception &e)
		{
			std::cout << "exception get size: " << e.what() << std::endl;
		}
		return;
	}
	if(msg.compare(0, strlen("!rolet "), "!rolet ") == 0)
	{
		std::vector<std::string> vek;
		while(msg.find(".") != std::string::npos)
		{
			msg.replace(msg.find("."), 1, "·");
		}
		size_t pos = 0;
		int i = 0;
		while((pos = msg.find(" ")) != std::string::npos && i < 2)
		{
			vek.push_back(msg.substr(0, pos));
			msg.erase(0, pos + 1);
			i++;
		}
		vek.push_back(msg);
		if(vek.size() >= 2)
		{
			std::string msgback;
			if(vek.at(1).find("!untuck") != std::string::npos)
			{
				msgback = user + " one does not simply untuck LUL";
			}
			else
			{
				while(true)
				{
					int rolet = this->dice();
					if(rolet > 50)
					{
						msgback = user + " won " + vek.at(1) + " in rolet and is now rich as fuck! FeelsAmazingMan";
						break;
					}
					else if(rolet < 50)
					{
						msgback = user + " lost " + vek.at(1) + " in rolet and now has nothing left! FeelsBadMan";
						break;
					}
					else continue;
				}
			}
			this->sendMsg(channel, msgback);
			return;
				
		}
		return;
		
	}
	while(msg.find(".") != std::string::npos)
	{
		msg.replace(msg.find("."), 1, "·");
	}
	msg.erase(0, 1);
	while(msg.find("!") != std::string::npos)
	{
		msg.replace(msg.find("!"), 1, "\u00A1");
	}
	while(msg.find("/") != std::string::npos)
	{
		msg.replace(msg.find("/"), 1, "\\");
	}
	std::string msgcopy = msg;
	std::string delimiter = " ";
	std::vector<std::string> vekmsg;
	size_t pos = 0;
	while((pos = msgcopy.find(delimiter)) != std::string::npos)
	{
		vekmsg.push_back(msgcopy.substr(0, pos));
		msgcopy.erase(0, pos + delimiter.length());
	}
	vekmsg.push_back(msgcopy);
	
	//to lower the !cmd
	changeToLower(vekmsg.at(0));
	std::pair<std::multimap<std::string, Command>::const_iterator, std::multimap<std::string, Command>::const_iterator> ret = this->commands.equal_range("!" + vekmsg.at(0));
	
	if(vekmsg.at(0) == "asay" && user == "hemirt")
	{
		while(msg.find("\u00A1") != std::string::npos)
		{
			msg.replace(msg.find("\u00A1"), strlen("\u00A1"), "!");
		}
		while(msg.find("\\") != std::string::npos)
		{
			msg.replace(msg.find("\\"), 1, "/");
		}
	}
	
	if(ret.first == ret.second) 
	{
		return; //cmd not found
	}
	--ret.first;
	--ret.second;
	std::multimap<std::string, Command>::const_iterator it;
	bool found = false;
	std::string users[3] = {user, "admin", "all"};
	int i = 0;
	while(!found && i < 3)
	{
		//std::cout << "ding: " << i << std::endl;
		it = ret.second;
		while(it != ret.first)
		{
			//std::cout << "dong: " << i << std::endl;
			//std::cout << "it " << it->first << it->second.who << std::endl;
			if(it->second.who == users[i])
			{
				if(i == 1 && this->isAdmin(user) == false)
				{
					--it;
					continue; //found admin cmd, but user isnt an admin
				}
				found = true; //found command
				//std::cout << "found\n";
				break;
			}
			--it;
		}
		if(!found) ++i;
	}
	std::cout << "found" << std::endl;
	if(found)
	{
		std::string msgback = it->second.data;
		while(msgback.find("@me@") != std::string::npos)
		{
			msgback.replace(msgback.find("@me@"), 4, user);
		}
		int i = 1;
		std::stringstream ss;
		ss << "@id" << i << "@";
		while(msgback.find(ss.str()) != std::string::npos && i < vekmsg.size())
		{
			msgback.replace(msgback.find(ss.str()), ss.str().size(), vekmsg[i]);
			i++;
			ss.str("");
			ss << "@id" << i << "@";
		}
		std::string all = msg.substr(it->first.size() - 1, std::string::npos);
		while(msgback.find("@all@") != std::string::npos)
		{
			msgback.replace(msgback.find("@all@"), 5, all);
		}
		
		int rnd = this->dice();
		while(msgback.find("@irnd@") != std::string::npos)
		{
			msgback.replace(msgback.find("@irnd@"), 6, std::to_string(rnd));
		}
		
		size_t pos = 0;
		while((pos = msgback.find("@ifrnd@")) != std::string::npos)
		{
			msgback.erase(pos, 7);
			//std::cout << "msgback: " << "\"" << msgback << "\"" << std::endl;
			//std::cout << "pos: " << msgback.at(pos) << std::endl;
			if(msgback.at(pos) == '<')
			{
				msgback.erase(pos, 1);
				size_t hash = msgback.find('#', pos);
				std::string number = msgback.substr(pos, hash-pos);
				int num = std::atoi(number.c_str());
				//std::cout << "substr: " << msgback.substr(pos, hash-pos) << std::endl;
				msgback.erase(pos, hash-pos);
				if(rnd < num)
				{
					msgback.erase(pos, 1);
					size_t end = msgback.find('#', pos);
					msgback.erase(end, 1);
				}
				else
				{
					msgback.erase(pos, 1);
					size_t end = msgback.find('#', pos);
					msgback.erase(pos, end-pos + 1);
				}
			}
			else if(msgback.at(pos) == '>')
			{
				msgback.erase(pos, 1);
				size_t hash = msgback.find('#', pos);
				std::string number = msgback.substr(pos, hash-pos);
				int num = std::atoi(number.c_str());
				std::cout << "substr: " << msgback.substr(pos, hash-pos) << std::endl;
				msgback.erase(pos, hash-pos);
				if(rnd > num)
				{
					msgback.erase(pos, 1);
					size_t end = msgback.find('#', pos);
					msgback.erase(end, 1);
				}
				else
				{
					msgback.erase(pos, 1);
					size_t end = msgback.find('#', pos);
					msgback.erase(pos, end-pos + 1);
				}
			}
			if(msgback.at(pos) == '=')
			{
				msgback.erase(pos, 1);
				size_t hash = msgback.find('#', pos);
				std::string number = msgback.substr(pos, hash-pos);
				int num = std::atoi(number.c_str());
				std::cout << "substr: " << msgback.substr(pos, hash-pos) << std::endl;
				msgback.erase(pos, hash-pos);
				if(rnd == num)
				{
					msgback.erase(pos, 1);
					size_t end = msgback.find('#', pos);
					msgback.erase(end, 1);
				}
				else
				{
					msgback.erase(pos, 1);
					size_t end = msgback.find('#', pos);
					msgback.erase(pos, end-pos + 1);
				}
			}
		}
		
		while(msgback.find("@time@") != std::string::npos)
		{
			msgback.replace(msgback.find("@time@"), 6, timenow());
		}
		std::cout << "aftersend" << std::endl;
		if(it->second.who == "all")
		{
			if(it->second.action == "say")
			{
				this->sendMsg(channel, msgback);
				return;
			}
		}
		else if(it->second.who == "admin")
		{
			if(this->isAdmin(user))
			{
				if(it->second.action == "say")
				{
					this->sendMsg(channel, msgback);
					return;
				}
				else if (it->second.action == "repeat" && user == "hemirt")
				{
					while(msgback.find("\u00A1") != std::string::npos)
					{
						msgback.replace(msgback.find("\u00A1"), strlen("\u00A1"), "!");
					}
					std::cout << "msgb: " << msgback << std::endl;
					std::string delimiter = " ";
					msgback.erase(0, 1);
					size_t pos = 0;
					int rp = 0;
					if((pos = msgback.find(delimiter)) != std::string::npos)
					{
						rp = std::atoi(msgback.substr(0, pos).c_str());
						msgback.erase(0, pos + delimiter.length());
					}
					for(int y = 0; y < rp; y++)
					{
						this->sendMsg(channel, msgback);
						std::this_thread::sleep_for(std::chrono::milliseconds(1501));
					}
				}
			}
		}
		else if(it->second.who == user)
		{
			if(it->second.action == "say")
			{
				this->sendMsg(channel, msgback);
				return;
			}
		}
	}
	}catch(std::exception& e)
	{
		std::cout << msg << std::endl;
		std::cout << e.what() << std::endl;
	}
}

void IrcConnection::processEventQueue()
{
	while(!(this->quit()))
	{
		this->eventQueue.wait();
		while(!(this->eventQueue.empty()) && !(this->quit()))
		{
			//std::cout << "event" << std::endl;
			auto pair = this->eventQueue.pop();
			std::unique_ptr<asio::streambuf> b(std::move(pair.first));
			std::string chn = pair.second;
			
			std::istream is(&(*b));
			std::string line(std::istreambuf_iterator<char>(is), {});
			
			std::string delimiter = "\r\n";
			std::vector<std::string> vek;
			size_t pos = 0;
			while((pos = line.find(delimiter)) != std::string::npos)
			{
				vek.push_back(line.substr(0, pos));
				line.erase(0, pos + delimiter.length());
			}
			
			for(int i = 0; i < vek.size(); ++i)
			{
				
				std::string oneline = vek.at(i);
			
				//std::cout << oneline <<std::endl;
				if(oneline.find("PRIVMSG") != std::string::npos)
				{
					size_t pos = oneline.find("PRIVMSG #") + strlen("PRIVMSG #");		
					std::string channel = oneline.substr(pos, oneline.find(":", oneline.find("PRIVMSG #")) - pos - 1);
					std::string ht_chn = "#" + channel;
					std::string msg = oneline.substr(oneline.find(":", oneline.find(ht_chn)) + 1, std::string::npos);
					std::string user = oneline.substr(oneline.find(":") + 1, oneline.find("!") - oneline.find(":") - 1);

					if(msg[0] == '!')
					{
						this->handleCommands(user, channel, msg);
					}
				}
				else if(oneline.find("PING") != std::string::npos)
				{
					std::cout << "PONGING" << chn << std::endl;
					std::string pong = "PONG :tmi.twitch.tv\r\n";
					this->channelSockets[chn]->async_send(asio::buffer(pong), handler);
				}
				else if(oneline.find("PONG") != std::string::npos)
				{
					{
						std::cout << "got ping " << chn << std::endl;
						std::lock_guard<std::mutex> lk(this->pingMap[chn]->mtx);
						this->pingMap[chn]->pinged = true;
					}
					this->pingMap[chn]->cv.notify_all();
				}
			}			
		}
	}
}

void IrcConnection::listenAndHandle(const std::string& chn)
{
	try
		{
			while(!(this->quit()))
			{
				std::unique_ptr<asio::streambuf> b(new asio::streambuf);
				asio::error_code ec;
				asio::read_until(*(this->channelSockets[chn]), *b, "\r\n", ec);				
				if(!(ec && !(this->quit())))
				{
					this->eventQueue.push(std::pair<std::unique_ptr<asio::streambuf>, std::string>(std::move(b), chn));
				}
				else
				{
					std::cout << "error: " << chn << " ecx: " << ec << std::endl;
					std::cout << "err leaving " << chn << std::endl;
					this->leaveChannel(chn);
					std::cout << "err reconnecting " << chn << std::endl;
					this->joinChannel(chn);
					std::cout << "err reconnected " << chn << std::endl;
					return;
				}
			}
		}
	catch (std::exception& e)
	{
		std::cout << "exc : " << chn << " " << e.what() << std::endl;
		if(!(this->quit()))
		{
			this->leaveChannel(chn);
			this->joinChannel(chn);
			return;
		}
	}
}

int main(int argc, char *argv[])
{
	IrcConnection myIrc;
	myIrc.start(argv[1], argv[2]);
	
	myIrc.joinChannel("pajlada");
	myIrc.joinChannel("hemirt");
	myIrc.joinChannel("forsenlol");
	
	myIrc.channels.addChannel("hemirt");
	myIrc.channels.addChannel("forsenlol");
	myIrc.channels.addChannel("pajlada");
	
	myIrc.waitEnd();
	return 0;
}