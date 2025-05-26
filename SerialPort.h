#pragma once
#include <string>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <stdexcept>
#include <system_error>
#include <cstring>

class SerialPort 
{
    int fd = -1;
    std::string port;
    int baud;

    speed_t getBaud() const 
    {
        switch(baud) 
        {
            case 50: return B50;
            case 110: return B110;
            case 300: return B300;
            case 600: return B600;
            case 1200: return B1200;
            case 2400: return B2400;
            case 4800: return B4800;
            case 9600: return B9600;
            case 19200: return B19200;
            case 38400: return B38400;
            case 57600: return B57600;
            case 115200: return B115200;
            default: throw std::invalid_argument("Unsupported baud rate");
        }
    }

public:
    explicit SerialPort(std::string port, int baud) 
        : port(std::move(port)), baud(baud) {}

    ~SerialPort() { close(); }

    void open() {
        fd = ::open(port.c_str(), O_RDWR | O_NOCTTY | O_SYNC);
        if(fd < 0) {
            throw std::system_error(errno, std::system_category(),"Failed to open " + port);
        }

        termios tty{};
        if(tcgetattr(fd, &tty) != 0) {
            throw std::system_error(errno, std::system_category(), "tcgetattr failed");
        }

        // Основные настройки (8N1, no flow control)
        tty.c_cflag &= ~PARENB;    // No parity
        tty.c_cflag &= ~CSTOPB;    // 1 stop bit
        tty.c_cflag &= ~CSIZE;     // Clear data bits
        tty.c_cflag |= CS8;        // 8 data bits
        tty.c_cflag &= ~CRTSCTS;   // No hardware flow control
        tty.c_iflag &= ~(IXON | IXOFF | IXANY); // No software flow control
        
        // Минимальные настройки для работы
        tty.c_cflag |= CREAD;      // Enable receiver
        tty.c_lflag = 0;           // Raw mode
        tty.c_oflag = 0;           // Raw output
        
        // Настройка скорости
        cfsetispeed(&tty, getBaud());
        cfsetospeed(&tty, getBaud());


        if(tcsetattr(fd, TCSANOW, &tty) != 0) {
            throw std::system_error(errno, std::system_category(), "tcsetattr failed");
        }
    }

    void close() {
        if(fd != -1) {
            ::close(fd);
            fd = -1;
        }
    }

    size_t read(uint8_t* buf, size_t size) const {
        ssize_t n = ::read(fd, buf, size);
        if(n < 0 && errno != EAGAIN) {
            throw std::system_error(errno, std::system_category(), "Read failed");
        }
        return n > 0 ? static_cast<size_t>(n) : 0;
    }

    size_t write(const uint8_t* buf, size_t size) const {
        ssize_t n = ::write(fd, buf, size);
        if(n < 0) {
            throw std::system_error(errno, std::system_category(), "Write failed");
        }
        return static_cast<size_t>(n);
    }
};