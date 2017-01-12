#include "connectionhandler.hpp"

#include <iostream>

int
main(int argc, char *argv[])
{
    ConnectionHandler *irc;

    // Make sure the user is passing through the required arguments
    if (argc < 3) {
        irc = new ConnectionHandler();
    } else {
        irc = new ConnectionHandler(argv[1], argv[2]);
    }

    irc->joinChannel("pajlada");
    irc->joinChannel("hemirt");
    irc->joinChannel("forsenlol");

    std::cout << "added all" << std::endl;
    irc->run();

    std::cout << "ended running" << std::endl;
    delete irc;

    return 0;
}
