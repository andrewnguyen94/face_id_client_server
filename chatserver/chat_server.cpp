
#include <cstdlib>
#include <deque>
#include <iostream>
#include <thread>
#include <list>
#include <memory>
#include <set>
#include <utility>
#include <boost/asio.hpp>
#include "../chat_message.hpp"
#include "jsoncpp/json/json.h"
#include "AsioServer.hpp"

using boost::asio::ip::tcp;

//----------------------------------------------------------------------
//Working on first chat-room at this time
std::list<chat_server> servers;
int chatRoomId = 0;

int main()
{
    try
    {
        int numberPortsOrChatRooms = 1;
        int portNumber[numberPortsOrChatRooms];
        portNumber[0] = 3000;

        boost::asio::io_service io_service;

        for (int i = 0; i < numberPortsOrChatRooms; ++i)
        {
            tcp::endpoint endpoint(tcp::v4(), portNumber[i]);
            servers.emplace_back(io_service, endpoint);
            //chat_server cserver(io_service, endpoint);
        }

        std::thread t([&io_service](){ io_service.run(); });

        char line[100];
        while (std::cin.getline(line, chat_message::max_body_length + 1))
        {
            chat_message msg;
            Json::Value jsonValue;
            jsonValue["engineID"] = 0;
            jsonValue["queryName"] = "search";
            jsonValue["contentVec"] = "0.1 0.2 0.3";
            std::string lineSend = jsonValue.toStyledString();

            msg.body_length(lineSend.length());
            std::memcpy(msg.body(), lineSend.c_str(), msg.body_length());
            msg.encode_header();

            std::list<chat_server>::iterator it = std::next(servers.begin(), chatRoomId);
            it->room_.deliverToOneEngine(msg,0);
            //cserver.room_.deliver(msg);

        }

        t.join();
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}
