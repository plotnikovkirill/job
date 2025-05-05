#include <iostream>
#include <ctime>
#include <unistd.h>
#include "SerialPort.h"

int main() {
    SerialPort serial("/dev/pts/0"); // Укажите свой порт
    if (!serial.openPort()) {
        std::cerr << "Failed to open port" << std::endl;
        return 1;
    }

    while (true) {
        char buf[1];
        int n = serial.readData(buf, sizeof(buf));
        if (n > 0) {
            time_t now = time(nullptr);
            std::string timeStr = ctime(&now);
            timeStr.pop_back(); // Удалить \n в конце строки
            serial.writeData(timeStr.c_str(), timeStr.size());
        }
        usleep(100000); // 100 ms задержка
    }

    return 0;
}