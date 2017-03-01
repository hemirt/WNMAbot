#include "connectionhandler.hpp"

#include <iostream>
#include <csignal>

ConnectionHandler *irc;

void signalHandler(int signum) {
    std::cout << "Interrupted " << signum << std::endl;
    irc->shutdown();
}

int
main(int argc, char *argv[])
{
    std::signal(SIGINT, signalHandler);
    // Make sure the user is passing through the required arguments
    if (argc < 3) {
        irc = new ConnectionHandler();
    } else {
        irc = new ConnectionHandler(argv[1], argv[2]);
    }

    irc->run();
    delete irc;
    
    std::cout << "Exiting" << std::endl;
    return 0;
}
