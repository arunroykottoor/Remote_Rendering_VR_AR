#include "Game.h"

#include "socketServer.hpp"
#include "threadsafeQueue.hpp"

#include <opencv2/opencv.hpp>
#include <chrono>
#include <fstream>
#include <iomanip>

using std::chrono::high_resolution_clock;
using std::chrono::duration_cast;
using std::chrono::duration;
using std::chrono::milliseconds;


#define QUEUE_BUFFER_SIZE 10

static SafeQueue<ServerPacket> serverTXdataQueue(QUEUE_BUFFER_SIZE);
static SafeQueue<ClientPacket> serverRXdataQueue(QUEUE_BUFFER_SIZE);

#define WINDOW_WIDTH 960
#define WINDOW_HEIGHT 1080

static long int generated_packet_number = 0;
static long int rejected_packet_number = 0;
static long int accepted_packet_number = 0;

DWORD WINAPI renderingThread(LPVOID lpParameter)
{
	cv::namedWindow("Stiched Image", cv::WINDOW_AUTOSIZE);
	//cv::namedWindow("Desiarilsed Image", cv::WINDOW_AUTOSIZE);

	Game game("3D_Engine", WINDOW_WIDTH, WINDOW_HEIGHT, 4, 4, false);
	GLsizei nrChannels = 3;
	GLsizei stride = nrChannels * game.get_framebufferWidth();
	stride += (stride % 4) ? (4 - stride % 4) : 0;
	GLsizei bufferSize = stride * game.get_framebufferHeight();
	std::cout << "Required Buffer Size: " << bufferSize << std::endl;
	//std::cout << "Allocated packet Buffer Size: " << ALLOCATED_BUFFER_SIZE << std::endl;
	//assert((bufferSize == ALLOCATED_BUFFER_SIZE) && "ALLOCATED_BUFFER_SIZE must be equal to bufferSize");
	int count = 0;

	//MAIN LOOP
	while (true /*!game.getWindowShouldClose()*/)
	{		
			
			auto update_start = high_resolution_clock::now();
			std::cout << "Receiving End and Game update Start Time: " << std::chrono::duration_cast<std::chrono::milliseconds>(update_start.time_since_epoch()).count() << " ms" << std::endl;


			// timer for calculating fps of rendering
			auto rendering_start = high_resolution_clock::now();
		
			//struct ServerPacket packet;
			//cv::Mat rendered_img;
			std::vector<unsigned char> buffer;

			//UPDATE INPUT
			game.update();
			game.render();
			game.save_image(&buffer);

			auto render_start = high_resolution_clock::now();
			std::cout << "Update End and Game Render Start Time: " << std::chrono::duration_cast<std::chrono::milliseconds>(render_start.time_since_epoch()).count() << " ms" << std::endl;


			//std::cout << "buffer.size " << buffer.size() << std::endl;

			// stich two images together
			char imagepath[] = "C:/git/gump/3D_Engine/Images/test/1.png";
			//cv::Mat rendered_img = cv::imread(imagepath);
			int numChannels = 3;
			cv::Mat rendered_img(game.get_framebufferHeight(), game.get_framebufferWidth(), numChannels == 1 ? CV_8UC1 : CV_8UC3, buffer.data());
			cv::flip(rendered_img, rendered_img, 0);
			cv::Mat stiched_img(rendered_img.rows, rendered_img.cols * 2, rendered_img.type());
			cv::hconcat(rendered_img, rendered_img, stiched_img);
			cv::imshow("Stiched Image", stiched_img);
			cv::waitKey(1);

			//auto framebufferHeight = game.get_framebufferHeight();
			//auto framebufferWidth = game.get_framebufferWidth();
			//std::cout << " \n framebufferHeight = " << framebufferHeight << " \n framebufferWidth = " << framebufferWidth << std::endl;

			auto encode_start = high_resolution_clock::now();
			std::cout << "Game Render End and Encode Start Time: " << std::chrono::duration_cast<std::chrono::milliseconds>(encode_start.time_since_epoch()).count() << " ms" << std::endl;

			//continue;
			std::vector<unsigned char> encoded_buffer;
			auto t1 = high_resolution_clock::now();
			cv::imencode(".jpg", stiched_img, encoded_buffer);
			auto t2 = high_resolution_clock::now();
			std::cout << "Size after encoding : " << encoded_buffer.size()  << " Bytes" << std::endl;
			//std::cout << "Encoding time : " << duration_cast<milliseconds>(t2 - t1).count() << " Milli Sec" << std::endl;
			struct ServerPacket packet(encoded_buffer);

			//cv::Mat decoded_img = cv::imdecode(packet.buffer, cv::IMREAD_UNCHANGED);
			//cv::imshow("Decoded Image", decoded_img);

			//packet.timestamp = count++;


			// test serialization
			char filepath[] = "C:/git/gump/3D_Engine/Images/test/1_deser.png";

			//auto t1 = high_resolution_clock::now();
			//auto ss = packet.serialize();
			//struct ServerPacket deserialised_packet;
			//deserialised_packet.deserialize(ss);
			//auto t2 = high_resolution_clock::now();
			//std::cout << "Execution time: " << duration_cast<milliseconds>(t2 - t1).count() << std::endl;
		

			////deserialised_packet
			//std::cout << " \n Serialized Packet Size = " << ss.size() << std::endl;
			//auto framebufferHeight = game.get_framebufferHeight();
			//auto framebufferWidth = game.get_framebufferWidth();
			//std::cout << " \n framebufferHeight = " << framebufferHeight << " \n framebufferWidth = " << framebufferWidth << std::endl;
			//SOIL_save_image(filepath, SOIL_SAVE_TYPE_BMP, framebufferWidth, framebufferHeight, nrChannels, deserialised_packet.buffer.data());

			//std::cout << " \n framebufferHeight = " << rendered_img.rows << " \n framebufferWidth = " << rendered_img.cols * 2 
			//		  << " \n Buffer size = " << packet.buffer.size() << std::endl;
			//cv::Mat deser_stiched(stiched_img.rows, stiched_img.cols,
			//						numChannels == 1 ? CV_8UC1 : CV_8UC3, 
			//						packet.buffer.data());

			//// show deserialised image
			//cv::imshow("Desiarilsed Image", deser_stiched);
			//cv::waitKey(1);

			//std::cout << " generated_packet_number: " << generated_packet_number++ 
			//			<< "\t rejected_packet_number: " << rejected_packet_number 
			//			<< "\t accepted_packet_number: " << accepted_packet_number  << std::endl;

			auto transmission_start = high_resolution_clock::now();
			std::cout << "Encode End and Transmission start Time: " << std::chrono::duration_cast<std::chrono::milliseconds>(transmission_start.time_since_epoch()).count() << " ms" << std::endl;

			if (serverTXdataQueue.enqueue(packet) == false)
			{
				rejected_packet_number++;
			}
			else
			{
				accepted_packet_number++;
			}
			//std::this_thread::sleep_for(std::chrono::milliseconds(10));
			auto rendering_stop = high_resolution_clock::now();
			std::cout << std::fixed << std::setprecision(1)  << "Total FPS achieved for rendering : "
				<< 1000/(duration_cast<milliseconds>(rendering_stop - rendering_start).count() + 0.000001) 
				<< "FPS"  << std::endl;

	}

	return 0;
}


int main()
{
	//testlog();
	//return 1;
	// create a thread for the rendering
	HANDLE renderHandle;
	DWORD renderingThreadID;
	renderHandle = CreateThread(0, 0, renderingThread, 0, 0, &renderingThreadID);

	// create threads for socket comm
	connection(&serverTXdataQueue, &serverRXdataQueue);

	WaitForSingleObject(renderingThread, INFINITE);
	if(renderHandle)
	{
		CloseHandle(renderHandle);
	}
	return 0;
}

