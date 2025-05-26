#include <iostream>
#include <csignal>
#include <chrono>
#include <array>
#include "SerialPort.h"
#include <arpa/inet.h>

volatile sig_atomic_t running = 1;

#pragma pack(push, 1)
struct TimePacket 
{
    uint32_t timestamp; // Unix timestamp
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
        std::cout << "Use: " << argv[0] << " (port) (baudrate)\n";
        return 1;
    }
    
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    try 
    {
        
        SerialPort serial(argv[1], std::stoi(argv[2]));
        serial.open();

        std::array<uint8_t, 32> buf;
        TimePacket response;

        while(running) 
        {
            
            size_t bytes = serial.read(buf.data(), buf.size());

            if(bytes > 0) 
            {
                bool valid = false;
                for(size_t i = 0; i < bytes; ++i) 
                {
                    if(buf[i] == 'T') 
                    {
                        valid = true;
                        break;
                    }
                }

                if(valid) 
                {
                    const auto now = std::chrono::system_clock::now();
                    const auto ts = std::chrono::duration_cast<std::chrono::seconds>(
                        now.time_since_epoch()).count();
                    uint32_t net_ts = htonl(static_cast<uint32_t>(ts));
                    const uint16_t crc = crc16(
                        reinterpret_cast<const uint8_t*>(&net_ts), 
                        sizeof(net_ts)
                    );
                    response.timestamp = net_ts;
                    response.crc16 = htons(crc);

                    try 
                    {
                        serial.write(reinterpret_cast<const uint8_t*>(&response), sizeof(response));                        
                    }
                    catch(const std::exception& e) {
                        std::cerr << "Write error: " << e.what() << '\n';
                    }
                }
            }
            usleep(10000);
        }
    }
    catch(const std::exception& e) 
    {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }

    std::cout << "Server stopped\n";
    return 0;
}