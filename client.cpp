#include <iostream>
#include <ctime>
#include <unistd.h>
#include "SerialPort.h"

int main() {
    SerialPort serial("/dev/pts/8"); // Укажите свой порт
    if (!serial.openPort()) {
        std::cerr << "Failed to open port" << std::endl;
        return 1;
    }

    while (true) {
        time_t loopStart = time(nullptr); 
        char request = 'T';
        if (serial.writeData(&request, 1) != 1) {
            std::cerr << "Write error" << std::endl;
        }

        char buffer[256]{};
        int totalRead = 0;
        time_t start = time(nullptr);
        while (time(nullptr) - start < 0.5) {
            int n = serial.readData(buffer + totalRead, sizeof(buffer) - totalRead - 1);
            if (n > 0) {
                totalRead += n;
            }
            usleep(10000); // 10 ms между попытками чтения
        }

        if (totalRead > 0) {
            buffer[totalRead] = '\0';
            std::cout << "Server time: " << buffer << std::endl;
        } else {
            std::cout << "No response from server" << std::endl;
        }
        //Если цикл оказался больше секунды то сразу идем к следующему циклу
        // иначе ждем оставшееся время до 1 секунды
        time_t elapsed = time(nullptr) - loopStart;
        if (elapsed < 1) {
            sleep(1 - elapsed);
        }
    }

    return 0;
}