#pragma once

#include <iostream>
#include <string.h>
#include <winsock2.h>
#include <sstream>
#include <opencv2/opencv.hpp>

#include "threadsafeQueue.hpp"

#define MSG_MAX_LENGTH 1024

//struct ServerPacket
//{
//	long int timestamp;
//	char buffer[640 * 480 * 3];
//};

const int ALLOCATED_BUFFER_SIZE = 600000; //921600;

//struct ServerPacket
//{
//	ServerPacket() : buffer(ALLOCATED_BUFFER_SIZE),
//					 bufferSize(ALLOCATED_BUFFER_SIZE),
//					 timestamp(0) {}
//	long int timestamp;
//	std::vector<unsigned char> buffer;
//	int bufferSize;
//};

struct ServerPacket
{
    // constructor and copy constructor
    ServerPacket() : buffer(ALLOCATED_BUFFER_SIZE),
        bufferSize(ALLOCATED_BUFFER_SIZE),
        timestamp(0) {}
    ServerPacket(std::vector<unsigned char>& encode_img) : buffer(encode_img), timestamp(0) 
    {
        bufferSize = encode_img.size();
    }
    ServerPacket(cv::Mat& img) : timestamp(0)
    {    
        for (int i = 0; i < img.rows; ++i) 
        {
            buffer.insert(buffer.end(), img.ptr<uchar>(i), img.ptr<uchar>(i) + img.cols * img.channels());
        }
        bufferSize = buffer.size();
    }
    long int timestamp;
    std::vector<unsigned char> buffer;
    int bufferSize;

    // Serialization
    //std::string serialize() const
    //{
    //    std::stringstream ss;
    //    ss << timestamp << ' ' << bufferSize << ' ';
    //    for (auto& b : buffer)
    //        ss << static_cast<int>(b) << ' ';
    //    return ss.str();
    //}
    std::string serialize() const
    {
        // Calculate the total length of the serialized string
        size_t totalLength = std::to_string(timestamp).length() + 1 + std::to_string(bufferSize).length() + 1 + bufferSize * (std::to_string(INT_MAX).length() + 1);

        std::string result;
        result.reserve(totalLength);  // Reserve memory

        result.append(std::to_string(timestamp)).append(" ");
        result.append(std::to_string(bufferSize)).append(" ");

        //for (auto& b : buffer)
        //    result.append(std::to_string(b)).append(" ");

        return result;
    }

    // Deserialization
    //void deserialize(const std::string& s)
    //{
    //    std::stringstream ss(s);
    //    ss >> timestamp >> bufferSize;
    //    buffer.resize(bufferSize);
    //    for (auto& b : buffer)
    //    {
    //        int temp;
    //        ss >> temp;
    //        b = static_cast<unsigned char>(temp);
    //    }
    //}
    void deserialize(const std::string& s)
    {
        std::istringstream iss(s);
        iss >> timestamp >> bufferSize;
        buffer.resize(bufferSize);
        //for (auto& b : buffer)
        //    iss >> b;
    }
};

struct dummyServerPacket
{
	long int timestamp;
	unsigned char buffer[10];
};

struct img_metdaData
{

};

struct ClientPacket
{
	long int timestamp;
	long int lastrxtimestamp;
	char msg[MSG_MAX_LENGTH];
	bool startImgTx = false;

};

DWORD WINAPI serverReceive(LPVOID lpParam);
DWORD WINAPI serverSend(LPVOID lpParam);
int connection(SafeQueue<ServerPacket>* txqueue, SafeQueue<ClientPacket>* rxqueue);

void readImage();