#include "commandshandler.hpp"
#include "ayah.hpp"
#include "bible.hpp"
#include "channel.hpp"
#include "connectionhandler.hpp"
#include "mtrandom.hpp"
#include "utilities.hpp"
#include "randomquote.hpp"
#include "hemirt.hpp"
#include "time.hpp"
#include "acccheck.hpp"
#include "trivia.hpp"

#include <stdint.h>
#include <vector>
#include <iterator>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/regex.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/optional/optional.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/regex.hpp>

namespace pt = boost::property_tree;

std::unordered_map<std::string, Response (CommandsHandler::*) (const IRCMessage &, std::vector<std::string> &)> CommandsHandler::adminCommands = {
    {"!raweditcmd", &CommandsHandler::rawEditCommand}, {"!addcmd", &CommandsHandler::addCommand},
    {"!editcmd", &CommandsHandler::editCommand}, {"!deletecmd", &CommandsHandler::deleteCommand},
    {"!deletefullcommand", &CommandsHandler::deleteFullCommand}, {"!joinchn", &CommandsHandler::joinChannel},
    {"!leavechn", &CommandsHandler::leaveChannel}, {"!asay", &CommandsHandler::say},
    {"!addblacklist", &CommandsHandler::addBlacklist},
    {"!removeblacklist", &CommandsHandler::removeBlacklist}, {"!setfrom", &CommandsHandler::setUserCountryFrom},
    {"!setlive", &CommandsHandler::setUserCountryLive}, {"!deleteuser", &CommandsHandler::deleteUser},
    {"!createcountry", &CommandsHandler::createCountry}, {"!deletecountry", &CommandsHandler::deleteCountry},
    {"!renamecountry", &CommandsHandler::renameCountry}, {"!getcountryid", &CommandsHandler::getCountryID},
    {"!createalias", &CommandsHandler::createAlias}, {"!deletealias", &CommandsHandler::deleteAlias},
    {"!clearqueue", &CommandsHandler::clearQueue}, {"!createmodule", &CommandsHandler::createModule},
    {"!moduletype", &CommandsHandler::setModuleType}, {"!modulesubtype", &CommandsHandler::setModuleSubtype},
    {"!modulename", &CommandsHandler::setModuleName}, {"!modulestatus", &CommandsHandler::setModuleStatus},
    {"!moduleformat", &CommandsHandler::setModuleFormat}, {"!ignore", &CommandsHandler::ignore},
    {"!unignore", &CommandsHandler::unignore}, {"!savemodule", &CommandsHandler::saveModule},
    {"!asetdata", &CommandsHandler::setDataAdmin}, {"!aset", &CommandsHandler::setDataAdmin},
    {"!deletemodule", &CommandsHandler::deleteModule}, {"!adel", &CommandsHandler::deleteUserData}, 
    {"!adeletedata", &CommandsHandler::deleteUserData}, {"!whoisafk", &CommandsHandler::whoIsAfk},
    {"!rcall", &CommandsHandler::reconnectAllChannels}, {"!chns", &CommandsHandler::printChannels},
    {"!adelfull", &CommandsHandler::deleteUserDataFull}, {"!ignorecmd", &CommandsHandler::ignoreCmd},
    {"!acmds", &CommandsHandler::getCommandsWheres}, {"!acmd", &CommandsHandler::getCommandAt},
    {"!purge", &CommandsHandler::purge}, {"!test0", &CommandsHandler::test0},
    {"!addallow", &CommandsHandler::addAllow}, {"!allow", &CommandsHandler::addAllow},
    {"!delallow", &CommandsHandler::delAllow}, {"!unallow", &CommandsHandler::delAllow},
    {"!deny", &CommandsHandler::delAllow}
};

std::unordered_map<std::string, Response (CommandsHandler::*) (const IRCMessage &, std::vector<std::string> &)> CommandsHandler::normalCommands = {
    {"!remindme", &CommandsHandler::remindMe},
    {"!remind", &CommandsHandler::remind}, {"!afk", &CommandsHandler::afk},
    {"!gn", &CommandsHandler::goodNight}, {"!isafk", &CommandsHandler::isAfk},
    {"!here", &CommandsHandler::isAfk},
    {"!where", &CommandsHandler::isFrom}, {"!country", &CommandsHandler::isFrom},
    {"!usersfrom", &CommandsHandler::getUsersFrom}, {"!userslive", &CommandsHandler::getUsersLiving},
    {"!myfrom", &CommandsHandler::myFrom}, {"!mylive", &CommandsHandler::myLiving},
    {"!mydelete", &CommandsHandler::myDelete}, {"!reminders", &CommandsHandler::myReminders},
    {"!reminder", &CommandsHandler::checkReminder}, {"!pingme", &CommandsHandler::pingMeCommand},
    {"!ri", &CommandsHandler::randomIslamicQuote}, {"!rb", &CommandsHandler::randomChristianQuote},
    {"!moduleinfo", &CommandsHandler::getModuleInfo}, {"!info", &CommandsHandler::getUserData},
    /*{"!arq", &CommandsHandler::getRandomQuote},*/ {"!set", &CommandsHandler::setData},
    {"!modules", &CommandsHandler::modules}, {"!del", &CommandsHandler::deleteMyData},
    {"!comebackmsg", &CommandsHandler::comeBackMsg}, {"!notify", &CommandsHandler::comeBackMsg},
    {"!purgeme", &CommandsHandler::purgeMe}, {"!gdpr", &CommandsHandler::gdpr},
    {"!time", &CommandsHandler::time}, {"!settime", &CommandsHandler::setTime},
    {"!deletetime", &CommandsHandler::deleteTime}, {"!deltime", &CommandsHandler::deleteTime},
    {"!timezones", &CommandsHandler::timezones}, {"!safk", &CommandsHandler::shadowAfk},
    {"!shadowafk", &CommandsHandler::shadowAfk}, {"$afk", &CommandsHandler::shadowAfk},
    {"$gn", &CommandsHandler::shadowAfk}, {"!triviastart", &CommandsHandler::triviaStart},
    {"!triviapoints", &CommandsHandler::triviaPoints}, {"!triviatop", &CommandsHandler::triviaTop5},
    {"!triviatop5", &CommandsHandler::triviaTop5}, {"!triviastop", &CommandsHandler::triviaStop}
};

std::set<std::string> emotes = {"SingsNote", "SingsMic", "TwitchSings", "SoonerLater", "HolidayTree", "HolidaySanta", "HolidayPresent", "HolidayOrnament", "HolidayLog", "HolidayCookie", "GunRun", "PixelBob", "PeteZarollOdyssey", "PeteZaroll", "FBPenalty", "FBChallenge", "FBCatch", "FBBlock", "FBSpiral", "FBPass", "FBRun", "GenderFluidPride", "NonBinaryPride", "MaxLOL", "IntersexPride", "TwitchRPG", "PansexualPride", "AsexualPride", "TransgenderPride", "GayPride", "LesbianPride", "BisexualPride", "PinkMercy", "MercyWing2", "MercyWing1", "PartyHat", "EarthDay", "TombRaid", "PopCorn", "FBtouchdown", "PurpleStar", "AngryJack", "HappyJack", "GreenTeam", "RedTeam", "TPFufun", "TwitchVotes", "DarkMode", "HSWP", "HSCheers", "PowerUpL", "PowerUpR", "LUL", "EntropyWins", "TPcrunchyroll", "TwitchUnity", "Squid4", "Squid3", "Squid2", "Squid1", "CrreamAwk", "CarlSmile", "TwitchLit", "TehePelo", "TearGlove", "SabaPing", "PunOko", "KonCha", "Kappu", "InuyoFace", "BigPhish", "BegWan", "ArigatoNas", "ThankEgg", "MorphinTime", "BlessRNG", "TheIlluminati", "TBAngel", "MVGame", "NinjaGrumpy", "PartyTime", "RlyTho", "UWot", "YouDontSay", "KAPOW", "ItsBoshyTime", "CoolStoryBob", "TriHard", "SuperVinlin", "FreakinStinkin", "Poooound", "CurseLit", "BatChest", "BrainSlug", "PrimeMe", "StrawBeary", "RaccAttack", "UncleNox", "WTRuck", "TooSpicy", "Jebaited", "DogFace", "BlargNaut", "TakeNRG", "GivePLZ", "imGlitch", "pastaThat", "copyThis", "UnSane", "DatSheffy", "TheTarFu", "PicoMause", "TinyFace", "DrinkPurple", "DxCat", "RuleFive", "VoteNay", "VoteYea", "PJSugar", "DoritosChip", "OpieOP", "FutureMan", "ChefFrank", "StinkyCheese", "NomNom", "SmoocherZ", "cmonBruh", "KappaWealth", "MikeHogu", "VoHiYo", "KomodoHype", "SeriousSloth", "OSFrog", "OhMyDog", "KappaClaus", "KappaRoss", "MingLee", "SeemsGood", "twitchRaid", "bleedPurple", "duDudu", "riPepperonis", "NotLikeThis", "DendiFace", "CoolCat", "KappaPride", "ShadyLulu", "ArgieB8", "CorgiDerp", "HumbleLife", "PraiseIt", "TTours", "mcaT", "NotATK", "HeyGuys", "Mau5", "PRChase", "WutFace", "BuddhaBar", "PermaSmug", "panicBasket", "BabyRage", "HassaanChop", "TheThing", "EleGiggle", "RitzMitz", "YouWHY", "PipeHype", "BrokeBack", "ANELE", "PanicVis", "GrammarKing", "PeoplesChamp", "SoBayed", "BigBrother", "Keepo", "Kippa", "RalpherZ", "TF2John", "ThunBeast", "WholeWheat", "DAESuppy", "FailFish", "HotPokket", "4Head", "ResidentSleeper", "FUNgineer", "PMSTwin", "PogChamp", "ShazBotstix", "BibleThump", "AsianGlow", "DBstyle", "BloodTrail", "HassanChop", "OneHand", "FrankerZ", "SMOrc", "ArsonNoSexy", "PunchTrees", "SSSsss", "Kreygasm", "KevinTurtle", "PJSalt", "SwiftRage", "DansGame", "GingerPower", "BCWarrior", "MrDestructoid", "JonCarnage", "Kappa", "RedCoat", "TheRinger", "StoneLightning", "OptimizePrime", "JKanStyle", "R)", ";P", ":P", ";)", ":/", "<3", ":O", "B)", "O_o", ":Z", ">(", ":D", ":(", ":)", "OhMyGoodness", "(poolparty)", "aPliS", "bttvSurprised", "tehPoleCat", "ShoopDaWhoop", "PancakeMix", "ForeverAlone", "RebeccaBlack", "PedoBear", "SexPanda", "BaconEffect", "SavageJerky", ":tf:", "BatKappa", "PokerFace", "RageFace", "bttvTongue", "CiGrip", "bttvHappy", "CHAccepted", "FuckYea", "bttvHeart", "OhhhKee", "WhatAYolk", "DatSauce", "monkaS", "GabeN", "bttvUnsure", "FeelsBadMan", "bttvSad", "HailHelix", "Blackappa", "HerbPerve", "iDog", "bttvGrin", "rStrike", "SwedSwag", "bttvAngry", "M&Mjc", "bttvNice", "TopHam", "D:", "TwaT", "WatChuSay", "VisLaud", "DogeWitIt", "BadAss", "Zappa", "AngelThump", "bttvWink", "Kaged", "SoSerious", "HHydro", "TaxiBro", "BroBalt", "ButterSauce", "RonSmug", "bttvCool", "SuchFraud", "bttvTwink", "CandianRage", "(puke)", "She'llBeRight", ":'(", "bttvConfused", "bttvSleep", "(chompy)", "KaRappa", "YetiZ", "miniJulia", "FishMoley", "Hhhehehe", "KKona", "LuL", "OhGod", "PoleDoge", "motnahP", "sosGame", "CruW", "RarePepe", "iamsocal", "haHAA", "FeelsBirthdayMan", "KappaCool", "BasedGod", "bUrself", "ConcernDoge", "FapFapFap", "FeelsGoodMan", "FireSpeed", "NaM", "SourPls", "SaltyCorn", "FCreep", "VapeNation", "ariW", "notsquishY", "FeelsAmazingMan", "DuckerZ", "SqShy", "Wowee", "YellowFever", "ZrehplaR", "BORT", "YooHoo", "LilZ", "ManChicken", "BeanieHipster", "CatBag", "ZliL", "ZreknarF", "LaterSooner", "OBOY", "OiMinna", "AndKnuckles", "peepoPog", "nymnBoss", "sumSmash", "gondolaPls", "feelyHands", "SlavPls", "ppHop", "gachiBASS", "nymnBOING", "nymnWink", "SchubertWalk", "nymnPls", "nymnCorn", "notBald", "GachiPls", "nymnShuffle", "gachiANGEL", "GuitarTime", "nymnC", "nymnT", "nymnUh", "gachiHop", "BBaper", "nymnPride", "gachiPRIDE", "gachiGASM", "nymnChairPls", "nymnCringe", "pepeCD", "nymnBASS", "nymnPills", "forsenSWA", "nymnCat", "nymnHammer", "nymnSmart", "headBang", "EatPooPoo", "nymnxD", "Clap", "EZ", "nymnLaugh", "peepoHonk", "crayonTime", "SchubertBall", "nymnSon", "gachiBOP", "PepeS", "forsenPls", "billyReady", "nymnSWA", "WAYTOODANK", "KKool", "ppHopper", "pepeD", "ppOverheat", "HYPERCLAP", "gachiHYPER", "pepeJAM", "Gondola2", "ppSmok", "MEGAZULUL", "DonaldPls", "TeaTime", "ppPoof", "KKaper", "ricardoFlick", "TriKool", "ppCircle", "pepeMeltdown", "FeelsOkayMan", "FeelsWeirdMan", "WEEBSDETECTED", "gachiGOLD", "POGGERS", "BBoomer", "monkaOMEGA", "weirdL", "widepeepoHappy", "SillyChamp", "4House", "ppL", "StareChamp", "HappyHobo", "WeirdChamp", "OkayChamp", "PogU", "TriEasy", "paaaajaW", "POGTOS", "NaMChamp", "Spurdoga", "HandsUp", "wideBruh", "PagChomp", "ZULOL", "PepeHands", "pepeL", "monkaW", "5Head", "KKrikey", "WeebsOut", "VaN", "PepeLaugh", "HYPERDANSGAMEW", "MaN", "OMGScoots", "monkaPickle", "4HEad", "LULW", "forsenCD", "noxWhat", "noxSorry", "WideHard", "KKoooona", "FeelsDankMan", "KKonaW", "HONEYDETECTED", "Kapp", "weSmart", "tooDank", "KKomrade", "ZULUL", "eShrug", "miniDank", "OMEGALUL"};

bool isEmote(const std::string& str)
{
    return emotes.count(str);
}

CommandsHandler::CommandsHandler(boost::asio::io_service &_ioService,
                                 Channel *_channelObject)
    : channelObject(_channelObject)
    , ioService(_ioService)
{
}

CommandsHandler::~CommandsHandler()
{
    std::cout << "CommandsHandler::~CommandsHandler(): " << this->channelObject->channelName << std::endl;
}

static float relativeSimilarity(const std::string &str1, const std::string &str2)
{
    // Longest Common Substring Problem
    std::vector<std::vector<int>> tree(str1.size(),
                                       std::vector<int>(str2.size(), 0));
    int z = 0;

    for (int i = 0; i < str1.size(); ++i)
    {
        for (int j = 0; j < str2.size(); ++j)
        {
            if (str1[i] == str2[j])
            {
                if (i == 0 || j == 0)
                {
                    tree[i][j] = 1;
                }
                else
                {
                    tree[i][j] = tree[i - 1][j - 1] + 1;
                }
                if (tree[i][j] > z)
                {
                    z = tree[i][j];
                }
            }
            else
            {
                tree[i][j] = 0;
            }
        }
    }

    // ensure that no div by 0
    return z == 0 ? 0.f
                  : float(z) /
                        std::max<int>(1, std::max(str1.size(), str2.size()));
}

Response
CommandsHandler::handle(const IRCMessage &message)
{
    UserIDs::getInstance().addUser(message.user, message.userid, message.displayname, this->channelObject->channelName);
    
    {
        std::unique_lock lk(this->triviaMtx);
        if (this->triviaRunning && this->triviaQuestionInFlight) {
            if (relativeSimilarity(changeToLower_copy(message.message), this->currentQuestion.answer) > 0.85) {
                if(this->triviaTimer) this->triviaTimer->cancel();
                this->triviaQuestionInFlight = false;
                this->triviaTimer.reset();
                this->channelObject->messenger.push_back("Trivia: " + message.user + " got it right! PogChamp the answer was: \"" + this->currentQuestion.displayAnswer + "\"");
                Trivia::addPoint(message.user);
                lk.unlock();
                
                auto t = std::make_shared<boost::asio::steady_timer>(ioService,
                                               std::chrono::seconds(5));
                t->async_wait([this, t]([[maybe_unused]] const boost::system::error_code &er) {
                    if (!this->startNextTriviaQuestion()) {
                        this->channelObject->messenger.push_back("And trivia is over FeelsBadMan");
                    }
                });
            }
        }
    }
    std::vector<std::string> tokens;
    boost::algorithm::split(tokens, message.message,
                            boost::algorithm::is_space(),
                            boost::token_compress_on);

    changeToLower(tokens[0]);

    if (!this->isAdmin(message.user)) {
        for (decltype(tokens.size()) i = 1; i < tokens.size(); i++) {
            this->channelObject->owner->sanitizeMsg(tokens[i]);
        }
    }

    Response response;

    if (this->isAdmin(message.user)) {
        auto search = CommandsHandler::adminCommands.find(tokens[0]);
        if (search != CommandsHandler::adminCommands.end()) {
            response = (this->*(search->second))(message, tokens);
        }
    }

    // a valid response
    if (response.type != Response::Type::UNKNOWN) {
        response.trigger = tokens[0];
        return response;
    }

    // messenger queue is full
    if (!this->channelObject->messenger.able() && !this->isAdmin(message.user)) {
        return response;
    }

    pt::ptree commandTree = redisClient.getCommandTree(tokens[0]);
    if (!commandTree.empty()) {
        
        // channel user command
        boost::optional<std::string> responseString =
            commandTree.get_optional<std::string>("channels." + message.channel +
                                                  "." + message.user + ".response");
        if (responseString) {
            return this->makeResponse(
                message, *responseString, tokens, commandTree,
                "channels." + message.channel + "." + message.user);
        }

        // global user command
        responseString =
            commandTree.get_optional<std::string>(message.user + ".response");
        if (responseString) {
            return this->makeResponse(message, *responseString, tokens, commandTree,
                                      message.user);
        }

        // default channel command
        responseString = commandTree.get_optional<std::string>(
            "channels." + message.channel + ".default.response");
        if (responseString) {
            return this->makeResponse(message, *responseString, tokens, commandTree,
                                      "channels." + message.channel + ".default");
        }

        // default global command
        responseString = commandTree.get_optional<std::string>("default.response");
        if (responseString) {
            return this->makeResponse(message, *responseString, tokens, commandTree,
                                      "default");
        }
    }
    
    {
        auto search = CommandsHandler::normalCommands.find(tokens[0]);
        if (search != CommandsHandler::normalCommands.end()) {
            response = (this->*(search->second))(message, tokens);
        }
    }

    // response valid
    if (response.type != Response::Type::UNKNOWN) {
        response.trigger = tokens[0];
        return response;
    }
    
    if (this->isAdmin(message.user)) {
        response = this->delegateWords(message, tokens);
        if (response.type != Response::Type::UNKNOWN) {
            return response;
        }
    }
    return this->highlightResponse(message, tokens);
}

Response
CommandsHandler::getCommandsWheres(const IRCMessage &message,
                               std::vector<std::string> &tokens)
{
   Response response(1);
   if (tokens.size() < 2) {
       return response;
   }
   changeToLower(tokens[1]);
   pt::ptree commandTree = redisClient.getCommandTree(tokens[1]);
   for (const auto& it : commandTree) {
       if (it.first == "channels") {
           response.message += "channels{";
           for (const auto& secondp : it.second) {
               response.message += secondp.first + "." + secondp.second.begin()->first + " ";
           }
           response.message.pop_back();
           response.message += "} ";
       } else {
       response.message += it.first + " ";
       }
   }
   response.type = Response::Type::MESSAGE;
   return response;
}

Response
CommandsHandler::getCommandAt(const IRCMessage &message,
                               std::vector<std::string> &tokens)
{
   Response response(1);
   if (tokens.size() < 3) {
       return response;
   }
   changeToLower(tokens[1]);
   changeToLower(tokens[2]);
   pt::ptree commandTree = redisClient.getCommandTree(tokens[1]);
   boost::optional<pt::ptree &> child =
        commandTree.get_child_optional(tokens[2]);
   if (child) {
       std::stringstream ss;
       pt::write_json(ss, *child);
       response.message = ss.str();
       response.type = Response::Type::MESSAGE;
       boost::algorithm::replace_all(response.message, "\n", "");
   }
   return response;
}

Response
CommandsHandler::delegateWords(const IRCMessage &message,
                               std::vector<std::string> &tokens)
{
    Response response(1);
    
    std::vector<std::pair<int, int>> poswords;
    for (int i = 0; i < tokens.size(); ++i) {
        const auto &token = tokens[i];
        if (token.compare(0, 3, "!\\\\") == 0) {
            int words = 1;
            if (token.size() > 3) {
                words = std::atoi(token.c_str() + 3);
            }
            poswords.push_back({i, words});
        }
        if (token.compare(0, 9, "\u206D\u206D\u206D") == 0) {
            int words = 1;
            if (token.size() > 9) {
                words = std::atoi(token.c_str() + 9);
            }
            poswords.push_back({i, words});
        }
    }
    if (poswords.empty()) {
        return response;
    }

    for (auto &i : poswords) {
        auto &pos = i.first;
        auto &words = i.second;
        if (pos < tokens.size()) {
            ++pos;
            int j = 0;
            while ((pos + j) < tokens.size() && j < words) {
                response.message += tokens[pos + j] + " ";
                ++j;
            }
        }
    }
    
    if (response.message.empty()) {
        return response;
    }
    response.type = Response::Type::MESSAGE;
    return response;
}

Response
CommandsHandler::time([[maybe_unused]] const IRCMessage &message,
                                   std::vector<std::string> &tokens)
{
    Response response;
    auto& time = Time::getInstance();
    
    if (tokens.size() < 2) {
        auto str = time.getUsersCurrentTime(message.userid);
        if (str.empty()) {
            response.message = message.user + ", you have not set your time zone, use !settime <TIMEZONE>, for a list of time zones see https://en.wikipedia.org/wiki/List_of_tz_database_time_zones";
            response.whisperReceiver = message.user;
            response.type = Response::Type::WHISPER;
            return response;
        }
        response.message = message.user + ", your current time is " + str;
        response.type = Response::Type::MESSAGE;
        return response;
    } else {
        changeToLower(tokens[1]);
        
        auto& userids = UserIDs::getInstance();
        if(userids.isUser(tokens[1])) {
            auto userid = userids.getID(tokens[1]); 
            auto str = time.getUsersCurrentTime(userid);
            if (str.empty()) {
                response.message = "User " + tokens[1] + " has not set their time zone";
            } else {
                response.message = message.user + ", " + tokens[1] + "'s current time is " + str;
            }
            response.type = Response::Type::MESSAGE;
        } else {
            response.message = "User " + tokens[1] + " does not exist";
            response.whisperReceiver = message.user;
            response.type = Response::Type::WHISPER;
        }
        return response;
    }
}

Response
CommandsHandler::setTime([[maybe_unused]] const IRCMessage &message,
                                   std::vector<std::string> &tokens)
{
    Response response;
    
    if (tokens.size() < 2) {
        response.message = "Usage: !settime <TIMEZONE>, for a list of time zones see https://en.wikipedia.org/wiki/List_of_tz_database_time_zones";
        response.whisperReceiver = message.user;
        response.type = Response::Type::WHISPER;
        return response;
    }
    auto& time = Time::getInstance();
    
    if (!time.setTimeZone(message.userid, tokens[1])) {
        response.message = message.user + ", this time zone does not exist, type !timezones for a whisper with time zones wiki link or search for \"List of tz database time zones\"";
        
    } else {
        response.message = message.user + ", successfully set your time zone to " + tokens[1];
    }
    response.type = Response::Type::MESSAGE;
    return response;
}

Response
CommandsHandler::deleteTime([[maybe_unused]] const IRCMessage &message,
                                   std::vector<std::string> &tokens)
{
    Response response;
    auto &time = Time::getInstance();
    time.delTimeZone(message.userid);
    response.message = message.user + ", successfully deleted your time zone";
    response.type = Response::Type::MESSAGE;
    return response;
}

Response
CommandsHandler::timezones([[maybe_unused]] const IRCMessage &message,
                                   std::vector<std::string> &tokens)
{
    Response response;
    response.message = "For a list of time zones see https://en.wikipedia.org/wiki/List_of_tz_database_time_zones";
    response.whisperReceiver = message.user;
    response.type = Response::Type::WHISPER;
    return response;
}

Response
CommandsHandler::highlightResponse([[maybe_unused]] const IRCMessage &message,
                                   std::vector<std::string> &tokens)
{
    Response response;
    return response;
    if (!this->softCooldownCheck("", "\\_weneedmoreautisticbots", "\\_weneedmoreautisticbots", message.channel, 30)) {
        return response;
    }
    
    auto &mt = MTRandom::getInstance();
    
    if (mt.getBool(0.9)) {
        return response;
    }
    
    for (auto &i : tokens) {
        changeToLower(i);
        if (i.find("weneedmoreautisticbots") != std::string::npos) {
            static const char *rsp[] = {"monkaS", "monkaGIGA", "hwat pajaDank", "OMEGALUL", "LUL", "Kappa", "Keepo", "forsenSWA", "pajaDank"};
            response.message = rsp[mt.getInt(0, std::size(rsp) - 1)];
            response.type = Response::Type::MESSAGE;
            this->cooldownCheck("", "\\_weneedmoreautisticbots", "\\_weneedmoreautisticbots", message.channel, 25);
            return response;
        }
    }
    
    return response;
}

Response
CommandsHandler::joinChannel(const IRCMessage &message,
                             std::vector<std::string> &tokens)
{
    Response response(1);
    if (tokens.size() < 2) {
        this->channelObject->whisper("Usage: !joinchn <channel>", message.user);
        return response;
    }

    changeToLower(tokens[1]);

    if (this->channelObject->owner->joinChannel(tokens[1])) {
        response.message = "Joined the " + tokens[1] + " channel SeemsGood";
        response.type = Response::Type::MESSAGE;
    }
    return response;
}

Response
CommandsHandler::leaveChannel(const IRCMessage &message,
                              std::vector<std::string> &tokens)
{
    Response response(1);
    if (tokens.size() < 2) {
        this->channelObject->whisper("Usage: !leavechn <channel>",
                                     message.user);
        return response;
    }

    changeToLower(tokens[1]);
    if (tokens[1] == this->channelObject->channelName) {
        response.message = "Cant leave " + tokens[1] + " channel from itself tooDank";
        response.type = Response::Type::MESSAGE;
        return response;
    }

    if (this->channelObject->owner->leaveChannel(tokens[1])) {
        response.message = "Left the " + tokens[1] + " channel SeemsGood";
        response.type = Response::Type::MESSAGE;
    }
    return response;
}

Response
CommandsHandler::printChannels(const IRCMessage &message,
                               std::vector<std::string> &tokens)
{
    Response response(1);
    
    if (!this->cooldownCheck("", message.user, tokens[0], message.channel)) {
        return response;
    }

    std::string channels;
    for (const auto &i : this->channelObject->owner->channels) {
        channels += i.first + ", ";
    }
    channels.pop_back();
    channels.pop_back();

    response.message = "Currently active in channels " + channels;
    response.type = Response::Type::MESSAGE;
    return response;
}

Response
CommandsHandler::makeResponse(const IRCMessage &message,
                              std::string &responseString,
                              std::vector<std::string> &tokens,
                              const boost::property_tree::ptree &commandTree,
                              const std::string &path)
{
    Response response(0);
    response.trigger = tokens[0];

    if (!this->isAdmin(message.user)) {
        {
            boost::optional<int> cooldown =
            commandTree.get_optional<int>(path + ".cooldown");
            if (cooldown && *cooldown != 0) {
                std::lock_guard<std::mutex> lk(this->cooldownsMtx);
                auto it = this->cooldownsMap.find(tokens[0] + ":" + path + ":" + message.channel);
                auto now = std::chrono::steady_clock::now();
                if (it != cooldownsMap.end()) {
                    if (std::chrono::duration_cast<std::chrono::seconds>(now -
                                                                         it->second)
                            .count() < *cooldown) {
                        return response;
                    }
                }
                cooldownsMap[tokens[0] + ":" + path + ":" + message.channel] = now;
                auto t = std::make_shared<boost::asio::steady_timer>(ioService,
                                                       std::chrono::seconds(*cooldown));
                t->async_wait([ this, key = tokens[0] + ":" + path + ":" + message.channel, t ](
                    [[maybe_unused]] const boost::system::error_code &er) {
                        std::lock_guard<std::mutex> lk(this->cooldownsMtx);
                        this->cooldownsMap.erase(key);
                });
            }
        }
        {
            boost::optional<int> cooldown =
            commandTree.get_optional<int>(path + ".usercd");
            if (cooldown && *cooldown != 0) { 
                if (!this->cooldownCheck(path, message.user, tokens[0], message.channel, *cooldown)) {
                    return response;
                }
            }
        }
    }
    
    {
        boost::regex e("{if:(\\d+):([^:]+):del}");
        boost::match_results<std::string::const_iterator> results;
        while (boost::regex_search(responseString, results, e)) {
            std::string numAfter = results[1];
            int num = std::atoi(numAfter.c_str());
            if (num < tokens.size()) {
                std::string what = results[2];
                if ((what == "user" && UserIDs::getInstance().isUser(tokens[num]) && !isEmote(tokens[num])) || tokens[num] == what) {
                    tokens.erase(tokens.begin() + num);
                }
            }
            responseString = boost::regex_replace(responseString, e, "", boost::match_default | boost::format_first_only);
        }
    }
    
    {
        boost::regex e("\\[if:(\\d+):([^:]+):(.+?)\\]");
        boost::match_results<std::string::const_iterator> results;
        while (boost::regex_search(responseString, results, e)) {
            std::string numAfter = results[1];
            int num = std::atoi(numAfter.c_str());
            if (num < tokens.size()) {
                std::string what = results[2];
                std::string with = results[3];
                if (with == "{user}") {
                    with = message.user;
                } else if (with == "{channel}") {
                    with = message.channel;
                } else if (auto nm = std::atoi(numAfter.c_str()); nm && nm < tokens.size()) {
                    with = tokens[nm];
                }
                if ((what == "user" && UserIDs::getInstance().isUser(tokens[num]) && !isEmote(tokens[num])) || tokens[num] == what) {
                    tokens[num] = with;
                }
                if (what == "!user" && !UserIDs::getInstance().isUser(tokens[num])) {
                    tokens[num] = with;
                }
            }
            responseString = boost::regex_replace(responseString, e, "", boost::match_default | boost::format_first_only);
        }
    }
    
    {
        boost::regex e("\\[if;(\\d+);(.*);(.*)\\]");
        boost::match_results<std::string::const_iterator> results;
        while (boost::regex_search(responseString, results, e)) {
            std::string numAfter = results[1];
            int num = std::atoi(numAfter.c_str());
            if (num < tokens.size()) {
                std::string exists = results[2];
                responseString = boost::regex_replace(responseString, e, exists, boost::match_default | boost::format_first_only);
            } else {
                std::string notexists = results[3];
                responseString = boost::regex_replace(responseString, e, notexists, boost::match_default | boost::format_first_only);
            }
        }
    }
    
    
    
    // defaults
    
    {
        // \d+ 1
        // (({user})|({channel})) 2
        // ({user}) 3
        // ({channel}) 4
        boost::regex e("{(\\d+)\\|((user)|(channel))}");
        boost::match_results<std::string::const_iterator> results;
        while (boost::regex_search(responseString, results, e)) {
            std::string numAfter = results[1];
            int num = std::atoi(numAfter.c_str());
            if (num < tokens.size()) {
                std::string rpl = "{" + numAfter + "}";
                responseString = boost::regex_replace(responseString, e, tokens[num], boost::match_default | boost::format_first_only);
            } else {
                if (results[3].matched) {
                    responseString = boost::regex_replace(responseString, e, message.user, boost::match_default | boost::format_first_only);
                }
                if (results[4].matched) {
                    responseString = boost::regex_replace(responseString, e, message.channel, boost::match_default | boost::format_first_only);
                }
            }
        }
    }
    
    {
        // \d+ 1
        // user 2
        boost::regex e("{(\\d+)\\&\\|((user)|(channel))}");
        boost::match_results<std::string::const_iterator> results;
        while (boost::regex_search(responseString, results, e)) {
            std::string numAfter = results[1];
            int num = std::atoi(numAfter.c_str());
            if (num < tokens.size() && UserIDs::getInstance().isUser(tokens[num])) {
                responseString = boost::regex_replace(responseString, e, tokens[num], boost::match_default | boost::format_first_only);
                
            } else if (results[3].matched) {
                responseString = boost::regex_replace(responseString, e, message.user, boost::match_default | boost::format_first_only);
            } else if (results[4].matched) {
                responseString = boost::regex_replace(responseString, e, message.channel, boost::match_default | boost::format_first_only);
            }
        }
    }
    
    
    
    {
        // \d+\+ 1
        // ([^}]*) 2
        boost::regex e("{(\\d+)\\+\\|((user)|(channel))}");
        boost::match_results<std::string::const_iterator> results;
        while (boost::regex_search(responseString, results, e)) {
            std::string numAfter = results[1];
            int num = std::atoi(numAfter.c_str());
            std::string msg;
            for (decltype(tokens.size()) i = num; i < tokens.size(); i++) {
                msg += tokens[i] + " ";
            }
            if (!msg.empty()) {
                msg.pop_back();
                responseString = boost::regex_replace(responseString, e, msg, boost::match_default | boost::format_first_only);
            } else {
                if (results[3].matched) {
                    responseString = boost::regex_replace(responseString, e, message.user, boost::match_default | boost::format_first_only);
                }
                if (results[4].matched) {
                    responseString = boost::regex_replace(responseString, e, message.channel, boost::match_default | boost::format_first_only);
                }
            }
        }
    }
    
    
    
    {
        // \d+ 1
        // ([^}]*) 2
        boost::regex e("{(\\d+)\\|([^}]*)}");
        boost::match_results<std::string::const_iterator> results;
        while (boost::regex_search(responseString, results, e)) {
            std::string numAfter = results[1];
            int num = std::atoi(numAfter.c_str());
            if (num < tokens.size()) {
                std::string rpl = "{" + numAfter + "}";
                responseString = boost::regex_replace(responseString, e, tokens[num], boost::match_default | boost::format_first_only);
            } else {
                if (results[2].matched) {
                    std::string match = results[2];
                    responseString = boost::regex_replace(responseString, e, match, boost::match_default | boost::format_first_only);
                }
            }
        }
    }
    
    
    
    {
        // \d+\+ 1
        // ([^}]*) 2
        boost::regex e("{(\\d+)\\+\\|([^}]*)}");
        boost::match_results<std::string::const_iterator> results;
        while (boost::regex_search(responseString, results, e)) {
            std::string numAfter = results[1];
            int num = std::atoi(numAfter.c_str());
            std::string msg;
            for (decltype(tokens.size()) i = num; i < tokens.size(); i++) {
                msg += tokens[i] + " ";
            }
            if (!msg.empty()) {
                msg.pop_back();
                responseString = boost::regex_replace(responseString, e, msg, boost::match_default | boost::format_first_only);
            } else {
                if (results[2].matched) {
                    std::string match = results[2];
                    responseString = boost::regex_replace(responseString, e, match, boost::match_default | boost::format_first_only);
                }
            }
        }
    }
    
    
    
    int foundNumParamsInToken = 0;
    {
        std::vector<int> vecNumParams{0};  // vector of {number} numbers
        boost::match_results<std::string::const_iterator> results;
        if (boost::regex_search(responseString, results, boost::regex("{\\d+}"))) {
            for (const auto& it_range : results) {
                vecNumParams.push_back(boost::lexical_cast<int>(
                std::string(it_range.begin() + 1, it_range.end() - 1)));
            }
            
        }
        foundNumParamsInToken = *std::max_element(vecNumParams.begin(), vecNumParams.end());
    }
    
    
    // defaults

    boost::optional<int> numParams =
        commandTree.get_optional<int>(path + ".numParams");
    if (numParams || foundNumParamsInToken) {
        if (tokens.size() <= std::max(*numParams, foundNumParamsInToken)) {
            return response;
        }

        std::stringstream ss;
        for (int i = 1; i <= std::max(*numParams, foundNumParamsInToken); ++i) {
            ss.str(std::string());
            ss.clear();
            ss << '{' << i << '}';
            if (boost::algorithm::find_regex(
                    tokens[i], boost::regex("{\\d+}|{user}|{channel}"))) {
                return response;
            }

            // this->channelObject->owner->sanitizeMsg(tokens[i]);
            boost::algorithm::replace_all(responseString, ss.str(), tokens[i]);
        }
    }
    
    
    
    if (responseString.find("{all}") != std::string::npos)
    {
        std::string valueString;
        for (decltype(tokens.size()) i = 1; i < tokens.size(); ++i) {
            valueString += tokens[i] + ' ';
        }
        if (!valueString.empty()) {
            valueString.pop_back();
            boost::algorithm::replace_all(responseString, "{all}", valueString);
        } else {
            return response;
        }
    }
    
    
    
    boost::algorithm::replace_all(responseString, "{user}", message.user);
    boost::algorithm::replace_all(responseString, "{channel}", message.channel);
    boost::algorithm::replace_all(responseString, "{time}", localTime());
    boost::algorithm::replace_all(responseString, "{date}", localDate());

    
    
    
    {
        boost::regex e("{(\\d+)\\+}");
        boost::match_results<std::string::const_iterator> results;
        while (boost::regex_search(responseString, results, e)) {
            std::string numAfter = results[1];
            int num = std::atoi(numAfter.c_str());
            std::string msg;
            for (decltype(tokens.size()) i = num; i < tokens.size(); i++) {
                msg += tokens[i] + " ";
            }
            if (msg.empty()) {
                return response;
            } else {
                msg.pop_back();
            }
            responseString = boost::regex_replace(responseString, e, msg, boost::match_default | boost::format_first_only);
        }
    }
    
    
    
    {
        boost::regex e("{\\?(\\d+)\\+}");
        boost::match_results<std::string::const_iterator> results;
        while (boost::regex_search(responseString, results, e)) {
            std::string numAfter = results[1];
            int num = std::atoi(numAfter.c_str());
            std::string msg;
            for (decltype(tokens.size()) i = num; i < tokens.size(); i++) {
                msg += tokens[i] + " ";
            }
            if (!msg.empty()) {
                msg.pop_back();
            }
            responseString = boost::regex_replace(responseString, e, msg, boost::match_default | boost::format_first_only);
        }
    }
    
    

    if (boost::algorithm::find_regex(responseString, boost::regex("{cirnd}"))) {
        boost::algorithm::replace_all(
            responseString, "{cirnd}",
            std::to_string(MTRandom::getInstance().getInt()));
    }
    
    

    while (
        boost::algorithm::find_regex(responseString, boost::regex("{irnd}"))) {
        boost::algorithm::replace_first(
            responseString, "{irnd}",
            std::to_string(MTRandom::getInstance().getInt()));
    }
    
    

    if (boost::algorithm::find_regex(responseString, boost::regex("{cdrnd}"))) {
        boost::algorithm::replace_all(
            responseString, "{cdrnd}",
            std::to_string(MTRandom::getInstance().getReal()));
    }
    
    

    while (
        boost::algorithm::find_regex(responseString, boost::regex("{drnd}"))) {
        boost::algorithm::replace_first(
            responseString, "{drnd}",
            std::to_string(MTRandom::getInstance().getReal()));
    }
    
    
    
    {
        boost::regex e("{cirnd (-?\\d+) (-?\\d+)}");
        boost::match_results<std::string::const_iterator> results;
        while (boost::regex_search(responseString, results, e)) {
            std::string smin = results[1];
            std::string smax = results[2];
            auto num = MTRandom::getInstance().getInt(std::atoll(smin.c_str()), std::atoll(smax.c_str()));
            boost::algorithm::replace_all(responseString, "{cirnd " + smin + " " + smax + "}", std::to_string(num));
        }
    }
    
    
    
    {
        boost::regex e("{irnd\\s(-?\\d+)\\s(-?\\d+)}");
        boost::match_results<std::string::const_iterator> results;
        while (boost::regex_search(responseString, results, e)) {
            std::string smin = results[1];
            std::string smax = results[2];
            auto num = MTRandom::getInstance().getInt(std::atoll(smin.c_str()), std::atoll(smax.c_str()));
            boost::algorithm::replace_first(responseString, "{irnd " + smin + " " + smax + "}", std::to_string(num));
        }
    }
    
    

    {
        boost::regex e("{page:\\s*([^}]+)}");
        boost::match_results<std::string::const_iterator> results;
        while (boost::regex_search(responseString, results, e)) {
            std::string page = results[1];
            std::string replace = Hemirt::getRandom(page);
            responseString = boost::regex_replace(responseString, e, replace, boost::match_default | boost::format_first_only);
        }
    }

    
    
    {
        boost::regex e("{rawpage:\\s*([^}]+)}");
        boost::match_results<std::string::const_iterator> results;
        while (boost::regex_search(responseString, results, e)) {
            std::string page = results[1];
            std::string replace = Hemirt::getRaw(page);
            responseString = boost::regex_replace(responseString, e, replace, boost::match_default | boost::format_first_only);
        }
    }
    
    


    auto it = boost::algorithm::find_regex(
        responseString, boost::regex("\\{([^\\}:]+):([^\\}]*)\\}"));
    bool success = MTRandom::getInstance().getBool();
    if (it) {
        do {
            std::string match(it.begin(), it.end());
            if (match.compare(0, strlen("{true:"), "{true:") == 0) {
                if (success) {
                    boost::algorithm::replace_regex(
                        responseString,
                        boost::regex("\\{([^\\}:]+):([^\\}]*)\\}"),
                        std::string(match.begin() + sizeof("{true:") - 1,
                                    match.end() - 1),
                        boost::match_default | boost::format_first_only);
                } else {
                    boost::algorithm::erase_regex(
                        responseString,
                        boost::regex("\\{([^\\}:]+):([^\\}]*)\\}"),
                        boost::match_default | boost::format_first_only);
                }
            } else if (match.compare(0, strlen("{false:"), "{false:") == 0) {
                if (!success) {
                    boost::algorithm::replace_regex(
                        responseString,
                        boost::regex("\\{([^\\}:]+):([^\\}]*)\\}"),
                        std::string(match.begin() + sizeof("{false:") - 1,
                                    match.end() - 1),
                        boost::match_default | boost::format_first_only);
                } else {
                    boost::algorithm::erase_regex(
                        responseString,
                        boost::regex("\\{([^\\}:]+):([^\\}]*)\\}"),
                        boost::match_default | boost::format_first_only);
                }
            } else {
                std::cerr << "unknown match: " << match << std::endl;
                boost::algorithm::replace_regex(
                    responseString, boost::regex("\\{([^\\}:]+):([^\\}]*)\\}"),
                    std::string("{unknown}"),
                    boost::match_default | boost::format_first_only);
            }
        } while ((
            it = boost::algorithm::find_regex(
                responseString, boost::regex("\\{([^\\}:]+):([^\\}]*)\\}"))));
    }
    
    if (!this->isAdmin(message.user))
    {
        std::cout << response.message << std::endl;
        boost::regex e("\\$\\$sntz\\$\\$(.*?)\\$\\$sntz\\$\\$");
        boost::match_results<std::string::const_iterator> results;
        while (boost::regex_search(responseString, results, e)) {
            std::string replace = results[1];
            this->channelObject->owner->sanitizeMsg(replace);
            responseString = boost::regex_replace(responseString, e, replace, boost::match_default | boost::format_first_only);
        }
    } else {
        std::cout << response.message << std::endl;
        boost::regex e("\\$\\$sntz\\$\\$(.*?)\\$\\$sntz\\$\\$");
        boost::match_results<std::string::const_iterator> results;
        while (boost::regex_search(responseString, results, e)) {
            std::string replace = results[1];
            responseString = boost::regex_replace(responseString, e, replace, boost::match_default | boost::format_first_only);
        }
    }
    
    response.message = responseString;
    response.type = Response::Type::MESSAGE;
    return response;
}

Response
CommandsHandler::addCommand(const IRCMessage &message,
                            std::vector<std::string> &tokens)
{
    Response response(1);
    if (tokens.size() < 4) {
        response.message = "Invalid use of command NaM";
        response.type = Response::Type::MESSAGE;
        return response;
    }

    changeToLower(tokens[1]);
    changeToLower(tokens[2]);

    // toke[0] toke[1] toke[2] toke[3]
    // !addcmd trigger default fkfkfkfkfk kfs kfosd kfods kfods

    std::string valueString;
    for (size_t i = 3; i < tokens.size(); ++i) {
        valueString += tokens[i] + ' ';
    }
    valueString.pop_back();

    pt::ptree commandTree = redisClient.getCommandTree(tokens[1]);

    boost::optional<pt::ptree &> child =
        commandTree.get_child_optional(tokens[2]);
    if (child) {
        response.message =
            "Command " + tokens[1] + " at " + tokens[2] + " already exists.";
        response.type = Response::Type::MESSAGE;
        return response;
    }

    std::vector<int> vecNumParams{0};  // vector of {number} numbers
    for (size_t i = 3; i < tokens.size(); ++i) {
        auto it_range =
            boost::algorithm::find_regex(tokens[i], boost::regex("{\\d+}"));
        if (!it_range) {
            continue;
        }
        vecNumParams.push_back(boost::lexical_cast<int>(
            std::string(it_range.begin() + 1, it_range.end() - 1)));
    }
    int numParams = *std::max_element(vecNumParams.begin(), vecNumParams.end());

    commandTree.put(tokens[2] + ".response", valueString);
    commandTree.put(tokens[2] + ".numParams", numParams);
    commandTree.put(tokens[2] + ".cooldown", 0);

    std::stringstream ss;
    pt::write_json(ss, commandTree, false);

    redisClient.setCommandTree(tokens[1], ss.str());

    response.message = message.user + " added command " + tokens[1] + " at " +
                       tokens[2] + "  SeemsGood";
    response.type = Response::Type::MESSAGE;
    return response;
}

Response
CommandsHandler::ignoreCmd(const IRCMessage &message,
                            std::vector<std::string> &tokens)
{
    Response response(1);
    if (tokens.size() < 2) {
        response.message = "Invalid use of command NaM : !ignorecmd [cmdname] {optionaly where, see !syntax, default is channels.[thischannel].default}";
        response.type = Response::Type::MESSAGE;
        return response;
    }

    changeToLower(tokens[1]);

    // toke[0] toke[1]
    // !ignorecmd trigger

    pt::ptree commandTree = redisClient.getCommandTree(tokens[1]);

    std::string subchn = "channels." + message.channel + ".default";
    if (tokens.size() == 3) {
        if (tokens[2].size() > 2) {
            changeToLower(tokens[2]);
            subchn = tokens[2];
        }
    }
    std::string valueString = "/w ";

    int numParams = 0;

    commandTree.put(subchn + ".response", valueString);
    commandTree.put(subchn + ".numParams", numParams);  
    commandTree.put(subchn + ".cooldown", 0);

    std::stringstream ss;
    pt::write_json(ss, commandTree, false);

    redisClient.setCommandTree(tokens[1], ss.str());

    response.message = message.user + " ignored command " + tokens[1] + " at " +
                       subchn + "  PepeHands";
    response.type = Response::Type::MESSAGE;
    return response;
}

Response
CommandsHandler::editCommand(const IRCMessage &message,
                             std::vector<std::string> &tokens)
{
    Response response(1);
    if (tokens.size() < 4) {
        response.message = "Invalid use of command NaM";
        response.type = Response::Type::MESSAGE;
        return response;
    }

    changeToLower(tokens[1]);
    changeToLower(tokens[2]);

    // toke[0] toke[1] toke[2] toke[3]
    // !editcmd trigger default fkfkfkfkfk kfs kfosd kfods kfods

    std::string valueString;
    for (size_t i = 3; i < tokens.size(); ++i) {
        valueString += tokens[i] + ' ';
    }
    valueString.pop_back();

    pt::ptree commandTree = redisClient.getCommandTree(tokens[1]);

    boost::optional<pt::ptree &> child =
        commandTree.get_child_optional(tokens[2]);
    if (!child) {
        response.message =
            "Command " + tokens[1] + " at " + tokens[2] + " doesn\'t exists.";
        response.type = Response::Type::MESSAGE;
        return response;
    }

    std::vector<int> vecNumParams{0};  // vector of {number} numbers
    for (size_t i = 3; i < tokens.size(); ++i) {
        auto it_range =
            boost::algorithm::find_regex(tokens[i], boost::regex("{\\d+}"));
        if (!it_range) {
            continue;
        }
        vecNumParams.push_back(boost::lexical_cast<int>(
            std::string(it_range.begin() + 1, it_range.end() - 1)));
    }
    int numParams = *std::max_element(vecNumParams.begin(), vecNumParams.end());

    commandTree.put(tokens[2] + ".response", valueString);
    commandTree.put(tokens[2] + ".numParams", numParams);

    std::stringstream ss;
    pt::write_json(ss, commandTree, false);

    redisClient.setCommandTree(tokens[1], ss.str());

    response.message = message.user + " edited command " + tokens[1] + " at " +
                       tokens[2] + "  SeemsGood";
    response.type = Response::Type::MESSAGE;
    return response;
}

Response
CommandsHandler::rawEditCommand(const IRCMessage &message,
                                std::vector<std::string> &tokens)
{
    Response response(1);
    if (tokens.size() < 5) {
        response.message = "Invalid use of command NaM";
        response.type = Response::Type::MESSAGE;
        return response;
    }
    changeToLower(tokens[1]);
    changeToLower(tokens[2]);

    // toke[0]   toke[1] toke[2] toke[3]
    // !editcmd trigger default response fkfkfkfkfk kfs kfosd kfods kfods
    // !editcmd trigger default numParams 0
    // !editcmd trigger default cooldown 4

    if (tokens[3].find('.') != std::string::npos) {
        return response;
    }
    std::string pathstring = tokens[2] + '.' + tokens[3];
    pt::ptree::path_type path(pathstring);
    pt::ptree commandTree = redisClient.getCommandTree(tokens[1]);

    std::string valueString;
    for (size_t i = 4; i < tokens.size(); ++i) {
        valueString += tokens[i] + ' ';
    }
    valueString.pop_back();

    commandTree.put(path, valueString);

    std::stringstream ss;

    pt::write_json(ss, commandTree, false);

    redisClient.setCommandTree(tokens[1], ss.str());

    response.message =
        message.user + " edited " + tokens[1] + " command SeemsGood";
    response.type = Response::Type::MESSAGE;
    return response;
}

Response
CommandsHandler::deleteCommand(const IRCMessage &message,
                               std::vector<std::string> &tokens)
{
    Response response(1);
    if (tokens.size() < 3) {
        response.message = "Invalid use of command NaM";
        response.type = Response::Type::MESSAGE;
        return response;
    }
    changeToLower(tokens[1]);
    changeToLower(tokens[2]);
    // tokens[0]     1       2
    //! deletecmd trigger default

    pt::ptree commandTree = redisClient.getCommandTree(tokens[1]);

    boost::optional<pt::ptree &> child =
        commandTree.get_child_optional(tokens[2]);
    if (child) {
        child->clear();
        std::vector<std::string> pathtokens;
        boost::algorithm::split(pathtokens, tokens[2],
                                boost::algorithm::is_any_of("."),
                                boost::token_compress_on);
        std::string last;
        while (pathtokens.size()) {
            last = pathtokens.back();
            pathtokens.pop_back();
            auto &node =
                commandTree.get_child(boost::algorithm::join(pathtokens, "."));
            node.erase(last);

            if (!node.empty()) {
                break;
            }
        }
    }

    if (commandTree.empty()) {
        redisClient.deleteFullCommand(tokens[1]);
    } else {
        std::stringstream ss;
        pt::write_json(ss, commandTree, false);
        redisClient.setCommandTree(tokens[1], ss.str());
        ;
    }

    response.message = message.user + " deleted " + tokens[1] + " command at " +
                       tokens[2] + " FeelsBadMan";
    response.type = Response::Type::MESSAGE;
    return response;
}

Response
CommandsHandler::deleteFullCommand(const IRCMessage &message,
                                   std::vector<std::string> &tokens)
{
    Response response(1);
    if (tokens.size() < 2) {
        response.message = "Invalid use of command NaM";
        response.type = Response::Type::MESSAGE;
        return response;
    }
    // tokens[0]          tokens[1]
    //! deletefullcommand trigger
    changeToLower(tokens[1]);

    redisClient.deleteFullCommand(tokens[1]);
    response.message = message.user + " deleted everything related to the " +
                       tokens[1] + " command BibleThump";
    response.type = Response::Type::MESSAGE;
    return response;
}

bool
CommandsHandler::isAdmin(const std::string &user)
{
    return this->redisClient.isAdmin(user);
}

size_t
findLastIn(std::vector<std::string> &tokens)
{
    auto i = tokens.size();
    for (; i-- > 1;) {
        if (tokens[i] == "in" || tokens[i] == "IN" || tokens[i] == "iN" ||
            tokens[i] == "In") {
            break;
        }
    }
    return i;
}

int64_t
getSecondsCount(std::vector<std::string> &tokens, size_t inPos)
{
    int64_t seconds = 0;
    for (decltype(tokens.size()) i = inPos + 1; i < tokens.size(); i++) {
        if (tokens[i].back() == 'd') {
            seconds += std::atoll(tokens[i].c_str()) * 24 * 3600;
        } else if (tokens[i].back() == 'h') {
            seconds += std::atoll(tokens[i].c_str()) * 3600;
        } else if (tokens[i].back() == 'm') {
            seconds += std::atoll(tokens[i].c_str()) * 60;
        } else if (tokens[i].back() == 's') {
            seconds += std::atoll(tokens[i].c_str());
        }
    }
    return seconds;
}

std::string
makeReminderMsg(std::vector<std::string> &tokens, size_t inPos)
{
    std::string str;
    for (decltype(tokens.size()) j = 1; j < inPos; j++) {
        str += tokens[j] + " ";
    }
    if (str.back() == ' ') {
        str.pop_back();
    }
    return str;
}

Response
CommandsHandler::remindMe(const IRCMessage &message,
                          std::vector<std::string> &tokens)
{
    Response response(0);
    auto inPos = findLastIn(tokens);
    if (inPos == 0 || inPos + 1 >= tokens.size()) {
        response.type = Response::Type::WHISPER;
        response.whisperReceiver = message.user;
        response.message = "Usage \"!remindme [your message] in 20s "
                           "15h 10d 9m (the order does not matter, "
                           "the number must be immediately followed "
                           "by an identifier)\"";
        return response;
    }

    long long seconds = getSecondsCount(tokens, inPos);
    if (seconds == 0) {
        response.type = Response::Type::WHISPER;
        response.message = "Usage \"!remindme [your message] in 20s "
                           "15h 10d 9m (the order does not matter, "
                           "the number must be immediately followed "
                           "by an identifier)\"";
        response.whisperReceiver = message.user;
        return response;
    } else if (seconds > 604800) {
        response.type = Response::Type::WHISPER;
        response.message = "Maximum amount of time is 7 days NaM";
        response.whisperReceiver = message.user;
        return response;
    } else if (seconds < 0) {
        response.type = Response::Type::WHISPER;
        response.message = "Negative amount of time "
                           "\xF0\x9F\xA4\x94, sorry I can't travel "
                           "back in time (yet)",
        response.whisperReceiver = message.user;
        return response;
    }

    std::string reminderMessage = makeReminderMsg(tokens, inPos);
    if (reminderMessage.empty()) {
        reminderMessage = "No reminder";
    }

    std::string whichReminder =
        this->redisClient.addReminder(message.user, seconds, reminderMessage);

    auto t =
        std::make_shared<boost::asio::steady_timer>(ioService, std::chrono::seconds(seconds));
        
    auto whisperMessage =
        reminderMessage + " - " + makeTimeString(seconds) + " ago";
    auto remindFunction =
        [
          owner = this->channelObject->owner, user = message.user,
          whisperMessage, whichReminder, t
        ](const boost::system::error_code &er)
            ->void
    {
        if (er) {
            std::cerr << "Timer error: " << er << std::endl;
            return;
        }
        if (owner->channels.count(owner->nick) == 0) {
            owner->joinChannel(owner->nick);
        }

        owner->channels.at(owner->nick)->whisper(whisperMessage, user);
        owner->channels.at(owner->nick)
            ->commandsHandler.redisClient.removeReminder(user, whichReminder);
    };

    
    t->async_wait(remindFunction);

    // this->channelObject->owner->sanitizeMsg(reminderMessage);

    std::string msg = message.user + ", reminding you in " +
                      std::to_string(seconds) + " seconds (" + reminderMessage +
                      ") SeemsGood";

    response.type = Response::Type::MESSAGE;
    response.message = msg;
    return response;
}

Response
CommandsHandler::remind(const IRCMessage &message,
                        std::vector<std::string> &tokens)
{
    Response response(0);
    if (tokens.size() < 2) {
        response.type = Response::Type::WHISPER;
        response.whisperReceiver = message.user;
        response.message = "Usage \"!remind <who> [your message] in 20s "
                           "15h 10d 9m (the order does not matter, "
                           "the number must be immediately followed "
                           "by an identifier)\"";
        return response;
    }
    changeToLower(tokens[1]);
    std::string remindedUser = tokens[1];
    tokens.erase(tokens.begin() + 1);

    auto inPos = findLastIn(tokens);
    if (inPos == 0 || inPos + 1 >= tokens.size()) {
        response.type = Response::Type::WHISPER;
        response.whisperReceiver = message.user;
        response.message = "Usage \"!remind <who> [your message] in 20s "
                           "15h 10d 9m (the order does not matter, "
                           "the number must be immediately followed "
                           "by an identifier)\"";
        return response;
    }

    long long seconds = getSecondsCount(tokens, inPos);
    if (seconds == 0) {
        response.type = Response::Type::WHISPER;
        response.message = "Usage \"!remind <who> [your message] in 20s "
                           "15h 10d 9m (the order does not matter, "
                           "the number must be immediately followed "
                           "by an identifier)\"";
        response.whisperReceiver = message.user;
        return response;
    } else if (seconds > 604800) {
        response.type = Response::Type::WHISPER;
        response.message = "Maximum amount of time is 7 days NaM";
        response.whisperReceiver = message.user;
        return response;
    } else if (seconds < 0) {
        response.type = Response::Type::WHISPER;
        response.message = "Negative amount of time "
                           "\xF0\x9F\xA4\x94, sorry I can't travel "
                           "back in time (yet)";
        response.whisperReceiver = message.user;
        return response;
    }

    std::string reminderMessage =
        makeReminderMsg(tokens, inPos) + " - Reminder by " + message.user;
    if (reminderMessage.empty()) {
        reminderMessage = "Reminder by " + message.user;
    }

    std::string whichReminder =
        this->redisClient.addReminder(remindedUser, seconds, reminderMessage);

    int countOfReminders =
        this->channelObject->owner->userReminders.count(message.user);
    RemindUsers::Reminder pair;
    if (countOfReminders >= 2) {
        pair = this->channelObject->owner->userReminders.getFirst(message.user);
        this->channelObject->owner->userReminders.removeFirst(message.user);
        this->redisClient.removeReminder(pair.toUser, pair.whichReminder);
        pair.timer->cancel();
    }

    auto t =
        std::make_shared<boost::asio::steady_timer>(ioService, std::chrono::seconds(seconds));
    
    auto whisperMessage =
        reminderMessage + " " + makeTimeString(seconds) + " ago";
    auto remindFunction =
        [
          owner = this->channelObject->owner, user = remindedUser,
          whisperMessage, whichReminder, from = message.user, t
        ](const boost::system::error_code &er)
            ->void
    {
        if (er) {
            std::cerr << "Timer error: " << er << std::endl;
            return;
        }
        if (owner->channels.count(owner->nick) == 0) {
            owner->joinChannel(owner->nick);
        }
        owner->channels.at(owner->nick)->whisper(whisperMessage, user);
        owner->channels.at(owner->nick)
            ->commandsHandler.redisClient.removeReminder(user, whichReminder);
        owner->userReminders.remove(from, user, whichReminder);
    };

    
    t->async_wait(remindFunction);
    this->channelObject->owner->userReminders.addReminder(
        message.user, remindedUser, whichReminder, t);

    // this->channelObject->owner->sanitizeMsg(reminderMessage);
    std::string msg = message.user + ", reminding " + remindedUser + " in " +
                      std::to_string(seconds) + " seconds (" + reminderMessage +
                      ") SeemsGood";
    if (countOfReminders >= 2) {
        msg += " Also removed old reminder for " + pair.toUser;
    }

    response.type = Response::Type::MESSAGE;
    response.message = msg;
    return response;
}

Response
CommandsHandler::say([[maybe_unused]] const IRCMessage &message,
                     std::vector<std::string> &tokens)
{
    Response response(1);
    if (tokens.size() < 2) {
        return response;
    }

    tokens.erase(tokens.begin());
    for (const auto &i : tokens) {
        response.message += i + " ";
    }
    response.type = Response::Type::MESSAGE;
    return response;
}

Response
CommandsHandler::afk(const IRCMessage &message,
                     std::vector<std::string> &tokens)
{
    Response response(0);
    std::string msg;
    for (decltype(tokens.size()) i = 1; i < tokens.size(); ++i) {
        msg += tokens[i] + ' ';
    }

    if (msg.back() == ' ') {
        msg.pop_back();
    }

    if (!this->isAdmin(message.user) && msg.size() > 300) {
        msg.resize(300);
    }
    // this->channelObject->owner->sanitizeMsg(msg);

    this->channelObject->owner->afkers.setAfker(message.user, msg);

    response.type = Response::Type::MESSAGE;
    
    response.message = message.user + " is now afk";

    if (msg.size() != 0) {
        response.message += " - " + msg;
    }

    return response;
}

Response
CommandsHandler::shadowAfk(const IRCMessage &message,
                     std::vector<std::string> &tokens)
{
    Response response(0);
    std::string msg;
    for (decltype(tokens.size()) i = 1; i < tokens.size(); ++i) {
        msg += tokens[i] + ' ';
    }

    if (msg.back() == ' ') {
        msg.pop_back();
    }

    if (!this->isAdmin(message.user) && msg.size() > 300) {
        msg.resize(300);
    }
    // this->channelObject->owner->sanitizeMsg(msg);

    this->channelObject->owner->afkers.setShadowAfker(message.user, msg);
    
    response.type = Response::Type::FUNCTION;
    return response;
}

Response
CommandsHandler::goodNight(const IRCMessage &message,
                           std::vector<std::string> &tokens)
{
    Response response(0);

    if (tokens.size() > 1) {
        auto name = changeToLower_copy(tokens[1]);
        if (UserIDs::getInstance().isUser(name)) {
            auto str = Hemirt::getRaw("http://tmi.twitch.tv/group/user/" + message.channel + "/chatters");
            if (auto pos = str.find(name); pos != std::string::npos && str[pos-1] == '\"' && str[pos+name.size()] == '\"') {
                return response;
            }
        }
    }

    std::string gnmsg;
    if (tokens.size() > 1) {
        for (decltype(tokens.size()) i = 1; i < tokens.size(); i++) {
            gnmsg += tokens[i] + " ";
        }
    }
   

    if (gnmsg.empty()) {
        this->channelObject->owner->afkers.setAfker(message.user,
                                                    "ResidentSleeper");
    } else {
        gnmsg.pop_back();
        this->channelObject->owner->afkers.setAfker(message.user, gnmsg);
    }

    response.type = Response::Type::MESSAGE;
    response.message = message.user + " is now sleeping ResidentSleeper - " + gnmsg;
    if (gnmsg.empty()) {
        response.message.pop_back();
        response.message.pop_back();
        response.message.pop_back();
    }
    if (message.channel == "forsen" || message.channel == "nani") {
        boost::algorithm::replace_all(response.message, "ResidentSleeper", "zzz");
    }

    return response;
}

Response
CommandsHandler::isAfk(const IRCMessage &message,
                       std::vector<std::string> &tokens)
{
    Response response(0);
    if (tokens.size() < 2) {
        return response;
    }

    changeToLower(tokens[1]);
    if (this->channelObject->owner->isBlacklisted(tokens[1])) {
        return response;
    }

    auto afk = this->channelObject->owner->afkers.getAfker(tokens[1]);

    if (afk.exists) {
        auto now = std::chrono::system_clock::now();
        auto seconds =
            std::chrono::duration_cast<std::chrono::seconds>(now - afk.time)
                .count();
        std::string when = makeTimeString(seconds);
        if (when.size() == 0) {
            when = "0s";
        }
        response.message =
            message.user + ", " + tokens[1] + " went afk " + when + " ago";
        if (afk.message.size() != 0) {
            response.message += " - " + afk.message;
        }
    } else {
        response.message =
            message.user + ", user " + tokens[1] + " is not afk SeemsGood";
    }

    response.type = Response::Type::MESSAGE;
    return response;
}

Response
CommandsHandler::comeBackMsg(const IRCMessage &message,
                             std::vector<std::string> &tokens)
{
    Response response(0);
    if (tokens.size() < 3) {
        return response;
    }

    changeToLower(tokens[1]);
    
    if (!UserIDs::getInstance().isUser(tokens[1])) {
        return response;
    }
    
    if (tokens[1] == "nymn" && !this->isAdmin(message.user)) {
        return response;
    }
    std::string msg;
    if (!message.channel.empty()) {
        msg += message.channel.front();
        msg += "\xF3\xA0\x80\x80";
        msg += message.channel.substr(1);
    }
    msg += ") ";
    for (decltype(tokens.size()) i = 2; i < tokens.size(); ++i) {
        msg += tokens[i] + ' ';
    }

    if (msg.back() == ' ') {
        msg.pop_back();
    }
    
    bool admin = this->isAdmin(message.user);
    
    if (!admin && msg.size() > 150) {
        msg = msg.substr(0, 150);
    }
    
    

    int added = this->channelObject->owner->comebacks.makeComeBackMsg(message.user,
                                                          tokens[1], msg, admin);

    response.type = Response::Type::MESSAGE;
    if (added == 0) {
        response.message =
            message.user + ", the user " + tokens[1] +
            " will get your message once he writes anything in chat SeemsGood";
    } else if (added == 1) {
        response.message = message.user + ", you've already set enough messages for " + tokens[1];
    } else if (added == 2) {
        response.message = message.user + ", the user " + tokens[1] + " has already enough of notifications PajaWLookingOutOfHisMemePortal ";
    }
    
    return response;
}

Response
CommandsHandler::addBlacklist(const IRCMessage &message,
                              std::vector<std::string> &tokens)
{
    Response response(1);
    if (tokens.size() == 2) {
        tokens.push_back("*");
    } else if (tokens.size() < 3) {
        return response;
    }

    changeToLower(tokens[1]);

    this->channelObject->owner->addBlacklist(tokens[1], tokens[2]);

    response.type = Response::Type::MESSAGE;
    response.message =
        message.user + ", added blacklist for " + tokens[1] + " SeemsGood";
    return response;
}

Response
CommandsHandler::removeBlacklist(const IRCMessage &message,
                                 std::vector<std::string> &tokens)
{
    Response response(1);
    if (tokens.size() < 2) {
        return response;
    }

    changeToLower(tokens[1]);

    this->channelObject->owner->removeBlacklist(tokens[1]);
    response.type = Response::Type::MESSAGE;
    response.message =
        message.user + ", removed blacklist for " + tokens[1] + " SeemsGood";
    return response;
}

Response
CommandsHandler::whoIsAfk(const IRCMessage &message, [[maybe_unused]] std::vector<std::string> &tokens)
{
    Response response(1);
    if (message.user != "hemirt") {
        return response;
    }
    
    if (!this->cooldownCheck("", message.user, "!whoisafk", message.channel)) {
        return response;
    }
    
    std::string afkers = this->channelObject->owner->afkers.getAfkers();
    if (afkers.empty()) {
        response.message =
            message.user + ", right now there is nobody afk SeemsGood";
    } else {
        response.message = message.user + ", list of afk users: " + afkers;
    }
    response.type = Response::Type::MESSAGE;
    return response;
}

Response
CommandsHandler::regexTest(const IRCMessage &message,
                           std::vector<std::string> &tokens)
{
    Response response(0);
    if (!this->isAdmin(message.user)) {
        return response;
    }

    tokens.erase(tokens.begin());

    if (tokens.size() == 0) {
        return response;
    }

    std::string joined = boost::algorithm::join(tokens, " ");

    boost::smatch match;
    boost::regex expression("{b(:\\s?(\\d+))?}");
    if (boost::regex_search(joined.cbegin(), joined.cend(), match,
                            expression)) {
        std::cout << "size: " << match.size();
        std::cout << "\nmaxsize: " << match.max_size() << std::endl;
        for (const auto &i : match) {
            std::cout << i << "\nmatched: " << i.matched
                      << "\nlength: " << i.length() << "\nstr: " << i.str()
                      << std::endl;
        }
    }
    return response;
}

Response
CommandsHandler::setUserCountryFrom(const IRCMessage &message,
                                    std::vector<std::string> &tokens)
{
    Response response(1);
    if (tokens.size() < 3) {
        return response;
    }

    std::string country;
    for (decltype(tokens.size()) i = 2; i < tokens.size(); i++) {
        country += tokens[i] + " ";
    }
    if (country.back() == ' ') {
        country.pop_back();
    }

    auto result = Countries::getInstance().setFrom(tokens[1], country);
    if (result == Countries::Result::SUCCESS) {
        response.message = message.user + ", set the country where " +
                           tokens[1] + " comes from to " + country +
                           " SeemsGood";
        response.type = Response::Type::MESSAGE;
    } else if (result == Countries::Result::NOCOUNTRY) {
        response.message = message.user + ", you cannot use this country NaM";
        response.type = Response::Type::MESSAGE;
    }
    return response;
}

Response
CommandsHandler::setUserCountryLive(const IRCMessage &message,
                                    std::vector<std::string> &tokens)
{
    Response response(1);
    if (tokens.size() < 3) {
        return response;
    }

    std::string country;
    for (decltype(tokens.size()) i = 2; i < tokens.size(); i++) {
        country += tokens[i] + " ";
    }
    if (country.back() == ' ') {
        country.pop_back();
    }

    auto result = Countries::getInstance().setLive(tokens[1], country);
    response.type = Response::Type::MESSAGE;
    if (result == Countries::Result::SUCCESS) {
        response.message = message.user + ", set the country where " +
                           tokens[1] + " lives to " + country + " SeemsGood";
        response.type = Response::Type::MESSAGE;
    } else if (result == Countries::Result::NOCOUNTRY) {
        response.message = message.user + ", you cannot use this country NaM";
        response.type = Response::Type::MESSAGE;
    }
    return response;
}

Response
CommandsHandler::isFrom(const IRCMessage &message,
                        std::vector<std::string> &tokens)
{
    Response response(0);
    if (tokens.size() < 2) {
        return response;
    }

    auto from = Countries::getInstance().getFrom(tokens[1]);
    auto live = Countries::getInstance().getLive(tokens[1]);
    if (from.empty() && live.empty()) {
        response.message =
            message.user + ", there are no data about " + tokens[1] + " eShrug";
    } else if (!from.empty() && !live.empty() && live != from) {
        response.message = message.user + ", " + tokens[1] + " comes from " +
                           from + " and lives in " + live + " SeemsGood";
    } else if (live.empty()) {
        response.message = message.user + ", " + tokens[1] + " comes from " +
                           from + " SeemsGood";
    } else if (from.empty()) {
        response.message = message.user + ", " + tokens[1] + " lives in " +
                           live + " SeemsGood";
    } else if (live == from) {
        response.message = message.user + ", " + tokens[1] +
                           " comes from and lives in " + live + " SeemsGood";
    }

    response.type = Response::Type::MESSAGE;
    return response;
}

Response
CommandsHandler::getUsersFrom(const IRCMessage &message,
                              std::vector<std::string> &tokens)
{
    Response response(0);
    if (tokens.size() < 2) {
        return response;
    }
    
    if (!this->cooldownCheck("", message.user, tokens[0], message.channel)) {
        return response;
    }

    std::string country;
    for (decltype(tokens.size()) i = 1; i < tokens.size(); i++) {
        country += tokens[i] + " ";
    }
    if (country.back() == ' ') {
        country.pop_back();
    }

    auto pairDisplayUsers = Countries::getInstance().usersFrom(country);
    if (pairDisplayUsers.first.empty()) {
        response.message =
            message.user + ", no such country found in the database NaM";
    } else if (pairDisplayUsers.second.empty()) {
        response.message = message.user + ", there are no users from " +
                           pairDisplayUsers.first + " ForeverAlone";
    } else {
        response.message = message.user + ", from " + pairDisplayUsers.first +
                           " are these users: ";
        for (auto &i : pairDisplayUsers.second) {
            // insert special character at pos 1
            // and at pos before last
            i.insert(1, std::string("\xF3\xA0\x80\x80"));
            i.insert(i.size() - 1, std::string("\xF3\xA0\x80\x80"));
            response.message += i + ", ";
        }
        response.message.pop_back();
        response.message.pop_back();
        response.message += " SeemsGood";
    }

    response.type = Response::Type::MESSAGE;
    return response;
}

Response
CommandsHandler::getUsersLiving(const IRCMessage &message,
                                std::vector<std::string> &tokens)
{
    Response response(0);
    if (tokens.size() < 2) {
        return response;
    }
    
    if (!this->cooldownCheck("", message.user, tokens[0], message.channel)) {
        return response;
    }

    std::string country;
    for (decltype(tokens.size()) i = 1; i < tokens.size(); i++) {
        country += tokens[i] + " ";
    }
    if (country.back() == ' ') {
        country.pop_back();
    }

    auto pairDisplayUsers = Countries::getInstance().usersLive(country);
    if (pairDisplayUsers.first.empty()) {
        response.message =
            message.user + ", no such country found in the database NaM";
    } else if (pairDisplayUsers.second.empty()) {
        response.message = message.user + ", there are no users living in " +
                           pairDisplayUsers.first + " ForeverAlone";
    } else {
        response.message =
            message.user + ", in " + pairDisplayUsers.first + " live users: ";
        for (auto &i : pairDisplayUsers.second) {
            i.insert(1, std::string("\xF3\xA0\x80\x80"));
            i.insert(i.size() - 1, std::string("\xF3\xA0\x80\x80"));
            response.message += i + ", ";
        }
        response.message.pop_back();
        response.message.pop_back();
        response.message += " SeemsGood";
    }
    response.type = Response::Type::MESSAGE;
    return response;
}

Response
CommandsHandler::deleteUser(const IRCMessage &message,
                            std::vector<std::string> &tokens)
{
    Response response(1);
    if (tokens.size() < 2) {
        return response;
    }

    Countries::getInstance().deleteFrom(tokens[1]);
    Countries::getInstance().deleteLive(tokens[1]);
    response.message =
        message.user + ", deleted user " + tokens[1] + " SeemsGood";
    response.type = Response::Type::MESSAGE;
    return response;
}

Response
CommandsHandler::myFrom(const IRCMessage &message,
                        std::vector<std::string> &tokens)
{
    Response response(0);
    if (tokens.size() < 2) {
        return response;
    }

    std::string country;
    for (decltype(tokens.size()) i = 1; i < tokens.size(); i++) {
        country += tokens[i] + " ";
    }
    if (country.back() == ' ') {
        country.pop_back();
    }

    auto result = Countries::getInstance().setFrom(message.user, country);
    if (result == Countries::Result::SUCCESS) {
        response.message = message.user + ", set the country you're from to " +
                           country + " SeemsGood";
        response.type = Response::Type::MESSAGE;
    } else if (result == Countries::Result::NOCOUNTRY) {
        response.message =
            message.user + ", no such country found in the database NaM";
        response.type = Response::Type::MESSAGE;
    }

    return response;
}

Response
CommandsHandler::myLiving(const IRCMessage &message,
                          std::vector<std::string> &tokens)
{
    Response response(0);
    if (tokens.size() < 2) {
        return response;
    }

    std::string country;
    for (decltype(tokens.size()) i = 1; i < tokens.size(); i++) {
        country += tokens[i] + " ";
    }
    if (country.back() == ' ') {
        country.pop_back();
    }

    auto result = Countries::getInstance().setLive(message.user, country);
    if (result == Countries::Result::SUCCESS) {
        response.message = message.user + ", the country you live was set to " +
                           country + " SeemsGood";
        response.type = Response::Type::MESSAGE;
    } else if (result == Countries::Result::NOCOUNTRY) {
        response.message =
            message.user + ", no such country found in the database NaM";
        response.type = Response::Type::MESSAGE;
    }

    return response;
}

Response
CommandsHandler::myDelete(const IRCMessage &message,
                          std::vector<std::string> &tokens)
{
    Response response(0);

    if (tokens.size() < 2) {
        return response;
    }

    if (tokens[1] == "from") {
        Countries::getInstance().deleteFrom(message.user);
    } else if (tokens[1] == "live") {
        Countries::getInstance().deleteLive(message.user);
    }

    response.message = message.user + ", deleted your data SeemsGood";
    response.type = Response::Type::MESSAGE;
    return response;
}

Response
CommandsHandler::createCountry(const IRCMessage &message,
                               std::vector<std::string> &tokens)
{
    Response response(1);

    if (tokens.size() < 2) {
        return response;
    }

    std::string country;
    for (decltype(tokens.size()) i = 1; i < tokens.size(); i++) {
        country += tokens[i] + " ";
    }
    if (country.back() == ' ') {
        country.pop_back();
    }

    auto str = Countries::getInstance().createCountry(country);

    response.message = message.user + ", created new country \"" + country +
                       "\" (" + str + ") SeemsGood";
    response.type = Response::Type::MESSAGE;
    return response;
}

Response
CommandsHandler::deleteCountry(const IRCMessage &message,
                               std::vector<std::string> &tokens)
{
    Response response(1);

    if (tokens.size() < 2) {
        return response;
    }

    if (Countries::getInstance().deleteCountry(tokens[1])) {
        response.message =
            message.user + ", deleted country " + tokens[1] + " SeemsGood";
        response.type = Response::Type::MESSAGE;
    }

    return response;
}

Response
CommandsHandler::renameCountry(const IRCMessage &message,
                               std::vector<std::string> &tokens)
{
    Response response(1);

    if (tokens.size() < 3) {
        return response;
    }

    std::string country;
    for (decltype(tokens.size()) i = 2; i < tokens.size(); i++) {
        country += tokens[i] + " ";
    }
    if (country.back() == ' ') {
        country.pop_back();
    }

    if (Countries::getInstance().renameCountry(tokens[1], country)) {
        response.message = message.user + ", renamed country " + tokens[1] +
                           " to " + country + " SeemsGood";
        response.type = Response::Type::MESSAGE;
    }

    return response;
}

Response
CommandsHandler::getCountryID(const IRCMessage &message,
                              std::vector<std::string> &tokens)
{
    Response response(1);

    if (tokens.size() < 2) {
        return response;
    }

    std::string country;
    for (decltype(tokens.size()) i = 1; i < tokens.size(); i++) {
        country += tokens[i] + " ";
    }
    if (country.back() == ' ') {
        country.pop_back();
    }

    auto id = Countries::getInstance().getCountryID(country);
    if (!id.empty()) {
        response.message = message.user + ", countryID of \"" + country +
                           "\" is " + id + " SeemsGood";
    } else {
        response.message =
            message.user + ", country \"" + country + "\" not found NaM";
    }
    response.type = Response::Type::MESSAGE;
    return response;
}

Response
CommandsHandler::createAlias(const IRCMessage &message,
                             std::vector<std::string> &tokens)
{
    Response response(1);

    if (tokens.size() < 3) {
        return response;
    }

    std::string country;
    for (decltype(tokens.size()) i = 2; i < tokens.size(); i++) {
        country += tokens[i] + " ";
    }
    if (country.back() == ' ') {
        country.pop_back();
    }

    if (Countries::getInstance().createAlias(tokens[1], country)) {
        response.message = message.user + ", created alias for " + tokens[1] +
                           ": " + country + " SeemsGood";
    } else {
        response.message =
            message.user + ", country " + tokens[1] + " not found NaM";
    }
    response.type = Response::Type::MESSAGE;
    return response;
}

Response
CommandsHandler::deleteAlias(const IRCMessage &message,
                             std::vector<std::string> &tokens)
{
    Response response(1);

    if (tokens.size() < 3) {
        return response;
    }

    std::string country;
    for (decltype(tokens.size()) i = 2; i < tokens.size(); i++) {
        country += tokens[i] + " ";
    }
    if (country.back() == ' ') {
        country.pop_back();
    }

    if (Countries::getInstance().deleteAlias(tokens[1], country)) {
        response.message = message.user + ", deleted alias for " + tokens[1] +
                           ": " + country + " SeemsGood";
    } else {
        response.message =
            message.user + ", country " + tokens[1] + " not found NaM";
    }
    response.type = Response::Type::MESSAGE;
    return response;
}

Response
CommandsHandler::myReminders(const IRCMessage &message,
                             [[maybe_unused]] std::vector<std::string> &tokens)
{
    Response response(0);

    auto remindTree = this->redisClient.getRemindersOfUser(message.user);
    std::string whichReminders;
    for (const auto &kv : remindTree) {
        // kv.first == name of the child
        // kv.second == child tree
        whichReminders += kv.first + ", ";
    }

    if (whichReminders.empty()) {
        response.message =
            message.user + ", there are no reminders for you eShrug";
    } else {
        whichReminders.pop_back();
        whichReminders.pop_back();
        response.message =
            message.user + ", there are these reminders for you: " +
            whichReminders + ". Use !reminder number to check it out SeemsGood";
    }

    response.type = Response::Type::MESSAGE;
    return response;
}

Response
CommandsHandler::checkReminder(const IRCMessage &message,
                               std::vector<std::string> &tokens)
{
    Response response(0);
    if (tokens.size() < 2) {
        return response;
    }
    
    if (!this->cooldownCheck("", message.user, tokens[0], message.channel)) {
        return response;
    }

    auto reminderTree = this->redisClient.getRemindersOfUser(message.user);

    if (reminderTree.count(tokens[1])) {
        std::string what = reminderTree.get(tokens[1] + ".what", "");
        std::string timeSinceEpoch = reminderTree.get(tokens[1] + ".when", "");
        if (what.empty() || timeSinceEpoch.empty()) {
            return response;
        }
        int64_t sec = std::atoll(timeSinceEpoch.c_str());
        auto whentimepoint = std::chrono::time_point<std::chrono::system_clock>(
            std::chrono::seconds(sec));
        auto dur = whentimepoint - std::chrono::system_clock::now();
        std::string msg = makeTimeString(
            std::chrono::duration_cast<std::chrono::seconds>(dur).count());
        response.message = message.user + ", reminder " + tokens[1] + ": " +
                           what + " in " + msg + " SeemsGood";
    } else {
        response.message = message.user + ", no such reminder NaM";
    }

    response.type = Response::Type::MESSAGE;
    return response;
}

Response
CommandsHandler::pingMeCommand(const IRCMessage &message,
                               std::vector<std::string> &tokens)
{
    Response response(0);
    if (tokens.size() < 2) {
        return response;
    }

    changeToLower(tokens[1]);

    if (!UserIDs::getInstance().isUser(tokens[1])) {
        return response;
    }

    this->channelObject->pingMe.addPing(message.user, tokens[1],
                                        this->channelObject->channelName);
    response.message = message.user + ", you will be pinged when " + tokens[1] +
                       " writes something in chat SeemsGood";
    response.type = Response::Type::MESSAGE;
    return response;
}

Response
CommandsHandler::randomIslamicQuote(const IRCMessage &message,
                                    std::vector<std::string> &tokens)
{
    Response response(-1);

    if (!this->cooldownCheck("", message.user, tokens[0], message.channel)) {
        return response;
    }

    std::string str;
    if (tokens.size() < 2) {
        str = Ayah::getRandomAyah();
    } else {
        int number = std::atoi(tokens[1].c_str());
        if (number == 0) {
            str = Ayah::getRandomAyah();
        } else {
            str = Ayah::getAyah(number);
        }
    }

    if (str.empty()) {
        return response;
    }

    response.message = str;
    response.type = Response::Type::MESSAGE;
    return response;
}

Response
CommandsHandler::gdpr(const IRCMessage &message,
                                    std::vector<std::string> &tokens)
{
    Response response(1);

    if (!this->cooldownCheck("", message.user, tokens[0], message.channel, 120)) {
        return response;
    }

    response.message = "Any info you put in the bot via commands is entirely optional and everyone else can view this information via provided commands. Use !purgeme command to delete all your info. We also store data that identifies your Twitch account and can track you across the Twitch service (nickname, userid, displayname, et cetera). If you do not want to be part of this, write an email to gdpr@hemirt(dot)com and you will be removed afterwards (you will also lose access to this service).";
    response.type = Response::Type::MESSAGE;
    return response;
}

Response
CommandsHandler::randomChristianQuote(const IRCMessage &message,
                                      std::vector<std::string> &tokens)
{
    Response response(-1);

    if (!this->cooldownCheck("", message.user, tokens[0], message.channel)) {
        return response;
    }

    std::string str = Bible::getRandomVerse();

    if (str.empty()) {
        return response;
    }

    response.message = str;
    response.type = Response::Type::MESSAGE;
    return response;
}

Response
CommandsHandler::createModule(const IRCMessage &message,
                 std::vector<std::string> &tokens)
{
    Response response(1);
    if (tokens.size() < 2) {
        return response;
    }
    changeToLower(tokens[1]);
    
    response.message = message.user + ", " + std::to_string(this->channelObject->owner->modulesManager.createModule(tokens[1]));
    response.type = Response::Type::MESSAGE;
    return response;
}

Response
CommandsHandler::setModuleType(const IRCMessage &message,
                 std::vector<std::string> &tokens)
{
    Response response(1);
    if (tokens.size() < 3) {
        return response;
    }
    changeToLower(tokens[1]);
    changeToLower(tokens[2]);
    
    response.message = message.user + ", " + std::to_string(this->channelObject->owner->modulesManager.setType(tokens[1], tokens[2]));
    response.type = Response::Type::MESSAGE;
    return response;
}

Response
CommandsHandler::setModuleSubtype(const IRCMessage &message,
                 std::vector<std::string> &tokens)
{
    Response response(1);
    if (tokens.size() < 3) {
        return response;
    }
    changeToLower(tokens[1]);
    changeToLower(tokens[2]);
    
    response.message = message.user + ", " + std::to_string(this->channelObject->owner->modulesManager.setSubtype(tokens[1], tokens[2]));
    response.type = Response::Type::MESSAGE;
    return response;
}

Response
CommandsHandler::setModuleName(const IRCMessage &message,
                 std::vector<std::string> &tokens)
{
    Response response(1);
    if (tokens.size() < 3) {
        return response;
    }
    changeToLower(tokens[1]);
    changeToLower(tokens[2]);
    
    response.message = message.user + ", " + std::to_string(this->channelObject->owner->modulesManager.setName(tokens[1], tokens[2]));
    response.type = Response::Type::MESSAGE;
    return response;
}

Response
CommandsHandler::setModuleFormat(const IRCMessage &message,
                 std::vector<std::string> &tokens) 
{
    Response response(1);
    if (tokens.size() < 3) {
        return response;
    }
    changeToLower(tokens[1]);
    
    std::string valueString;
    for (size_t i = 2; i < tokens.size(); ++i) {
        valueString += tokens[i] + ' ';
    }
    valueString.pop_back();
    
    response.message = message.user + ", " + std::to_string(this->channelObject->owner->modulesManager.setFormat(tokens[1], valueString));
    response.type = Response::Type::MESSAGE;
    return response;
}

Response
CommandsHandler::setModuleStatus(const IRCMessage &message,
                 std::vector<std::string> &tokens)
{
    Response response(1);
    if (tokens.size() < 3) {
        return response;
    }
    changeToLower(tokens[1]);
    changeToLower(tokens[2]);
    
    response.message = message.user + ", " + std::to_string(this->channelObject->owner->modulesManager.setStatus(tokens[1], tokens[2]));
    response.type = Response::Type::MESSAGE;
    return response;
}

Response
CommandsHandler::getModuleInfo(const IRCMessage &message,
                 std::vector<std::string> &tokens)
{
    Response response(0);
    if (tokens.size() < 2) {
        return response;
    }
    changeToLower(tokens[1]);
    
    response.message = message.user + ", " + this->channelObject->owner->modulesManager.getInfo(tokens[1]);
    response.type = Response::Type::MESSAGE;
    return response;
}

Response
CommandsHandler::getUserData(const IRCMessage &message,
                 std::vector<std::string> &tokens)
{
    Response response(0);
    if (!this->cooldownCheck("", message.user, tokens[0], message.channel)) {
        return response;
    }
    if (tokens.size() < 3) {
        if (tokens.size() == 1) {
            auto from = Countries::getInstance().getFrom(message.user);
            auto live = Countries::getInstance().getLive(message.user);
            response.message = this->channelObject->owner->modulesManager.getAllData(message.user);
            if (!from.empty() && !live.empty() && from == live) {
                response.message += " from&live: " + from;
            } else {
                if (!from.empty()) {
                response.message += " from: " + from;
                }
                if (!live.empty()) {
                    response.message += " live: " + live;
                }
            }
            response.type = Response::Type::MESSAGE;
            
        } else if (tokens.size() == 2) {
            changeToLower(tokens[1]);
            auto from = Countries::getInstance().getFrom(tokens[1]);
            auto live = Countries::getInstance().getLive(tokens[1]);
            response.message = this->channelObject->owner->modulesManager.getAllData(tokens[1]);
            if (!from.empty() && !live.empty() && from == live) {
                response.message += " from&live: " + from;
            } else {
                if (!from.empty()) {
                response.message += " from: " + from;
                }
                if (!live.empty()) {
                    response.message += " live: " + live;
                }
            }
            response.type = Response::Type::MESSAGE;
        }
        return response;
    }
    changeToLower(tokens[1]);
    changeToLower(tokens[2]);
    
    // !data user module
    auto p = this->channelObject->owner->modulesManager.getData(tokens[1], tokens[2]);
    if (p.first == true) {
        response.message = message.user + ", " + p.second;
    } else {
        p = this->channelObject->owner->modulesManager.getData(tokens[2], tokens[1]);
        if (p.first == true) {
            response.message = message.user + ", " + p.second;
        } else if (UserIDs::getInstance().isUser(tokens[1])) {
            auto from = Countries::getInstance().getFrom(tokens[1]);
            auto live = Countries::getInstance().getLive(tokens[1]);
            response.message = this->channelObject->owner->modulesManager.getAllData(tokens[1]);
            if (!from.empty() && !live.empty() && from == live) {
                response.message += " from&live: " + from;
            } else {
                if (!from.empty()) {
                response.message += " from: " + from;
                }
                if (!live.empty()) {
                    response.message += " live: " + live;
                }
            }
        }
    }
    
    boost::algorithm::replace_all(response.message, "{time}", localTime());
    boost::algorithm::replace_all(response.message, "{date}", localDate());
    replaceTodayDates(response.message);

     
    response.type = Response::Type::MESSAGE;
    return response;
}

Response
CommandsHandler::deleteMyData(const IRCMessage &message,
                 std::vector<std::string> &tokens)
{
    Response response(0);
    if (tokens.size() < 2) {
        return response;
    }
    changeToLower(tokens[1]);

    response.message = message.user + ", " + this->channelObject->owner->modulesManager.deleteData(message.user, tokens[1]);
    response.type = Response::Type::MESSAGE;
    return response;
}


Response
CommandsHandler::deleteUserData(const IRCMessage &message,
                 std::vector<std::string> &tokens)
{
    Response response(1);
    if (tokens.size() < 3) {
        return response;
    }
    changeToLower(tokens[1]);
    changeToLower(tokens[2]);

    response.message = message.user + ", " + this->channelObject->owner->modulesManager.deleteData(tokens[2], tokens[1]);
    response.type = Response::Type::MESSAGE;
    return response;
}

Response
CommandsHandler::deleteUserDataFull(const IRCMessage &message,
                 std::vector<std::string> &tokens)
{
    Response response(1);
    if (tokens.size() < 2) {
        return response;
    }
    changeToLower(tokens[1]);
    
    Countries::getInstance().deleteFrom(tokens[1]);
    Countries::getInstance().deleteLive(tokens[1]);
    
    response.message = message.user + ", " + this->channelObject->owner->modulesManager.deleteDataFull(tokens[1]);
    response.type = Response::Type::MESSAGE;
    return response;
}

Response
CommandsHandler::purgeMe(const IRCMessage &message,
                 std::vector<std::string> &tokens)
{
    Response response(1);
    
    Countries::getInstance().deleteFrom(message.user);
    Countries::getInstance().deleteLive(message.user);

    auto &time = Time::getInstance();
    time.delTimeZone(message.userid);
    
    response.message = message.user + ", " + this->channelObject->owner->modulesManager.deleteDataFull(message.user) + ", country data was deleted, time data was deleted";
    
    response.type = Response::Type::MESSAGE;
    return response;
}

Response
CommandsHandler::purge(const IRCMessage &message,
                 std::vector<std::string> &tokens)
{
    Response response(1);
    if (!this->isAdmin(message.user)) {
        return response;
    }
    
    response.message = message.user + ", ";
    
    if (tokens.size() < 2) {
        response.type = Response::Type::WHISPER;
        response.whisperReceiver = message.user;
        response.message += "not enough parameters";
        return response;
    }
    
    changeToLower(tokens[1]);
    
    if (!UserIDs::getInstance().isUser(tokens[1])) {
        response.type = Response::Type::WHISPER;
        response.whisperReceiver = message.user;
        response.message += "\"" + tokens[1] + "\" is not an user";
        return response;
    }
    
    Countries::getInstance().deleteFrom(tokens[1]);
    Countries::getInstance().deleteLive(tokens[1]);
    
    auto id = UserIDs::getInstance().getID(tokens[1]);
    if (id == "error") {
        response.message += "Error getting userid, TimeZone data not deleted";
    } else {
        auto &time = Time::getInstance();
        time.delTimeZone(id);
        response.message += "TimeZone data deleted";
    }
    
    response.message += ", " + this->channelObject->owner->modulesManager.deleteDataFull(tokens[1]) + ", country data was deleted";
    
    response.type = Response::Type::MESSAGE;
    return response;
}

Response
CommandsHandler::getRandomQuote(const IRCMessage &message,
                 std::vector<std::string> &tokens)
{
    Response response(-1);
    if (message.channel == "pajlada")
        return response;
    
    if (this->channelObject->channelName == "forsen" && !this->isAdmin(message.user)) {
        return response;
    }
    
    if (!this->cooldownCheck("", message.user, tokens[0], message.channel)) {
        return response;
    }
    
    if (tokens.size() > 1 && UserIDs::getInstance().isUser(tokens[1])) {
        changeToLower(tokens[1]);
        response.message += tokens[1] + ": ";
        auto msg = RandomQuote::getRandomQuote(this->channelObject->channelName, tokens[1]);
        response.message += msg;
        if (msg.empty()) {
            msg += RandomQuote::getRandomQuote(this->channelObject->channelName, message.user);
            response.message += msg;
            if (msg.empty()) {
                return response;
            }
        }
        this->channelObject->owner->sanitizeMsg(response.message);
        response.type = Response::Type::MESSAGE;
        return response;
    } else {
        response.message += message.user + ": ";
        auto msg = RandomQuote::getRandomQuote(this->channelObject->channelName, message.user);
        response.message += msg;
        if (msg.empty()) {
                return response;
        }
        this->channelObject->owner->sanitizeMsg(response.message);
        response.type = Response::Type::MESSAGE;
        return response;
    }
}

Response
CommandsHandler::ignore(const IRCMessage &message,
                 std::vector<std::string> &tokens)
{
    Response response(1);
    if (tokens.size() > 1 && UserIDs::getInstance().isUser(tokens[1]) && !this->isAdmin(tokens[1])) {
        changeToLower(tokens[1]);
        this->channelObject->ignore.addUser(tokens[1]);
        response.message = message.user + ", ignored user " + tokens[1] + " NaM";
        response.type = Response::Type::MESSAGE;
        return response;
    }
    return response;
}

Response
CommandsHandler::unignore(const IRCMessage &message,
                 std::vector<std::string> &tokens)
{
    Response response(1);
    if (tokens.size() > 1 && UserIDs::getInstance().isUser(tokens[1]) && !this->isAdmin(tokens[1])) {
        changeToLower(tokens[1]);
        this->channelObject->ignore.removeUser(tokens[1]);
        response.message = message.user + ", unignored user " + tokens[1] + " NaM";
        response.type = Response::Type::MESSAGE;
        return response;
    }
    return response;
}

Response
CommandsHandler::saveModule(const IRCMessage &message,
                 std::vector<std::string> &tokens)
{
    Response response(1);
    if (tokens.size() < 1) {
        return response;
    }
    changeToLower(tokens[1]);
    
    if (this->channelObject->owner->modulesManager.saveModule(tokens[1])) {
        response.message = message.user + ", saved module " + tokens[1];
    } else {
        response.message = message.user + ", did not save module " + tokens[1] + " (does it exist?)";
    }
    response.type = Response::Type::MESSAGE;
    return response;
}

Response
CommandsHandler::deleteModule(const IRCMessage &message,
                 std::vector<std::string> &tokens)
{
    Response response(1);
    if (tokens.size() < 1) {
        return response;
    }
    changeToLower(tokens[1]);
    
    this->channelObject->owner->modulesManager.deleteModule(tokens[1]);
    response.message = message.user + ", deleted module " + tokens[1];
    response.type = Response::Type::MESSAGE;
    return response;
}

Response
CommandsHandler::setData(const IRCMessage &message,
                 std::vector<std::string> &tokens)
{
    Response response(0);
    if (tokens.size() < 3) {
        return response;
    }
    changeToLower(tokens[1]);
    if (tokens[1] == "from") {
        tokens.erase(tokens.begin() + 1);
        return this->myFrom(message, tokens);
    } else if (tokens[1] == "live") {
        tokens.erase(tokens.begin() + 1);
        return this->myLiving(message, tokens);
    }
    std::string valueString;
    for (size_t i = 2; i < tokens.size(); ++i) {
        if (UserIDs::getInstance().isUser(tokens[i]) && !isEmote(tokens[i])) {
            auto str = tokens[i];
            if (str.size() > 1) {
                str.insert(1, "\xF3\xA0\x80\x80");
            }
            tokens[i] = str;
        }
        valueString += tokens[i] + ' ';
        if (valueString.size() > 30) {
            break;
        }
    }
    valueString.pop_back();
    valueString = valueString.substr(0, 30);

    response.message = message.user + ", " + this->channelObject->owner->modulesManager.setData(message.user, tokens[1], valueString);
    response.type = Response::Type::MESSAGE;
    return response;
}

Response
CommandsHandler::setDataAdmin(const IRCMessage &message,
                 std::vector<std::string> &tokens)
{
    Response response(1);
    if (tokens.size() < 4) {
        return response;
    }
    changeToLower(tokens[1]);
    changeToLower(tokens[2]);
    if(!UserIDs::getInstance().isUser(tokens[2])) {
        return response;
    }
    std::string valueString;
    for (size_t i = 3; i < tokens.size(); ++i) {
        valueString += tokens[i] + ' ';
    }
    valueString.pop_back();

    response.message = message.user + ", " + this->channelObject->owner->modulesManager.setData(tokens[2], tokens[1], valueString);
    response.type = Response::Type::MESSAGE;
    return response;
}

Response
CommandsHandler::modules(const IRCMessage &message,
                 [[maybe_unused]] std::vector<std::string> &tokens)
{
    Response response(0);

    response.message = message.user + ", " + this->channelObject->owner->modulesManager.getAllModules();
    response.type = Response::Type::MESSAGE;
    return response;
}

bool
CommandsHandler::cooldownCheck(const std::string &path, const std::string &user, const std::string &cmd, const std::string &channel, int howmuch)
{
    if (!this->isAdmin(user)) {
        std::lock_guard<std::mutex> lk(this->cooldownsMtx);
        auto it = this->cooldownsMap.find(path + ":" + cmd + ":" + user + ":" + channel);
        auto now = std::chrono::steady_clock::now();
        if (it != cooldownsMap.end()) {
            if (std::chrono::duration_cast<std::chrono::seconds>(now -
                                                                 it->second)
                    .count() < howmuch) {
                return false;
            }
        }
        cooldownsMap[path + ":" + cmd + ":" + user + ":" + channel] = now;
        auto t =
            std::make_shared<boost::asio::steady_timer>(ioService, std::chrono::seconds(howmuch));
        t->async_wait([ this, key = path + ":" + cmd + ":" + user + ":" + channel, t ](
            [[maybe_unused]] const boost::system::error_code &er) {
            std::lock_guard<std::mutex> lk(this->cooldownsMtx);
            this->cooldownsMap.erase(key);
        });
        return true;
    } else {
        return true;
    }
}

bool
CommandsHandler::softCooldownCheck(const std::string &path, const std::string &user, const std::string &cmd, const std::string &channel, int howmuch)
{
    std::lock_guard<std::mutex> lk(this->cooldownsMtx);
    auto it = this->cooldownsMap.find(path + ":" + cmd + ":" + user + ":" + channel);
    auto now = std::chrono::steady_clock::now();
    if (it != cooldownsMap.end()) {
        if (std::chrono::duration_cast<std::chrono::seconds>(now -
                                                             it->second)
                .count() < howmuch) {
            return false;
        }
    }
    return true;
}

Response
CommandsHandler::clearQueue([[maybe_unused]] const IRCMessage &message,
                 [[maybe_unused]] std::vector<std::string> &tokens)
{
    Response response(0);
    this->channelObject->messenger.clearQueue();
    return response;
}

Response
CommandsHandler::reconnectAllChannels(const IRCMessage &message,
                               [[maybe_unused]] std::vector<std::string> &tokens)
{
    Response response(1);

    this->channelObject->owner->reconnectAllChannels(message.channel);
    response.message = "Reconnected all channels";
    response.type = Response::Type::MESSAGE;
    return response;
}

Response
CommandsHandler::test0(const IRCMessage &message,
                               [[maybe_unused]] std::vector<std::string> &tokens)
{
    Response response(0);
    
    if (tokens.size() <= 1) {
        AccCheck::ageByUsername(message.user);
        AccCheck::ageById(message.userid);
        AccCheck::ageByUsername<std::chrono::minutes>(message.user);
        AccCheck::ageById<std::chrono::minutes>(message.userid);
        AccCheck::ageByUsername<std::chrono::hours>(message.user);
        AccCheck::ageById<std::chrono::hours>(message.userid);
        AccCheck::ageByUsername<std::chrono::duration<std::int64_t, std::ratio<86400>>>(message.user);
        AccCheck::ageById<std::chrono::duration<std::int64_t, std::ratio<86400>>>(message.userid);
        AccCheck::ageByUsername<std::chrono::duration<std::int64_t, std::ratio<604800>>>(message.user);
        AccCheck::ageById<std::chrono::duration<std::int64_t, std::ratio<604800>>>(message.userid);
        AccCheck::ageByUsername<std::chrono::duration<std::int64_t, std::ratio<2629746>>>(message.user);
        AccCheck::ageById<std::chrono::duration<std::int64_t, std::ratio<2629746>>>(message.userid);
        AccCheck::ageByUsername<std::chrono::duration<std::int64_t, std::ratio<31556952>>>(message.user);
        AccCheck::ageById<std::chrono::duration<std::int64_t, std::ratio<31556952>>>(message.userid);
    } else {
        changeToLower(tokens[1]);
        
        auto& userids = UserIDs::getInstance();
        if(userids.isUser(tokens[1])) {
            auto userid = userids.getID(tokens[1]);
            
            AccCheck::ageByUsername(tokens[1]);
            AccCheck::ageById(userid);
            AccCheck::ageByUsername<std::chrono::minutes>(tokens[1]);
            AccCheck::ageById<std::chrono::minutes>(userid);
            AccCheck::ageByUsername<std::chrono::hours>(tokens[1]);
            AccCheck::ageById<std::chrono::hours>(userid);
            AccCheck::ageByUsername<std::chrono::duration<std::int64_t, std::ratio<86400>>>(tokens[1]);
            AccCheck::ageById<std::chrono::duration<std::int64_t, std::ratio<86400>>>(userid);
            AccCheck::ageByUsername<std::chrono::duration<std::int64_t, std::ratio<604800>>>(tokens[1]);
            AccCheck::ageById<std::chrono::duration<std::int64_t, std::ratio<604800>>>(userid);
            AccCheck::ageByUsername<std::chrono::duration<std::int64_t, std::ratio<2629746>>>(tokens[1]);
            AccCheck::ageById<std::chrono::duration<std::int64_t, std::ratio<2629746>>>(userid);
            AccCheck::ageByUsername<std::chrono::duration<std::int64_t, std::ratio<31556952>>>(tokens[1]);
            AccCheck::ageById<std::chrono::duration<std::int64_t, std::ratio<31556952>>>(userid);
        }
    }

    return response;
}

Response
CommandsHandler::addAllow(const IRCMessage &message,
                               [[maybe_unused]] std::vector<std::string> &tokens)
{
    Response response(1);
    
    if (tokens.size() <= 1) {
        return response;
    }

    changeToLower(tokens[1]);

    auto& userids = UserIDs::getInstance();
    if(!userids.isUser(tokens[1])) {
        return response;
    }

    AccCheck::addAllowed(tokens[1]);
    response.message = "Allowed user: " + tokens[1];
    response.type = Response::Type::MESSAGE;
    return response;
}

Response
CommandsHandler::delAllow(const IRCMessage &message,
                               [[maybe_unused]] std::vector<std::string> &tokens)
{
    Response response(1);
    
    if (tokens.size() <= 1) {
        return response;
    }

    changeToLower(tokens[1]);

    auto& userids = UserIDs::getInstance();
    if(!userids.isUser(tokens[1])) {
        return response;
    }

    AccCheck::delAllowed(tokens[1]);
    response.message = "Revoked allow for user: " + tokens[1];
    response.type = Response::Type::MESSAGE;
    return response;
}

Response
CommandsHandler::triviaStart(const IRCMessage &message,
                               [[maybe_unused]] std::vector<std::string> &tokens)
{
    Response response(1);
    
    if (!Trivia::allowedToStart(message.user)) {
        return response;
    }
    
    int i = 0;
    if (tokens.size() > 1) {
        i = std::atoi(tokens[1].c_str());
    }
    if (i <= 0) i = 10;    
    
    auto t = std::make_shared<boost::asio::steady_timer>(ioService,
                                               std::chrono::seconds(5));
    t->async_wait([this, t, i]([[maybe_unused]] const boost::system::error_code &er) {
            std::unique_lock lk(this->triviaMtx);
            this->triviaRunning = true;
            this->questionsLeft = i;
            lk.unlock();
            this->startNextTriviaQuestion();
    });
    
    
    response.message = "PogChamp New Trivia of " + std::to_string(i) + " questions started by " + message.user;
    response.type = Response::Type::MESSAGE;
    return response;
}

Response
CommandsHandler::triviaStop(const IRCMessage &message,
                               [[maybe_unused]] std::vector<std::string> &tokens)
{
    Response response(1);
    
    if (!Trivia::allowedToStart(message.user)) {
        return response;
    }
    

    std::unique_lock lk(this->triviaMtx);
    this->triviaRunning = false;
    this->questionsLeft = 0;
    if (this->triviaTimer) this->triviaTimer->cancel();
    this->triviaTimer.reset();
    lk.unlock();
    
    
    response.message = "Trivia stopped by " + message.user;
    response.type = Response::Type::MESSAGE;
    return response;
}

Response
CommandsHandler::triviaTop5(const IRCMessage &message,
                               [[maybe_unused]] std::vector<std::string> &tokens)
{
    Response response(0);
    response.message = "Trivia Top5: " + Trivia::top5();
    response.type = Response::Type::MESSAGE;
    return response;
}

Response
CommandsHandler::triviaPoints(const IRCMessage &message,
                               [[maybe_unused]] std::vector<std::string> &tokens)
{
    Response response(0);
    
    if (tokens.size() <= 1) {
        response.message = message.user + ", your trivia points are: " + std::to_string(Trivia::getPoints(message.user));
    } else {
        changeToLower(tokens[1]);
        auto& userids = UserIDs::getInstance();
        if(!userids.isUser(tokens[1])) {
            response.message = message.user + ", your trivia points are: " + std::to_string(Trivia::getPoints(message.user));
        } else {
            response.message = message.user + ", " + tokens[1] + "'s trivia points are: " + std::to_string(Trivia::getPoints(tokens[1]));
        }
    }
    response.type = Response::Type::MESSAGE;
    return response;
}


bool
CommandsHandler::startNextTriviaQuestion()
{
    std::lock_guard lk(this->triviaMtx);
    if (this->questionsLeft <= 0) {
        this->questionsLeft = 0;
        this->triviaRunning = false;
        return false;
    }
    if (!this->triviaRunning) {
        this->questionsLeft = 0;
        return false;
    }
    
    this->triviaQuestionInFlight = true;
    --this->questionsLeft;
    
    this->currentQuestion = std::move(Trivia::pop());
    this->triviaTimer = std::make_shared<boost::asio::steady_timer>(ioService,
                                               std::chrono::seconds(20));
    int answersize = this->currentQuestion.answer.size();
    int spaces = std::count(this->currentQuestion.answer.begin(), this->currentQuestion.answer.end(), ' ');

    std::stringstream ss;
    ss << "answer has " << answersize - spaces << " characters and " << spaces << " spaces";
    std::string hint_of_size = ss.str();
    this->triviaTimer->async_wait([this, hint_of_size] (const boost::system::error_code &er) mutable {
        if (er) return;
        std::unique_lock lk(this->triviaMtx);
        
        std::string hint;
        if (this->currentQuestion.hint1) hint = this->currentQuestion.hint1str;
        else hint = hint_of_size;
        
        this->channelObject->messenger.push_back("Hint: " + hint);
        
        if (!this->currentQuestion.hint1) hint_of_size = "";
        
        this->triviaTimer = std::make_shared<boost::asio::steady_timer>(ioService,
                                               std::chrono::seconds(20));
        this->triviaTimer->async_wait([this, hint_of_size](const boost::system::error_code &er) {
            if (er) return;
            std::unique_lock lk(this->triviaMtx);
            
            std::string hint;
            if (this->currentQuestion.hint2) hint = this->currentQuestion.hint2str;
            else hint = hint_of_size;
            
            if (hint_of_size == "") {
                int answersize = this->currentQuestion.answer.size();
                answersize /= 3;
                hint = this->currentQuestion.displayAnswer.substr(0, answersize);
                bool var = true;
                for (int i = answersize; i < this->currentQuestion.answer.size(); ++i) {
                    if (this->currentQuestion.answer[i] == ' ') hint += ' ';
                    else if (var) hint += '_';
                    else hint += '-';
                    var = !var;
                }
            }
            
            this->channelObject->messenger.push_back("Hint: " + hint);
            
            this->triviaTimer = std::make_shared<boost::asio::steady_timer>(ioService,
                                               std::chrono::seconds(20));
            this->triviaTimer->async_wait([this](const boost::system::error_code &er) {
                if (er) return;
                this->channelObject->messenger.push_back("Nobody got the answer right, the correct answer was " + this->currentQuestion.displayAnswer);
                
                this->triviaTimer = std::make_shared<boost::asio::steady_timer>(ioService,
                                               std::chrono::seconds(10));
                this->triviaTimer->async_wait([this](const boost::system::error_code &er) {
                    if (er) return;
                    if (!this->startNextTriviaQuestion()) {
                        this->channelObject->messenger.push_back("And Trivia is over FeelsBadMan");
                    }
                });
            });  
        });
    });
    std::string q;
    if (questionsLeft == 0) 
        q = std::to_string(questionsLeft+1)+ " trivia question left: \"" + this->currentQuestion.category + "\": " + this->currentQuestion.question;
    else q = std::to_string(questionsLeft+1)+ " trivia questions left: \"" + this->currentQuestion.category + "\": " + this->currentQuestion.question;
    this->channelObject->messenger.push_back(std::move(q));

    return true;
}