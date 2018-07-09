#include <cstdlib>
#include <deque>
#include <iostream>
#include <thread>
#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>
#include <boost/array.hpp>
#include "../chat_message.hpp"
#include "jsoncpp/json/json.h"
#include "AsioClient.hpp"

#ifdef __cplusplus
extern "C"{
#endif
#include <string.h>
#ifdef __cplusplus
}
#endif

char * get_json_value(char *val, size_t size){
    int count = 0;
    int count_ret = 0;
    char *ret;
    char s;
    char key = '{';
    for(size_t i = 0; i < size; ++i){
        s = val[i];
        if(s != '{') {
            count++;
        }else{
            break;
        }
    }   
    ret = new char[(int)(size - count)];
    for(size_t i = count; i < size; ++i){
        ret[count_ret] = val[i];
        count_ret ++;
    }
    return ret;
}

int main()
{
    try
    {
        std::string host = "127.0.0.1";
        int portNumber = 3000;

        boost::asio::io_service io_service;
        tcp::resolver resolver(io_service);
        auto endpoint_iterator = resolver.resolve({ host, std::to_string(portNumber) });
        chat_client chatClient(io_service, endpoint_iterator);

        std::thread t([&io_service]()
            { 
                io_service.run();
            });

        boost::array<char, 128> buf;
        boost::system::error_code error;
        Json::Value jsonValue;
        Json::Reader jsonReader;

        size_t len = chatClient.socket_.read_some(boost::asio::buffer(buf), error);
        if (error == boost::asio::error::eof)
            chatClient.socket_.close(); // Connection closed cleanly by peer.
        else if (error)
            throw boost::system::system_error(error); // Some other error.

        chat_message msg;
        std::memcpy(msg.data(), buf.data(), len);
        std::cout.write(msg.data(), len);

        //Waiting for the response syncronously
        ///////////////////////////////////////

        std::string str(buf.data());
        std::cout << str <<std::endl;
        std::string strReceived = "";
        std::size_t pos = str.find("{");
        std::cout << "Found at "<<pos<<std::endl;
        if (pos!=std::string::npos){
            std::cout << "Found at "<<pos<<std::endl;
            strReceived = str.substr(pos,10);
            std::cout<<strReceived<<std::endl;
        }

        //Parse the json data structure
        char *ret = get_json_value(msg.data(), len);
        bool parsingSuccessful =
                jsonReader.parse(ret, jsonValue); // parse process
        if (!parsingSuccessful) {
            std::cout << "Failed to parse" << jsonReader.getFormattedErrorMessages();
        }

        std::string queryName = jsonValue["queryName"].asString();
        //If received engineID message from the server,
        //this means the engineID is assigned to this engine client
        if (queryName == "engineID"){
            chatClient.engineID = jsonValue["engineID"].asInt();
            std::cout << "This engine is assinged to ID: " <<chatClient.engineID<< std::endl;
        }
        ////////////////////////////////////////

        char line[chat_message::max_body_length + 1];
        while (std::cin.getline(line, chat_message::max_body_length + 1))
        {
            //Create message of json-based to send to the server for a request
            printf("%s\n", line);
            chat_message msg;
            Json::Value jsonValue;
            //This is my engine ID
            jsonValue["engineID"] = chatClient.engineID;
            //I request that I want to make a search for k-nearest neighbour
            jsonValue["queryName"] = "search";
            //of this descriptor vector
            jsonValue["contentVec"] = "0.1 0.2 0.3";
            std::string lineSend = jsonValue.toStyledString();

            //Now engage the json-based data to the chat-message
            msg.body_length(lineSend.length());
            std::memcpy(msg.body(), lineSend.c_str(), msg.body_length());

            msg.encode_header();
            chatClient.write(msg);

            //Waiting for the response syncronously
            ///////////////////////////////////////

            boost::array<char, 128> buf;
            boost::system::error_code error;

            size_t len = chatClient.socket_.read_some(boost::asio::buffer(buf), error);
            if (error == boost::asio::error::eof)
                break; // Connection closed cleanly by peer.
            else if (error)
                throw boost::system::system_error(error); // Some other error.

            //std::cout.write(buf.data(), len);
            std::memcpy(msg.data(), buf.data(), len);
            //std::cout.write(msg.data(), len);

            std::string str(msg.body());
            std::string strReceived = str.substr(0,msg.body_length());
            std::cout << "received: " <<strReceived<< std::endl;

            //Parse the json data structure

            bool parsingSuccessful =
                    jsonReader.parse(strReceived, jsonValue); // parse process
            if (!parsingSuccessful) {
                std::cout << "Failed to parse" << jsonReader.getFormattedErrorMessages();
            }

            std::string queryName = jsonValue["queryName"].asString();

            //This responses to the search query from the server
            //                    if (queryName == "search"){
            std::cout << "received results of search query: " <<jsonValue["contentVec"].asString()<< std::endl;

            ////////////////////////////////////////////

        }

        chatClient.close();
        t.join();
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}
