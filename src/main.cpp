#include "connectionhandler.hpp"

#include <curl/curl.h>
#include <csignal>
#include <iostream>

ConnectionHandler *irc = nullptr;

extern "C" void
signalHandler(int signum)
{
    std::cout << "Interrupted " << signum << std::endl;
    irc->shutdown();
}

int
main(int argc, char *argv[])
{
    CURLcode res = curl_global_init(CURL_GLOBAL_ALL);
    if (res) {
        std::cout << "Curl error: " << res << std::endl;
        return 1;
    }
    std::signal(SIGINT, signalHandler);
    // Make sure the user is passing through the required arguments
    
    do {
        if (irc != nullptr) {
            delete irc;
        }
        if (argc < 3) {
            irc = new ConnectionHandler();
        } else {
            irc = new ConnectionHandler(argv[1], argv[2]);
        }
        
        irc->run();
        
    } while (irc->error() && !irc->stop());
    
    if (irc != nullptr) {
        delete irc;
    }

    curl_global_cleanup();
    std::cout << "Exiting" << std::endl;
    return 0;
}
