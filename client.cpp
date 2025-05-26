#include <iostream>
#include <csignal>
#include <chrono>
#include <ctime>
#include <array>
#include <arpa/inet.h>
#include "SerialPort.h"

volatile sig_atomic_t running = 1;

#pragma pack(push, 1)
struct TimePacket 
{
    uint32_t timestamp;
    uint16_t crc16;
};
#pragma pack(pop)

uint16_t crc16(const uint8_t* data, size_t len) 
{
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; ++i) 
    {
        crc ^= data[i];
        for (int j = 0; j < 8; ++j) 
        {
            crc = (crc & 1) ? (crc >> 1) ^ 0xA001 : crc >> 1;
        }
    }
    return crc;
}

void signal_handler(int) 
{
    running = 0;
}

int main(int argc, char* argv[]) 
{
    if(argc != 3) 
    {
        std::cerr << "Usage: " << argv[0] << " <port> <baudrate>\n";
        return 1;
    }

    signal(SIGINT, signal_handler);

    try 
    {
        SerialPort serial(argv[1], std::stoi(argv[2]));
        serial.open();

        const uint8_t request = 'T';
        TimePacket response;
        std::array<uint8_t, sizeof(response)> buf;
        size_t total = 0;

        while(running) 
        {
            try 
            {
                serial.write(&request, 1);
            }
            catch(...) 
            {
                std::cerr << "Write failed, retrying...\n";
                sleep(1);
                continue;
            }

            const auto start = std::chrono::steady_clock::now();
            bool received = false;

            while(running) {
                const auto now = std::chrono::steady_clock::now();
                
                if(now - start > std::chrono::milliseconds(100)) break;
                
                try {
                    const size_t bytes = serial.read(buf.data() + total,buf.size() - total);
                    if(bytes > 0) 
                    {
                        total += bytes;
                        if(total >= sizeof(response)) 
                        {
                            memcpy(&response, buf.data(), sizeof(response));
                            const uint16_t crc = ntohs(response.crc16);
                            const uint16_t calc_crc = crc16(
                                reinterpret_cast<const uint8_t*>(&response.timestamp), 
                                sizeof(response.timestamp)
                            );
                            response.timestamp = ntohl(response.timestamp);
                            if(calc_crc == crc) {
                                const time_t ts = response.timestamp;
                                std::cout << "Time: " << ctime(&ts);
                                received = true;
                            }
                            total = 0;
                            break;
                        }
                    }
                }
                catch(...) 
                {
                    break;
                }
                usleep(1000);
            }

            if(!received) 
            {
                std::cout << "No valid response\n";
            }

            sleep(1);
        }
    }
    catch(const std::exception& e) 
    {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }

    std::cout << "Client stopped\n";
    return 0;
}