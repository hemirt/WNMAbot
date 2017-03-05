# WNMAbot

# installation instructions
<h3> Linux </h3>
1. clone this repo `git clone https://github.com/hemirt/WNMAbot.git`
2. cd into the repo directory `cd WNMAbot`
3. Create the build directory `mkdir build`
4. cd into the build directory `cd build`
5. Run cmake `cmake ..`
6. Compile the bot `make`
7. Start the bot with `./WNMAbot oauth username`

# prerequisites
1. redis
2. hiredis - via your distro or https://github.com/redis/hiredis
3. c++17 compiler support
4. libcurl
