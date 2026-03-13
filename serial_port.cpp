#include "serial_port.h"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>

using namespace std;

speed_t baudrate_to_constant(int baudrate)
{
    switch(baudrate) {
        case 9600: return B9600;
        case 19200: return B19200;
        case 38400: return B38400;
        case 57600: return B57600;
        case 115200: return B115200;
        case 230400: return B230400;
        default: return 0; // неподдерживаемая скорость
    }
}

int serial_port_open(const char *portName, int baudrate)
{
    // Открываем порт
    int fd = open(portName, O_RDWR | O_NOCTTY | O_SYNC);
    
    if (fd < 0) {
        cerr << "Failed to open port " << portName 
             << ". Error: " << strerror(errno) << endl;
        return -1;
    }
    
    // Получаем константу скорости
    speed_t baud_const = baudrate_to_constant(baudrate);
    if (baud_const == 0) {
        cerr << "Unsupported baudrate: " << baudrate << endl;
        close(fd);
        return -1;
    }
    
    struct termios tty;
    
    // Получаем текущие настройки
    if (tcgetattr(fd, &tty) != 0) {
        cerr << "Failed to get port state. Error: " << strerror(errno) << endl;
        close(fd);
        return -1;
    }

    // Настройка скорости
    cfsetospeed(&tty, baud_const);
    cfsetispeed(&tty, baud_const);

    // Настройка параметров: 8 бит, без четности, 1 стоп-бит
    tty.c_cflag &= ~PARENB;          // без четности
    tty.c_cflag &= ~CSTOPB;           // 1 стоп-бит
    tty.c_cflag |= CS8;                // 8 бит данных
    tty.c_cflag &= ~CRTSCTS;           // без аппаратного управления потоком
    tty.c_cflag |= CREAD | CLOCAL;     // включить прием, игнорировать линии управления

    // Настройка для неканонического режима (raw)
    tty.c_lflag &= ~ICANON;            // неканонический режим
    tty.c_lflag &= ~ECHO;               // без эха
    tty.c_lflag &= ~ECHOE;              // без стирания
    tty.c_lflag &= ~ISIG;               // без генерации сигналов

    tty.c_iflag &= ~(IXON | IXOFF | IXANY); // без программного управления потоком
    tty.c_iflag &= ~(INLCR | ICRNL);        // без преобразования перевода строки

    tty.c_oflag &= ~OPOST;              // raw вывод

    // Настройка таймаутов для read
    tty.c_cc[VMIN] = 0;                 // не ждать минимального количества байт
    tty.c_cc[VTIME] = 10;                // таймаут 1 секунда (десятые доли секунды)

    // Применяем настройки
    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        cerr << "Failed to set port parameters. Error: " << strerror(errno) << endl;
        close(fd);
        return -1;
    }

    sleep(2);  // 2 секунды

    // Очищаем буфер от возможного мусора при включении
    tcflush(fd, TCIOFLUSH);
    
    return fd;
}

int serial_port_write(int fd, const char *command)
{
    if (fd < 0) return -1;
    
    string full_cmd = string(command) + '\r';
    ssize_t bytesWritten = write(fd, full_cmd.c_str(), full_cmd.size());
    
    if (bytesWritten < 0) {
        cerr << "Failed to write to port. Error: " << strerror(errno) << endl;
        return -1;
    }
    
    return bytesWritten;
}

int serial_port_read_line(int fd, char *response, size_t response_size, int timeout)
{
    if (fd < 0 || response == NULL || response_size == 0) return -1;

    memset(response, 0, response_size);
    
    // Настройка таймаута через VTIME (в десятых долях секунды)
    struct termios tty;
    if (tcgetattr(fd, &tty) == 0) {
        tty.c_cc[VTIME] = timeout;
        tcsetattr(fd, TCSANOW, &tty);
    }
    
    size_t pos = 0;
    char ch;
    ssize_t bytesRead;
    
    while (pos < response_size - 1) {
        bytesRead = read(fd, &ch, 1);
        
        if (bytesRead == 0) {
            // Таймаут
            break;
        }
        
        if (bytesRead < 0) {
            cerr << "Read error: " << strerror(errno) << endl;
            return -1;
        }
        
        response[pos++] = ch;
        
        if (ch == '\r') {
            size_t pos = ((string)response).rfind("OK\r");
            if (pos != std::string::npos) {
                // Конец строки
                break;
            }
            pos = ((string)response).rfind("E00\r");
            if (pos != std::string::npos) {
                // Конец строки
                break;
            }
            pos = ((string)response).rfind("E01\r");
            if (pos != std::string::npos) {
                // Конец строки
                break;
            }
            pos = ((string)response).rfind("E03\r");
            if (pos != std::string::npos) {
                // Конец строки
                break;
            }
        }
    }
    
    response[pos] = '\0'; // завершающий ноль
    return pos;
}

int serial_port_write_read(int fd, const char *command, char *response, 
                          size_t response_size, int timeout)
{
    if (serial_port_write(fd, command) < 0) {
        return -1;
    }
    
    return serial_port_read_line(fd, response, response_size, timeout);
}

void serial_port_close(int fd)
{
    if (fd >= 0) {
        close(fd);
    }
}