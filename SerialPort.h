#include <string>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <system_error>

class SerialPort {
public:
    SerialPort(const std::string& port);
    ~SerialPort();

    bool openPort();
    void closePort();

    int readData(char* buffer, size_t size);
    int writeData(const char* buffer, size_t size);

private:
    std::string portName;
    int fileDescriptor;
    bool isOpen;
};

SerialPort::SerialPort(const std::string& port) 
    : portName(port), fileDescriptor(-1), isOpen(false) {}

SerialPort::~SerialPort() {
    closePort();
}

bool SerialPort::openPort() {
    fileDescriptor = open(portName.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
    if (fileDescriptor < 0) {
        perror("Failed to open port");
        return false;
    }

    termios tty{};
    if (tcgetattr(fileDescriptor, &tty) != 0) {
        perror("tcgetattr failed");
        closePort();
        return false;
    }

    cfsetispeed(&tty, B9600);
    cfsetospeed(&tty, B9600);


    if (tcsetattr(fileDescriptor, TCSANOW, &tty) != 0) {
        perror("tcsetattr failed");
        closePort();
        return false;
    }

    isOpen = true;
    return true;
}

void SerialPort::closePort() {
    if (isOpen) {
        close(fileDescriptor);
        fileDescriptor = -1;
        isOpen = false;
    }
}

int SerialPort::readData(char* buffer, size_t size) {
    if (!isOpen) return -1;
    int n = read(fileDescriptor, buffer, size);
    return n;
}

int SerialPort::writeData(const char* buffer, size_t size) {
    if (!isOpen) return -1;
    int n = write(fileDescriptor, buffer, size);
    return n;
}