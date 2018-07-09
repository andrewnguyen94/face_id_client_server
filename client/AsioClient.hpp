#ifndef CHAT_CLIENT_HPP
#define CHAT_CLIENT_HPP

#include <cstdlib>
#include <deque>
#include <iostream>
#include <thread>
#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>
#include "../chat_message.hpp"

#include "jsoncpp/json/json.h"

using boost::asio::ip::tcp;
typedef std::deque<chat_message> chat_message_queue;
class chat_client
{
public:
    chat_client(boost::asio::io_service& io_service,
                tcp::resolver::iterator endpoint_iterator);

    void write(const chat_message& msg);

    void close();

//private:
    void do_connect_sync(tcp::resolver::iterator endpoint_iterator);
    void do_connect(tcp::resolver::iterator endpoint_iterator);

    void do_read_header();

    void do_read_body();

    void do_write();

public:
    boost::asio::io_service& io_service_;
    tcp::socket socket_;
    chat_message_queue write_msgs_;

public:
    chat_message read_msg_;
    int engineID = -1;

    Json::Value jsonValue;
    Json::Reader jsonReader;
    bool hasResponse = false;
};
#endif // CHAT_CLIENT_HPP
