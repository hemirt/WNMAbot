#include "connectionhandler.hpp"

#include <iostream>

int
main(int argc, char *argv[])
{
    // Make sure the user is passing through the required arguments
    if (argc < 3) {
        std::cerr << "usage: ./WNMAbot oauth username" << std::endl;
        return 1;
    }

    ConnectionHandler irc(argv[1], argv[2]);

    irc.joinChannel("pajlada");
    irc.joinChannel("hemirt");
    irc.joinChannel("forsenlol");

    std::cout << "added all" << std::endl;
    irc.run();
    std::cout << "ended running" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(2));
    std::cout << "zulul" << std::endl;

    return 0;
}
