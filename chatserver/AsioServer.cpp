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

typedef std::shared_ptr<chat_participant> chat_participant_ptr;

//---------------------------CHAT ROOM CLASS IMPLEMENT-----------------------------------------//

void chat_room::join(chat_participant_ptr participant)
{
    participants_.insert(participant);
    participant->remoteEngineID = globalID;

    globalID++;
    std::cout<<"Number of participants: "<<participants_.size()<<std::endl;
    //for (auto msg: recent_msgs_)
    //    participant->deliver(msg);

    //This is to assign to the remote engine, later when engine send request we know which engine sent
    //and hence the server only send back info to the particular requested engine, not all

    //Create the json-based message
    //This message is sent from server to the engine client which is just joined in the room
    //The remoteEngineID will be store as the EngineID at local engine client
    chat_message msg;
    Json::Value jsonValue;
    jsonValue["engineID"] = participant->remoteEngineID;;
    jsonValue["queryName"] = "engineID";
    jsonValue["contentVec"] = "";
    std::string lineSend = jsonValue.toStyledString();
    msg.body_length(lineSend.length());

    std::memcpy(msg.body(), lineSend.c_str(), msg.body_length());
    msg.encode_header();

    participant->deliver(msg);

}

void chat_room::leave(chat_participant_ptr participant)
{
    participants_.erase(participant);
    std::cout<<"Number of participants: "<<participants_.size()<<std::endl;
}

void chat_room::deliverToOneEngine(const chat_message& msg, int remoteEngineID)
{

    for (auto participant: participants_){
        //only deliver to the remote engine that needs the message
        if (participant->remoteEngineID == remoteEngineID)
            participant->deliver(msg);
    }
}

//---------------------------CHAT SESSION CLASS IMPLEMENT-----------------------------------------//

chat_session::chat_session(tcp::socket socket, chat_room& room)
    : socket_(std::move(socket)),
      room_(room)
{
}

void chat_session::start()
{
    room_.join(shared_from_this());
    do_read_header();
}

void chat_session::deliver(const chat_message& msg)
{
    bool write_in_progress = !write_msgs_.empty();
    write_msgs_.push_back(msg);
    if (!write_in_progress)
    {
        do_write();
    }
}

void chat_session::do_read_header()
{
    auto self(shared_from_this());
    boost::asio::async_read(socket_,
                            boost::asio::buffer(read_msg_.data(), chat_message::header_length),
                            [this, self](boost::system::error_code ec, std::size_t /*length*/)
    {
        if (!ec && read_msg_.decode_header())
        {
            do_read_body();
        }
        else
        {
            room_.leave(shared_from_this());
        }
    });
}

void chat_session::do_read_body()
{
    auto self(shared_from_this());
    boost::asio::async_read(socket_,
                            boost::asio::buffer(read_msg_.body(), read_msg_.body_length()),
                            [this, self](boost::system::error_code ec, std::size_t /*length*/)
    {
        if (!ec)
        {
            std::string str(read_msg_.body());
            std::string strReceived = str.substr(0,read_msg_.body_length());
            std::cout << "received: " <<strReceived<< std::endl;

            //Parse the json data structure
            Json::Value jsonValue;
            Json::Reader jsonReader;
            bool parsingSuccessful =
                    jsonReader.parse(strReceived, jsonValue); // parse process
            if (!parsingSuccessful) {
                std::cout << "Failed to parse" << jsonReader.getFormattedErrorMessages();
            }else{
                int remoteEngineID = jsonValue["engineID"].asInt();
                std::string queryName = jsonValue["queryName"].asString();

                //This responses to the search query from the server
                if (queryName == "search"){
                    std::cout << "received search query: " <<jsonValue["contentVec"].asString()<< std::endl;
                }


                std::cout << "received: " <<jsonValue["contentVec"].asString()<< std::endl;

                //After the query from engine is solved --> we send back the answer to the engine based on the
                //remote engine ID
                room_.deliverToOneEngine(read_msg_,remoteEngineID);
            }

            do_read_header();
        }
        else
        {
            room_.leave(shared_from_this());
        }
    });
}

void chat_session::do_write()
{
    auto self(shared_from_this());
    boost::asio::async_write(socket_,
                             boost::asio::buffer(write_msgs_.front().data(),
                                                 write_msgs_.front().length()),
                             [this, self](boost::system::error_code ec, std::size_t /*length*/)
    {
        if (!ec)
        {
            write_msgs_.pop_front();
            if (!write_msgs_.empty())
            {
                do_write();
            }
        }
        else
        {
            room_.leave(shared_from_this());
        }
    });
}


//---------------------------CHAT SERVER CLASS IMPLEMENT-----------------------------------------//

chat_server::chat_server(boost::asio::io_service& io_service,
                         const tcp::endpoint& endpoint)
    : acceptor_(io_service, endpoint),
      socket_(io_service)
{
    do_accept();
}

void chat_server::do_accept()
{
    acceptor_.async_accept(socket_,
                           [this](boost::system::error_code ec)
    {
        if (!ec)
        {
            std::make_shared<chat_session>(std::move(socket_), room_)->start();
        }

        do_accept();
    });
}


