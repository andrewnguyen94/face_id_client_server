#ifndef ASIOSERVER_HPP
#define ASIOSERVER_HPP


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

using boost::asio::ip::tcp;

//----------------------------------------------------------------------

typedef std::deque<chat_message> chat_message_queue;

//----------------------------------------------------------------------

class chat_participant
{
public:
    int remoteEngineID;
    virtual ~chat_participant() {}
    virtual void deliver(const chat_message& msg) = 0;
};

typedef std::shared_ptr<chat_participant> chat_participant_ptr;

//----------------------------------------------------------------------

class chat_room
{
public:
    void join(chat_participant_ptr participant);

    void leave(chat_participant_ptr participant);

    void deliverToOneEngine(const chat_message& msg, int remoteEngineID);

private:
    std::set<chat_participant_ptr> participants_;
    int globalID = 0;

};

//----------------------------------------------------------------------

class chat_session : public chat_participant, public std::enable_shared_from_this<chat_session>
{
public:
    chat_session(tcp::socket socket, chat_room& room);

    void start();

    void deliver(const chat_message& msg);

private:
    void do_read_header();

    void do_read_body();

    void do_write();

    tcp::socket socket_;
    chat_room& room_;
    chat_message read_msg_;
    chat_message_queue write_msgs_;
};

class listener : public std::enable_shared_from_this<>
{
    
}

//----------------------------------------------------------------------

class chat_server
{
public:
    chat_server(boost::asio::io_service& io_service,
                const tcp::endpoint& endpoint);

    void do_accept();

public:
    tcp::acceptor acceptor_;
    tcp::socket socket_;
    chat_room room_;
};


#endif // ASIOSERVER_HPP
