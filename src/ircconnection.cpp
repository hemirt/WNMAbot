#include "ircconnection.hpp"

void
handler(const asio::error_code &error, std::size_t bytes_transferred)
{
    std::cout << "sent msg: " << bytes_transferred << std::endl;
}

IrcConnection::IrcConnection()
{
    this->adminsFile.open("admins.txt", std::ios::in);
    if (this->adminsFile.is_open() == false) {
        this->adminsFile.open("admins.txt", std::ios::out);
        this->adminsFile << "hemirt" << std::endl;
        this->adminsFile.close();
        this->adminsFile.open("admins.txt", std::ios::in);
    }

    {
        std::string line;
        while (std::getline(this->adminsFile, line)) {
            this->admins.insert(line);
        }
        this->adminsFile.close();
        this->adminsFile.open("admins.txt", std::ios::out | std::ios::app);
    }

    this->commandsFile.open("commands.txt", std::ios::in);
    if (this->commandsFile.is_open() == false) {
        this->commandsFile.open("commands.txt", std::ios::out);
        this->commandsFile << "!addcmd admins Done" << std::endl;
        this->commandsFile.close();
        this->commandsFile.open("commands.txt", std::ios::in);
    }
    {
        std::string line;
        while (std::getline(this->commandsFile, line)) {
            std::string delimiter = "#";
            std::vector<std::string> vek;
            size_t pos = 0;
            int i = 1;
            while ((pos = line.find(delimiter)) != std::string::npos && i < 4) {
                vek.push_back(line.substr(0, pos));
                line.erase(0, pos + delimiter.length());
                i++;
            }
            vek.push_back(line);
            if (vek.size() == 4) {
                changeToLower(vek[0]);
                changeToLower(vek[1]);
                Command cmd = {vek[1], vek[2], vek[3]};
                this->commands.insert(
                    std::pair<std::string, Command>(vek[0], cmd));
            }
        }
        this->commandsFile.close();
        this->commandsFile.open("commands.txt", std::ios::out | std::ios::app);
    }

    blacklist.open("blacklist.txt", std::ios::in);
    std::string line;
    while (std::getline(blacklist, line)) {
        blacklistset.insert(line);
    }
    blacklist.close();
    blacklist.open("blacklist.txt", std::ios::out | std::ios::app);

    std::cout << "lulopxened\n";
}

IrcConnection::~IrcConnection()
{
    this->adminsFile.close();
    this->commandsFile.close();
    blacklist.close();
}

void
IrcConnection::processEventQueue()
{
    while (!(this->quit())) {
        this->eventQueue.wait();
        while (!(this->eventQueue.empty()) && !(this->quit())) {
            // std::cout << "event" << std::endl;
            auto pair = this->eventQueue.pop();
            std::unique_ptr<asio::streambuf> b(std::move(pair.first));
            std::string chn = pair.second;

            std::istream is(&(*b));
            std::string line(std::istreambuf_iterator<char>(is), {});

            std::string delimiter = "\r\n";
            std::vector<std::string> vek;
            size_t pos = 0;
            while ((pos = line.find(delimiter)) != std::string::npos) {
                vek.push_back(line.substr(0, pos));
                line.erase(0, pos + delimiter.length());
            }

            for (int i = 0; i < vek.size(); ++i) {
                std::string oneline = vek.at(i);

                // std::cout << oneline <<std::endl;
                if (oneline.find("PRIVMSG") != std::string::npos) {
                    size_t pos =
                        oneline.find("PRIVMSG #") + strlen("PRIVMSG #");
                    std::string channel = oneline.substr(
                        pos,
                        oneline.find(":", oneline.find("PRIVMSG #")) - pos - 1);
                    std::string ht_chn = "#" + channel;
                    std::string msg = oneline.substr(
                        oneline.find(":", oneline.find(ht_chn)) + 1,
                        std::string::npos);
                    std::string user = oneline.substr(
                        oneline.find(":") + 1,
                        oneline.find("!") - oneline.find(":") - 1);

                    // if(msg[0] == '!')
                    {
                        this->handleCommands(user, channel, msg);
                    }
                } else if (oneline.find("PING") != std::string::npos) {
                    if (this->channelSockets.count(chn) == 1) {
                        std::cout << "PONGING" << chn << std::endl;
                        std::string pong = "PONG :tmi.twitch.tv\r\n";
                        this->channelSockets[chn]->async_send(
                            asio::buffer(pong), handler);
                    }
                } else if (oneline.find("PONG") != std::string::npos) {
                    {
                        std::cout << timenow() << " got ping " << chn
                                  << std::endl;
                        std::lock_guard<std::mutex> lk(this->pingMap[chn]->mtx);
                        this->pingMap[chn]->pinged = true;
                    }
                    this->pingMap[chn]->cv.notify_all();
                }
            }
        }
    }
}

void
IrcConnection::listenAndHandle(const std::string &chn)
{
    try {
        while (!(this->quit())) {
            std::unique_ptr<asio::streambuf> b(new asio::streambuf);
            asio::error_code ec;
            asio::read_until(*(this->channelSockets[chn]), *b, "\r\n", ec);
            if (!(ec && !(this->quit()))) {
                this->eventQueue.push(
                    std::pair<std::unique_ptr<asio::streambuf>, std::string>(
                        std::move(b), chn));
            } else {
                std::cout << "listenandhandle " << chn << " error: " << ec
                          << std::endl
                          << ec.message() << std::endl;
                if (ec == asio::error::eof)
                    continue;
                std::cout << "after listen continue" << std::endl;
                std::lock_guard<std::mutex> lock(irc_m);
                this->channelSockets.erase(chn);
                this->channelMsgs.erase(chn);
                this->channelTimes.erase(chn);
                this->pingMap.erase(chn);
                if (this->channelBools.count(chn) == 1) {
                    this->joinChannel(chn);
                }
                return;
            }
        }
    } catch (std::exception &e) {
        std::cout << "exc : " << chn << " " << e.what() << std::endl;
        if (!(this->quit())) {
            this->channelSockets.erase(chn);
            this->channelMsgs.erase(chn);
            this->channelTimes.erase(chn);
            this->pingMap.erase(chn);
            if (this->channelBools.count(chn) == 1) {
                this->joinChannel(chn);
            }
            return;
        }
    }
}

bool
IrcConnection::isAdmin(const std::string &user)
{
    if (this->admins.count(user) == 1)
        return true;
    else
        return false;
}

void
IrcConnection::addAdmin(const std::string &admin)
{
    if (this->admins.count(admin) == 0) {
        this->admins.insert(admin);
        this->adminsFile.seekp(0, std::ios::end);
        this->adminsFile << admin << std::endl;
    }
}

void
IrcConnection::addCmd(const std::string &cmd, std::string message)
{
    std::pair<std::multimap<std::string, Command>::const_iterator,
              std::multimap<std::string, Command>::const_iterator>
        ret = this->commands.equal_range(cmd);

    Command cmds;

    std::string delimiter = "#";
    std::vector<std::string> vek;
    size_t pos = 0;
    int i = 1;
    while ((pos = message.find(delimiter)) != std::string::npos && i < 3) {
        vek.push_back(message.substr(0, pos));
        message.erase(0, pos + delimiter.length());
        i++;
    }
    vek.push_back(message);
    if (vek.size() == 3) {
        cmds = {vek[0], vek[1], vek[2]};

        for (auto &itr = ret.first; itr != ret.second; ++itr) {
            if (itr->second.who == cmds.who) {
                return;
            }
        }
        changeToLower(cmds.who);
        std::cout << "\n1: " << cmds.who << "\n2: " << cmds.action
                  << "\n3: " << cmds.data << "\n4: " << cmd << std::endl;
        this->commands.insert(std::pair<std::string, Command>(cmd, cmds));
        writeCommandsToFile();
    }
}

int
IrcConnection::deleteCmd(const std::string &cmd, const std::string &who)
{
    std::pair<std::multimap<std::string, Command>::const_iterator,
              std::multimap<std::string, Command>::const_iterator>
        ret = this->commands.equal_range(cmd);
    if (ret.first == ret.second) {
        std::cout << "wutwut " << ret.first->first << std::endl;
        std::cout << "cmd " << cmd << " who " << who << " not found"
                  << std::endl;
        return 1;  // cmd not found
    }
    if (who == "0") {
        this->commands.erase(ret.first, ret.second);
        std::cout << "deleted commands with trigger " << cmd << std::endl;
        return writeCommandsToFile();
    }
    auto it = ret.first;
    while (it != ret.second) {
        if (it->second.who == who) {
            std::cout << "deleting " << it->first << " for " << it->second.who
                      << std::endl;
            it = this->commands.erase(it);
        } else
            ++it;
    }
    return writeCommandsToFile();
}

int
IrcConnection::writeCommandsToFile()
{
    std::fstream newCommandsFile;
    newCommandsFile.open("newCommandsFile.txt",
                         std::ios::out | std::ios::trunc);
    if (!newCommandsFile)
        return 1;
    for (const auto &i : this->commands) {
        newCommandsFile << i.first << "#" << i.second.who << "#"
                        << i.second.action << "#" << i.second.data << std::endl;
    }
    if (!newCommandsFile) {
        return 1;
    } else {
        std::cout << "newCommandsFile okay" << std::endl;
        int rc = std::rename("newCommandsFile.txt", "commands.txt");
        if (rc) {
            std::perror("Error renaming");
            return 1;
        }
        std::cout << "newCommandsFile rename okay" << std::endl;
    }
    return 0;
}

void
IrcConnection::joinChannel(const std::string &chn)
{
    try {
        this->channelBools.insert({chn, true});
        if (this->channelSockets.count(chn) == 1) {
            this->leaveChannel(chn);
            // this->channelBools.erase(chn); //should already be deleted tho,
            // idk LUL
        }
        std::shared_ptr<asio::ip::tcp::socket> sock(
            new asio::ip::tcp::socket(this->m_io_service));
        this->channelSockets.insert(
            std::pair<std::string, std::shared_ptr<asio::ip::tcp::socket>>(
                chn, sock));
        asio::connect(*sock, this->twitch_it);

        std::string passx = "PASS " + this->pass + "\r\n";
        std::string nickx = "NICK " + this->nick + "\r\n";
        std::string cmds = "CAP REQ :twitch.tv/commands\r\n";

        sock->send(asio::buffer(passx));
        sock->send(asio::buffer(nickx));
        sock->send(asio::buffer(cmds));

        std::string join = "JOIN #" + chn + "\r\n";
        sock->send(asio::buffer(join));
        this->channelTimes.insert(
            std::pair<std::string,
                      std::chrono::high_resolution_clock::time_point>(
                chn, std::chrono::high_resolution_clock::now()));
        this->channelMsgs.insert(std::pair<std::string, unsigned>(chn, 0));
        std::cout << "joined" << chn << std::endl;

        /*
        asio::error_code ec;
        asio::socket_base::keep_alive option(true);
        sock->set_option(option, ec);
        std::cout << "keepalive set ec: " << ec << " option.value() " <<
        option.value();
        */
        std::unique_ptr<Pings> ptr(new Pings);
        this->pingMap.insert(std::pair<std::string, std::unique_ptr<Pings>>(
            chn, std::move(ptr)));
        this->threads.push_back(
            std::thread(&IrcConnection::listenAndHandle, this, chn));
        std::cout << "started thread: " << chn << std::endl;
    } catch (std::exception &e) {
        std::cout << "joinchn " << chn << " exc: " << e.what() << std::endl;
    }
}

void
IrcConnection::leaveChannel(const std::string &chn)
{
    try {
        if (this->channelSockets.count(chn) == 0) {
            std::cout << "leavechannel " << chn << " count = 0" << std::endl;
            return;
        }

        if (this->channelSockets[chn]->is_open()) {
            std::string part = "PART #" + chn + "\r\n";
            std::cout << "isopen: " << chn << std::endl;
            this->channelSockets[chn]->send(asio::buffer(part));
            std::cout << "sent part cmd" << std::endl;
            asio::error_code ec;
            this->channelSockets[chn]->shutdown(
                asio::ip::tcp::socket::shutdown_both, ec);
            std::cout << "shutdown" << std::endl;
            if (ec) {
                std::cout << "error: " << ec << std::endl;
            }
            this->channelSockets.at(chn)->close(ec);
            std::cout << "close" << std::endl;
            if (ec) {
                std::cout << "error: " << ec << std::endl;
            }
        }
    } catch (std::exception &e) {
        std::cout << "leavechannel exc: " << e.what();
    }

    // std::cout << "closedd" <<std::endl;
    // std::this_thread::sleep_for(std::chrono::seconds(5));
    // std::cout << "slept" <<std::endl;

    this->channelSockets.erase(chn);
    /*
    this->channelSockets.erase(chn);
    this->channelMsgs.erase(chn);
    this->channelTimes.erase(chn);
    this->pingMap.erase(chn);
    */
}

bool
IrcConnection::quit()
{
    std::lock_guard<std::mutex> lck(irc_m);
    return this->quit_m;
}

void
IrcConnection::stop()
{
    {
        std::lock_guard<std::mutex> lck(irc_m);
        this->wrk.reset();
        this->quit_m = true;
        for (auto &i : this->channelSockets) {
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
    this->eventQueue.notify();
    this->quit_cv.notify_all();
}

bool
IrcConnection::start(const std::string &pass, const std::string &nick)
{
    try {
        std::lock_guard<std::mutex> lck(irc_m);
        this->quit_m = false;
        this->pass = pass;
        this->nick = nick;

        asio::ip::tcp::resolver resolver(m_io_service);
        asio::ip::tcp::resolver::query query("irc.chat.twitch.tv", "6667");
        this->twitch_it = resolver.resolve(query);

        this->threads.push_back(std::thread(&IrcConnection::run, this));
        this->threads.push_back(std::thread(&IrcConnection::msgCount, this));
        this->threads.push_back(
            std::thread(&IrcConnection::processEventQueue, this));

        auto lambda = [this]() {
            while (!(this->quit())) {
                std::unique_lock<std::mutex> lock(irc_m);
                if (this->quit_cv.wait_for(lock, std::chrono::seconds(15),
                                           [this]() { return this->quit_m; })) {
                    std::cout << "quiting pingmap" << std::endl;
                    return;
                } else  // 15 seconds passed
                {
                    lock.unlock();
                    std::cout << "lambda pingmap: \n";
                    for (const auto &i : this->channelSockets) {
                        std::cout << "lambda: " << i.first << std::endl;
                        try {
                            std::string sendirc = "PING :tmi.twitch.tv\r\n";

                            this->channelSockets[i.first]->send(
                                asio::buffer(sendirc));
                            std::cout << "pinging" << i.first << std::endl;
                            if (this->pingMap.count(i.first) == 0)
                                continue;
                            std::unique_lock<std::mutex> lk(
                                this->pingMap.at(i.first)->mtx);
                            std::cout << "found mtx" << std::endl;
                            if (this->pingMap[i.first]->cv.wait_for(
                                    lk, std::chrono::seconds(3), [this, &i]() {
                                        return this->pingMap[i.first]->pinged;
                                    })) {
                                std::cout << "received the ping back "
                                          << i.first << std::endl;
                                this->pingMap[i.first]->pinged = false;
                            } else  // didnt get back;
                            {
                                try {
                                    std::cout << "didnt receive ping back "
                                              << i.first << std::endl;
                                    this->leaveChannel(i.first);
                                } catch (std::exception &e) {
                                    std::cout << "ping leave channel exc "
                                              << i.first << " e: " << e.what()
                                              << std::endl;
                                }
                                try {
                                    std::cout << "pinging: left channel << "
                                              << i.first << std::endl;
                                    this->joinChannel(i.first);  // idk what im
                                                                 // doing here
                                                                 // 4HEad
                                } catch (std::exception &e) {
                                    std::cout << "ping leave channel exc "
                                              << i.first << " e: " << e.what()
                                              << std::endl;
                                }
                                std::cout << "didnt receive, rejoined "
                                          << i.first << std::endl;
                            }
                        } catch (std::exception &e) {
                            std::cout
                                << "caught exception at pinging: " << i.first
                                << " " << e.what() << std::endl;
                            try {
                                this->leaveChannel(i.first);
                            } catch (...) {
                            }
                            try {
                                this->joinChannel(i.first);
                            } catch (...) {
                            }
                        }
                    }
                    std::cout << "lambda pingmap end" << std::endl;
                }
            }
        };

        this->threads.push_back(std::thread(lambda));
        // this->threads.push_back(std::thread(&IrcConnection::IncrementLoop,
        // this));
        return true;
    } catch (std::exception &e) {
        std::cout << "start exception: " << e.what() << std::endl;
        return false;
    }
}

void
IrcConnection::run()
{
    this->wrk = std::make_unique<asio::io_service::work>(this->m_io_service);
    this->m_io_service.run();
    this->m_io_service.reset();
    std::cout << "end wrk\n";
}

void
IrcConnection::msgCount()
{
    while (!(this->quit())) {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        for (auto &i : this->channelMsgs) {
            if (i.second > 0) {
                i.second--;
            }
        }
    }
    std::cout << "end msg\n";
}

bool
IrcConnection::sendMsg(const std::string &channel, const std::string &msg)
{
    std::lock_guard<std::mutex> lk(irc_m);
    if (this->channelSockets.count(channel) == 0)
        return false;
    if (this->channelMsgs[channel] < 19 &&
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now() -
            this->channelTimes[channel])
                .count() > 1500) {
        std::cout << "Sending msg okay\n";
        std::string sendirc = "PRIVMSG #" + channel + " :";
        sendirc += msg + ' ';
        if (sendirc.length() > 387)
            sendirc = sendirc.substr(0, 387) + " \r\n";
        else
            sendirc += " \r\n";
        std::cout << "sendirc: " << sendirc << std::endl;
        this->channelSockets[channel]->async_send(asio::buffer(sendirc),
                                                  handler);
        this->channelMsgs[channel]++;
        this->channelTimes[channel] = std::chrono::high_resolution_clock::now();
        return true;
    } else {
        std::cout << "sending msg not okay\n";
        return false;
    }
}
/*
void IrcConnection::handleCommands2(std::string& user, const std::string&
channel, std::string& msg)
{ try {
        changeToLower(user);
        std::vector<std::string> msgvec = splitMsg(msg);




} catch(std::exception &e)
{
        std::cout << "handleCommands2 caught exception " << e.what() <<
std::endl;
} catch(...)
{
        std::cout << "handleCommands2 caught weird exception" << std::endl;
}

}*/

void
IrcConnection::handleCommands(std::string &user, const std::string &channel,
                              std::string &msg)
{
    try {
        changeToLower(user);
        if (msg.compare(0, strlen("!blacklist"), "!blacklist") == 0 &&
            user == "hemirt") {
            std::string delimiter = " ";
            std::vector<std::string> vek;
            size_t pos = 0;
            while ((pos = msg.find(delimiter)) != std::string::npos) {
                vek.push_back(msg.substr(0, pos));
                msg.erase(0, pos + delimiter.length());
            }
            vek.push_back(msg);
            changeToLower(vek[1]);
            if (vek.size() == 2) {
                auto y = blacklistset.insert(vek[1]);
                if (y.second == false)
                    return;
                if (blacklist.is_open() == false) {
                    std::cout << "erroblckr" << std::endl;
                    return;
                }
                blacklist << vek[1] << std::endl;
            }
            return;
        }
        if (msg.compare(0, strlen("!addadmin"), "!addadmin") == 0 &&
            user == "hemirt") {
            std::string delimiter = " ";
            std::vector<std::string> vek;
            size_t pos = 0;
            while ((pos = msg.find(delimiter)) != std::string::npos) {
                vek.push_back(msg.substr(0, pos));
                msg.erase(0, pos + delimiter.length());
            }
            vek.push_back(msg);
            changeToLower(vek[1]);
            if (vek.size() == 2)
                this->addAdmin(vek[1]);
            return;
        }
        if (msg.compare(0, strlen("!deletecmd"), "!deletecmd") == 0 &&
            isAdmin(user)) {
            std::string delimiter = " ";
            std::vector<std::string> vek;
            size_t pos = 0;
            while ((pos = msg.find(delimiter)) != std::string::npos) {
                vek.push_back(msg.substr(0, pos));
                msg.erase(0, pos + delimiter.length());
            }
            vek.push_back(msg);
            changeToLower(vek[1]);
            if (vek.size() == 2)
                this->deleteCmd(vek[1], "all");
            else if (vek.size() == 3)
                this->deleteCmd(vek[1], vek[2]);
            std::cout << "deleted " << msg << std::endl;
            std::string mg(user + " deleted cmd " + vek[1]);
            this->sendMsg(channel, mg);
            return;
        }
        if (msg.compare(0, strlen("!editcmd"), "!editcmd") == 0 &&
            isAdmin(user)) {
            std::string delimiter = " ";
            std::vector<std::string> vek;
            size_t pos = 0;
            int i = 0;
            while ((pos = msg.find(delimiter)) != std::string::npos && i < 4) {
                vek.push_back(msg.substr(0, pos));
                msg.erase(0, pos + delimiter.length());
                ++i;
            }
            vek.push_back(msg);
            std::cout << "veksize: " << vek.size() << std::endl;
            for (auto i : vek) {
                std::cout << i << std::endl;
            }
            if (vek.size() == 5) {
                changeToLower(vek[1]);
                changeToLower(vek[2]);
                changeToLower(vek[3]);
                this->deleteCmd(vek[1], vek[2]);
                std::string cmd = vek[2] + "#" + vek[3] + "#" + vek[4];
                std::cout << "adding: " << vek[1] << "#" << cmd << std::endl;
                this->addCmd(vek[1], cmd);
            }
            std::cout << "EDITED " << vek[1] << std::endl;
            std::string mg(user + " edited cmd " + vek[1]);
            this->sendMsg(channel, mg);
            return;
        }
        // std::cout << "HANDLING COMMAND\n";

        // tolower the user name
        changeToLower(user);

        if (msg.compare(0, strlen("!quit"), "!quit") == 0 && user == "hemirt") {
            this->stop();
            return;
        }

        if (msg.compare(0, strlen("!rcn"), "!rcn") == 0 && user == "hemirt") {
            this->leaveChannel(channel);
            this->channelBools.insert({channel, true});
            return;
        }

        if (msg.compare(0, strlen("!joinchn"), "!joinchn") == 0 &&
            user == "hemirt") {
            std::string delimiter = " ";
            std::vector<std::string> vek;
            size_t pos = 0;
            while ((pos = msg.find(delimiter)) != std::string::npos) {
                vek.push_back(msg.substr(0, pos));
                msg.erase(0, pos + delimiter.length());
            }
            vek.push_back(msg);
            if (vek.size() == 2) {
                if (this->channelSockets.count(vek[1]) == 1)
                    return;
                this->joinChannel(vek[1]);
            }
            return;
        }

        if (msg.compare(0, strlen("!leavechn"), "!leavechn") == 0 &&
            user == "hemirt") {
            std::string delimiter = " ";
            std::vector<std::string> vek;
            size_t pos = 0;
            while ((pos = msg.find(delimiter)) != std::string::npos) {
                vek.push_back(msg.substr(0, pos));
                msg.erase(0, pos + delimiter.length());
            }
            vek.push_back(msg);
            if (vek.size() == 2) {
                this->channelBools.erase(vek[1]);
                this->leaveChannel(vek[1]);
            }
            return;
        }

        if (msg.compare(0, strlen("!chns"), "!chns") == 0 && user == "hemirt") {
            std::string msgback = "Connected in: ";
            for (const auto &i : this->channelSockets) {
                msgback += i.first + ", ";
            }
            msgback.pop_back();
            msgback.pop_back();
            msgback += ".";
            this->sendMsg(channel, msgback);
            return;
        }

        /*
        if(msg.compare(0, strlen("!addchannel"), "!addchannel") == 0 && user ==
        "hemirt")
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
        // if(msg.compare(0, strlen("!deleteincrement"), "!deleteincrement") ==
        // 0 && user == "hemirt")
        // {

        // std::string delimiter = " ";
        // std::vector<std::string> vek;
        // size_t pos = 0;
        // while((pos = msg.find(delimiter)) != std::string::npos)
        // {
        // vek.push_back(msg.substr(0, pos));
        // msg.erase(0, pos + delimiter.length());
        // }
        // vek.push_back(msg);
        // if(vek.size() == 5)
        // this->itemincr.remove(stoi(vek[1]), vek[2], vek[3], stod(vek[4]));
        // return;
        // }

        // if(msg.compare(0, strlen("!additemincrement"), "!additemincrement")
        // == 0 && user == "hemirt")
        // {
        // std::string delimiter = " ";
        // std::vector<std::string> vek;
        // size_t pos = 0;
        // while((pos = msg.find(delimiter)) != std::string::npos)
        // {
        // vek.push_back(msg.substr(0, pos));
        // msg.erase(0, pos + delimiter.length());
        // }
        // vek.push_back(msg);
        // if(vek.size() == 5)
        // this->itemincr.add(stoi(vek[1]), vek[2], vek[3], stod(vek[4]));
        // return;
        // }

        // if(msg.compare(0, strlen("!addnewitem"), "!addnewitem") == 0 && user
        // == "hemirt")
        // {
        // std::string delimiter = " ";
        // std::vector<std::string> vek;
        // size_t pos = 0;
        // while((pos = msg.find(delimiter)) != std::string::npos)
        // {
        // vek.push_back(msg.substr(0, pos));
        // msg.erase(0, pos + delimiter.length());
        // }
        // vek.push_back(msg);
        // if(vek.size() == 3)
        // this->itemincr.addNewItem(vek[1], stoull(vek[2]));
        // return;
        // }

        // if(msg.compare(0, strlen("!allitems"), "!allitems") == 0)
        // {
        // std::string msgback = user + ", the existing items are ";
        // for(auto i : this->itemincr.allItems)
        // {
        // msgback += i.first + "s, ";
        // }
        // msgback.pop_back();
        // msgback.pop_back();
        // msgback += ".";
        // this->sendMsg(channel, msgback);
        // return;
        // }

        // if(msg.compare(0, strlen("!abuy"), "!abuy") == 0)
        // {
        // std::string delimiter = " ";
        // std::vector<std::string> vek;
        // size_t pos = 0;
        // while((pos = msg.find(delimiter)) != std::string::npos)
        // {
        // vek.push_back(msg.substr(0, pos));
        // msg.erase(0, pos + delimiter.length());
        // }
        // vek.push_back(msg);
        // if(vek.size() < 3)
        // return;
        // if(vek[2][0] == '-')
        // {
        // std::string msgback = user + " wants to buy a negative number of " +
        // vek[1] + "s EleGiggle";
        // this->sendMsg(channel, msgback);
        // }
        // int rc = 0;
        // if(vek[1] == "all")
        // {
        // auto it = this->itemincr.allItems.find(vek[2]);
        // if(it == this->itemincr.allItems.end())
        // {
        // vek[2].pop_back();
        // it = this->itemincr.allItems.find(vek[2]);
        // if(it == this->itemincr.allItems.end()) return;
        // }
        // if(it->second == 0) return;
        // std::pair<bool, unsigned long long> pair;
        // try
        // {
        // std::lock_guard<std::mutex>
        // lk(*(this->channels.channelsItemsMap.at(channel).mtx));
        // pair = this->channels.channelsItemsMap.at(channel).get(user, "coin");
        // }
        // catch(std::exception &e)
        // {
        // std::cout << "exception get buy: " << e.what() << std::endl;
        // return;
        // }
        // unsigned long long newval;
        // if(pair.first == false)
        // {
        // return;
        // }
        // vek[1] = std::to_string(pair.second/it->second);
        // rc = this->buyItem(channel, user, vek[2], pair.second/it->second);
        // }
        // else
        // rc = this->buyItem(channel, user, vek[2], stoull(vek[1], nullptr,
        // 10));
        // if(rc == -1)
        // {
        // std::string msgback = user + ", you don't have enough coins.";
        // this->sendMsg(channel, msgback);
        // }
        // else if(rc == 1)
        // {
        // std::string msgback = user + ", you bought " + vek[1] + " " + vek[2]
        // + "s.";
        // this->sendMsg(channel, msgback);
        // }
        // else if(rc == -2)
        // {
        // std::string msgback = user + ", you cant buy " + vek[2] + "s.";
        // this->sendMsg(channel, msgback);
        // }
        // return;
        // }

        // if(msg.compare(0, strlen("!asell"), "!asell") == 0)
        // {
        // std::string delimiter = " ";
        // std::vector<std::string> vek;
        // size_t pos = 0;
        // while((pos = msg.find(delimiter)) != std::string::npos)
        // {
        // vek.push_back(msg.substr(0, pos));
        // msg.erase(0, pos + delimiter.length());
        // }
        // vek.push_back(msg);
        // if(vek.size() < 3)
        // return;
        // if(vek[2][0] == '-')
        // {
        // std::string msgback = user + " wants to sell a negative number of " +
        // vek[1] + "s EleGiggle";
        // this->sendMsg(channel, msgback);
        // }
        // int rc = 0;
        // if(vek[1] == "all")
        // {
        // std::pair<bool, unsigned long long> pair;
        // try
        // {
        // std::lock_guard<std::mutex>
        // lk(*(this->channels.channelsItemsMap.at(channel).mtx));
        // pair = this->channels.channelsItemsMap.at(channel).get(user, vek[2]);
        // }
        // catch(std::exception &e)
        // {
        // return;
        // }
        // if(pair.first == false)
        // {
        // std::string msgback = user + ", you don't have enough " + vek[2] +
        // "s.";
        // this->sendMsg(channel, msgback);
        // return;
        // }
        // vek[1] = std::to_string(pair.second);
        // rc = this->sellItem(channel, user, vek[2], pair.second);
        // }
        // else
        // rc = this->sellItem(channel, user, vek[2], stoull(vek[1], nullptr,
        // 10));
        // if(rc == -1)
        // {
        // std::string msgback = user + ", you don't have enough " + vek[2] +
        // "s.";
        // this->sendMsg(channel, msgback);
        // }
        // else if(rc == 1)
        // {
        // std::string msgback = user + ", you sold " + vek[1] + " " + vek[2] +
        // "s.";
        // this->sendMsg(channel, msgback);
        // }
        // else if(rc == -2)
        // {
        // std::string msgback = user + ", you can't sell " + vek[2] + "s.";
        // this->sendMsg(channel, msgback);
        // }
        // return;
        // }

        // if(msg.compare(0, strlen("!price"), "!price") == 0)
        // {
        // std::string delimiter = " ";
        // std::vector<std::string> vek;
        // size_t pos = 0;
        // while((pos = msg.find(delimiter)) != std::string::npos)
        // {
        // vek.push_back(msg.substr(0, pos));
        // msg.erase(0, pos + delimiter.length());
        // }
        // vek.push_back(msg);
        // if(vek.size() != 2)
        // return;
        // auto it = this->itemincr.allItems.find(vek[1]);
        // if(it != this->itemincr.allItems.end())
        // {
        // std::stringstream ss;
        // if(it->second == 0)
        // {
        // ss << user << ", you can't buy " << it->first << "s.";
        // this->sendMsg(channel, ss.str());
        // return;
        // }
        // ss << user << ", " << it->first << "s cost " << it->second << "
        // coin";
        // if(it->second != 1) ss << "s";
        // ss << " each.";
        // this->sendMsg(channel, ss.str());
        // }
        // else
        // {
        // vek[1].pop_back();
        // it = this->itemincr.allItems.find(vek[1]);
        // if(it != this->itemincr.allItems.end())
        // {
        // std::stringstream ss;
        // if(it->second == 0)
        // {
        // ss << user << ", you can't buy " << it->first << "s.";
        // this->sendMsg(channel, ss.str());
        // return;
        // }
        // ss << user << ", " << it->first << "s cost " << it->second << "
        // coin";
        // if(it->second != 1) ss << "s";
        // ss << " each.";
        // this->sendMsg(channel, ss.str());
        // }
        // }
        // return;
        // }

        if (msg.compare(0, strlen("!addcmd"), "!addcmd") == 0 &&
            isAdmin(user)) {
            std::string delimiter = "#";
            std::vector<std::string> vek;
            size_t pos = 0;
            int i = 1;
            while ((pos = msg.find(delimiter)) != std::string::npos && i < 5) {
                vek.push_back(msg.substr(0, pos));
                msg.erase(0, pos + delimiter.length());
                i++;
            }
            vek.push_back(msg);

            for (auto i : vek) {
                std::cout << "VKX:" << i << std::endl;
            }

            if (vek.size() == 5) {
                changeToLower(vek[1]);
                changeToLower(vek[2]);
                std::string cmd = vek[2] + "#" + vek[3] + "#" + vek[4];
                std::cout << "adding: " << vek[1] << "#" << cmd << std::endl;
                this->addCmd(vek[1], cmd);
            }
            return;
        }

        // if(msg.compare(0, strlen("!mycount"), "!mycount") == 0)
        // {
        // std::string delimiter = " ";
        // std::vector<std::string> vek;
        // size_t pos = 0;
        // while((pos = msg.find(delimiter)) != std::string::npos)
        // {
        // vek.push_back(msg.substr(0, pos));
        // msg.erase(0, pos + delimiter.length());
        // }
        // vek.push_back(msg);
        // std::unique_lock<std::mutex> lk;
        // std::pair<bool, unsigned long long> pair;
        // try
        // {
        // lk =
        // std::unique_lock<std::mutex>(*(this->channels.channelsItemsMap.at(channel).mtx));
        // pair = this->channels.channelsItemsMap.at(channel).get(user, vek[1]);
        // }
        // catch(std::exception &e)
        // {
        // std::cout << "exception get mycount: " << e.what() << std::endl;
        // return;
        // }
        // if(pair.first == false)
        // {
        // auto x = vek[1].back();
        // vek[1].pop_back();
        // try
        // {
        // pair = this->channels.channelsItemsMap.at(channel).get(user, vek[1]);
        // }
        // catch(std::exception &e)
        // {
        // std::cout << "exception get mycount: " << e.what() << std::endl;
        // return;
        // }
        // if(pair.first == false)
        // {
        // std::string msgback = user + ", you have 0 " + vek[1] + x + "s.";
        // this->sendMsg(channel, msgback);
        // return;
        // }
        // }
        // unsigned long long count = pair.second;
        // std::stringstream ss;
        // ss << user << ", you have " << count << " " << vek[1];
        // if(count != 1) ss << "s";
        // ss << ".";
        // this->sendMsg(channel, ss.str());
        // return;
        // }

        if (msg.compare(0, strlen("!peng"), "!peng") == 0) {
            std::string delimiter = " ";

            std::stringstream ss;
            ss << user << ", weneedmoreautisticbots is running for "
               << std::chrono::duration_cast<std::chrono::minutes>(
                      std::chrono::high_resolution_clock::now() -
                      this->currentTrigger)
                      .count()
               << " minutes PogChamp";
            this->sendMsg(channel, ss.str());
            return;
        }

        if (msg.compare(0, strlen("!rolet "), "!rolet ") == 0) {
            std::vector<std::string> vek;
            while (msg.find(".") != std::string::npos) {
                msg.replace(msg.find("."), 1, "·");
            }
            size_t pos = 0;
            int i = 0;
            while ((pos = msg.find(" ")) != std::string::npos && i < 2) {
                vek.push_back(msg.substr(0, pos));
                msg.erase(0, pos + 1);
                i++;
            }
            vek.push_back(msg);
            if (vek.size() >= 2) {
                std::string msgback;
                if (vek.at(1).find("!untuck") != std::string::npos) {
                    msgback = user + " one does not simply untuck LUL";
                } else {
                    while (true) {
                        int rolet = this->dice();
                        if (rolet > 50) {
                            msgback = user + " won " + vek.at(1) +
                                      " in rolet and is now rich as fuck! "
                                      "FeelsAmazingMan";
                            break;
                        } else if (rolet < 50) {
                            msgback = user + " lost " + vek.at(1) +
                                      " in rolet and now has nothing left! "
                                      "FeelsBadMan";
                            break;
                        } else
                            continue;
                    }
                }
                this->sendMsg(channel, msgback);
                return;
            }
            return;
        }

        //	while(msg.find(".") != std::string::npos)
        //	{
        //		msg.replace(msg.find("."), 1, "·");
        //	}
        //	msg.erase(0, 1);
        //	while(msg.find("!") != std::string::npos)
        //	{
        //		msg.replace(msg.find("!"), 1, "\u00A1");
        //	}

        std::string msgcopy = msg;

        try {
            if (msg.find("@me@") != std::string::npos)
                return;
            if (msg.find("@all@") != std::string::npos)
                return;
            if (msg.find("@irnd@") != std::string::npos)
                return;
            if (msg.find("@ifrnd@") != std::string::npos)
                return;
            if (msg.find("@time@") != std::string::npos)
                return;
            if (msg.find("@id") != std::string::npos &&
                msg.find("@", msg.find("@id") + 3) != std::string::npos)
                return;
        } catch (...) {
        }
        std::string delimiter = " ";
        std::vector<std::string> vekmsg;
        size_t pos = 0;
        while ((pos = msgcopy.find(delimiter)) != std::string::npos) {
            vekmsg.push_back(msgcopy.substr(0, pos));
            msgcopy.erase(0, pos + delimiter.length());
        }
        vekmsg.push_back(msgcopy);

        // to lower the !cmd
        changeToLower(vekmsg.at(0));
        std::pair<std::multimap<std::string, Command>::const_iterator,
                  std::multimap<std::string, Command>::const_iterator>
            ret = this->commands.equal_range(vekmsg.at(0));

        if (ret.first == ret.second) {
            return;  // cmd not found
        }
        // LUL i dont remember why i did this LUL, i think it was because i
        // wanted to iterate backwards but LUL its a multimap, its somehow
        // sorted ... i think... so i think it doesnt matter LUL
        std::multimap<std::string, Command>::const_iterator it;
        bool found = false;
        std::string users[3] = {user, "admin", "all"};
        int i = 0;
        while (!found && i < 3) {
            it = ret.first;
            while (it != ret.second) {
                // std::cout << "who: " << it->second.who <<std::endl;
                // std::cout << "user: " << user << std::endl;
                if (it->second.who == users[i]) {
                    if (i == 1 && this->isAdmin(user) == false) {
                        ++it;
                        continue;  // found admin cmd, but user isnt an admin
                    }
                    found = true;  // found command
                    break;
                }
                ++it;
            }
            if (!found)
                ++i;
        }
        /*
        it = ret.second;
        while(found == false && it != ret.first)
        {
                if(it->second.who == user)
                {
                        found = true;
                        break;
                }
                --it;
        }
        it = ret.second;
        while(found == false && this->isAdmin(user) && it != ret.first)
        {
                if(it->second.who == "admin")
                {
                        found = true;
                        break;
                }
                --it;
        }
        it = ret.second;
        while(found == false && it != ret.first)
        {
                if(it->second.who == "all")
                {
                        found = true;
                        break;
                }
                --it;
        }*/

        if (found) {
            // i should probably rework this LUL
            std::string msgback = it->second.data;
            while (msgback.find("@me@") != std::string::npos) {
                msgback.replace(msgback.find("@me@"), 4, user);
            }
            int i = 1;
            std::stringstream ss;
            ss << "@id" << i << "@";

            if (user != "hemirt") {
                for (const auto &i : blacklistset) {
                    for (int p = 0; p < vekmsg.size(); ++p) {
                        std::string s = vekmsg[p];
                        changeToLower(s);
                        size_t pos = 0;
                        while ((pos = s.find(i)) != std::string::npos) {
                            vekmsg[p].replace(pos, i.size(), "***");
                            s.replace(pos, i.size(), "***");
                        }
                    }
                }
            }

            while (msgback.find(ss.str()) != std::string::npos &&
                   i < vekmsg.size()) {
                msgback.replace(msgback.find(ss.str()), ss.str().size(),
                                vekmsg[i]);
                i++;
                ss.str("");
                ss << "@id" << i << "@";
            }

            std::string mymsg = msg.substr(it->first.size(), std::string::npos);
            std::string delimiter = " ";
            std::vector<std::string> vek;
            size_t pos = 0;
            while ((pos = mymsg.find(delimiter)) != std::string::npos) {
                vek.push_back(mymsg.substr(0, pos));
                mymsg.erase(0, pos + delimiter.length());
            }
            vek.push_back(mymsg);
            if (user != "hemirt") {
                for (const auto &i : blacklistset) {
                    for (int p = 0; p < vek.size(); ++p) {
                        std::string s = vek[p];
                        changeToLower(s);
                        size_t pos = 0;
                        while ((pos = s.find(i)) != std::string::npos) {
                            vek[p].replace(pos, i.size(), "***");
                            s.replace(pos, i.size(), "***");
                        }
                    }
                }
            }
            while (msgback.find("@all@") != std::string::npos) {
                std::string s = "";
                for (const auto &i : vek) {
                    s += i + ' ';
                }
                s.pop_back();
                msgback.replace(msgback.find("@all@"), 5, s);
            }

            int rnd = this->dice();
            while (msgback.find("@irnd@") != std::string::npos) {
                msgback.replace(msgback.find("@irnd@"), 6, std::to_string(rnd));
            }

            pos = 0;
            while ((pos = msgback.find("@ifrnd@")) != std::string::npos) {
                msgback.erase(pos, 7);
                if (msgback.at(pos) == '<') {
                    msgback.erase(pos, 1);
                    size_t hash = msgback.find('#', pos);
                    std::string number = msgback.substr(pos, hash - pos);
                    int num = std::atoi(number.c_str());
                    msgback.erase(pos, hash - pos);
                    if (rnd < num) {
                        msgback.erase(pos, 1);
                        size_t end = msgback.find('#', pos);
                        msgback.erase(end, 1);
                    } else {
                        msgback.erase(pos, 1);
                        size_t end = msgback.find('#', pos);
                        msgback.erase(pos, end - pos + 1);
                    }
                } else if (msgback.at(pos) == '>') {
                    msgback.erase(pos, 1);
                    size_t hash = msgback.find('#', pos);
                    std::string number = msgback.substr(pos, hash - pos);
                    int num = std::atoi(number.c_str());
                    std::cout << "substr: " << msgback.substr(pos, hash - pos)
                              << std::endl;
                    msgback.erase(pos, hash - pos);
                    if (rnd > num) {
                        msgback.erase(pos, 1);
                        size_t end = msgback.find('#', pos);
                        msgback.erase(end, 1);
                    } else {
                        msgback.erase(pos, 1);
                        size_t end = msgback.find('#', pos);
                        msgback.erase(pos, end - pos + 1);
                    }
                }
                if (msgback.at(pos) == '=') {
                    msgback.erase(pos, 1);
                    size_t hash = msgback.find('#', pos);
                    std::string number = msgback.substr(pos, hash - pos);
                    int num = std::atoi(number.c_str());
                    std::cout << "substr: " << msgback.substr(pos, hash - pos)
                              << std::endl;
                    msgback.erase(pos, hash - pos);
                    if (rnd == num) {
                        msgback.erase(pos, 1);
                        size_t end = msgback.find('#', pos);
                        msgback.erase(end, 1);
                    } else {
                        msgback.erase(pos, 1);
                        size_t end = msgback.find('#', pos);
                        msgback.erase(pos, end - pos + 1);
                    }
                }
            }

            while (msgback.find("@time@") != std::string::npos) {
                msgback.replace(msgback.find("@time@"), 6, timenow());
            }

            if (it->second.who == user) {
                if (it->second.action == "say") {
                    this->sendMsg(channel, msgback);
                    return;
                }
            } else if (it->second.who == "admin") {
                if (this->isAdmin(user)) {
                    if (it->second.action == "say") {
                        this->sendMsg(channel, msgback);
                        return;
                    } else if (it->second.action == "repeat" &&
                               (user == "hemirt" ||
                                user == "weneedmoreautisticbots")) {
                        std::cout << "\"" << msgback << "\"" << std::endl;
                        std::string delimiter = " ";
                        msgback.erase(0, 1);
                        size_t pos = 0;
                        int rp = 0;
                        if ((pos = msgback.find(delimiter)) !=
                            std::string::npos) {
                            rp = std::atoi(msgback.substr(0, pos).c_str());
                            msgback.erase(0, pos + delimiter.length());
                        }
                        std::cout << "rp: " << rp << std::endl;

                        auto lambda =
                            [ this, rp, channel, msgback{std::move(msgback)} ]()
                        {
                            for (int y = 0; y < rp && !(this->quit()); y++) {
                                this->sendMsg(channel, msgback);
                                std::this_thread::sleep_for(
                                    std::chrono::milliseconds(1501));
                            }
                        };
                        this->threads.push_back(std::thread(lambda));
                    }
                }
            } else if (it->second.who == "all") {
                if (it->second.action == "say") {
                    this->sendMsg(channel, msgback);
                    return;
                }
            }
        }
    } catch (std::exception &e) {
        std::cout << msg << std::endl;
        std::cout << e.what() << std::endl;
    }
}
