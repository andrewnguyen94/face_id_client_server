#include <cstdlib>
#include <deque>
#include <iostream>
#include <thread>
#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>
#include "../chat_message.hpp"
#include "AsioClient.hpp"

#include "jsoncpp/json/json.h"

using boost::asio::ip::tcp;

//---------------------------CHAT CLIENT CLASS IMPLEMENT-----------------------------------------//

chat_client::chat_client(boost::asio::io_service& io_service,
                         tcp::resolver::iterator endpoint_iterator)
    : io_service_(io_service),
      socket_(io_service)
{
//    do_connect_sync(endpoint_iterator);
    do_connect(endpoint_iterator);
}

void chat_client::write(const chat_message& msg)
{
    io_service_.post(
                [this, msg]()
    {
        bool write_in_progress = !write_msgs_.empty();
        write_msgs_.push_back(msg);
        if (!write_in_progress)
        {
            do_write();
        }
    });
}

void chat_client::close()
{
    io_service_.post([this]() { socket_.close(); });
}

void chat_client::do_connect_sync(tcp::resolver::iterator endpoint_iterator){

    tcp::resolver::iterator end;
    boost::system::error_code error = boost::asio::error::host_not_found;
    while (error && endpoint_iterator != end)
    {
      socket_.close();
      socket_.connect(*endpoint_iterator++, error);
    }
    if (error)
      throw boost::system::system_error(error);
}

void chat_client::do_connect(tcp::resolver::iterator endpoint_iterator)
{
    boost::asio::async_connect(socket_, endpoint_iterator,
                               [this](boost::system::error_code ec, tcp::resolver::iterator)
    {
//        if (!ec)
//        {
//            do_read_header();
//        }
    });
}

void chat_client::do_read_header()
{
    boost::asio::async_read(socket_,
                            boost::asio::buffer(read_msg_.data(), chat_message::header_length),
                            [this](boost::system::error_code ec, std::size_t /*length*/)
    {
        if (!ec && read_msg_.decode_header())
        {
            do_read_body();
        }
        else
        {
            socket_.close();
        }
    });
}

void chat_client::do_read_body()
{
    boost::asio::async_read(socket_,
                            boost::asio::buffer(read_msg_.body(), read_msg_.body_length()),
                            [this](boost::system::error_code ec, std::size_t /*length*/)
    {
        if (!ec)
        {
            std::string str(read_msg_.body());
            std::string strReceived = str.substr(0,read_msg_.body_length());

            //Parse the json data structure

            bool parsingSuccessful =
                    jsonReader.parse(strReceived, jsonValue); // parse process
            if (!parsingSuccessful) {
                std::cout << "Failed to parse" << jsonReader.getFormattedErrorMessages();
            }

            std::string queryName = jsonValue["queryName"].asString();
            //If received engineID message from the server,
            //this means the engineID is assigned to this engine client
            if (queryName == "engineID"){
                engineID = jsonValue["engineID"].asInt();
                std::cout << "This engine is assinged to ID: " <<engineID<< std::endl;

            }

            //This responses to the search query from the server
            if (queryName == "search"){
                hasResponse = true;
                //std::cout << "received results of search query: " <<jsonValue["contentVec"].asString()<< std::endl;
            }

            do_read_header();
        }
        else
        {
            std::cout << "Nothing" <<std::endl;
            socket_.close();
        }
    });
}

void chat_client::do_write()
{
    boost::asio::async_write(socket_,
                             boost::asio::buffer(write_msgs_.front().data(),
                                                 write_msgs_.front().length()),
                             [this](boost::system::error_code ec, std::size_t /*length*/)
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
            socket_.close();
        }
    });
}
