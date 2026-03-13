#ifndef SERIAL_PORT_H
#define SERIAL_PORT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>  // для size_t

/**
 * Открывает последовательный порт и настраивает его параметры
 * @param portName - имя порта (например, "/dev/ttyUSB0")
 * @param baudrate - скорость (9600, 115200 и т.д.)
 * @return файловый дескриптор или -1 в случае ошибки
 */
int serial_port_open(const char *portName, int baudrate);

/**
 * Отправляет команду в порт и читает ответ
 * @param fd - файловый дескриптор порта
 * @param command - команда для отправки
 * @param response - буфер для ответа (должен быть выделен вызывающим)
 * @param response_size - размер буфера ответа
 * @param timeout_ms - таймаут ожидания ответа в миллисекундах
 * @return количество прочитанных байт или -1 в случае ошибки
 */
int serial_port_write_read(int fd, const char *command, char *response, 
                          size_t response_size, int timeout_ms);

/**
 * Отправляет команду в порт (без ожидания ответа)
 * @param fd - файловый дескриптор порта
 * @param command - команда для отправки
 * @return количество отправленных байт или -1 в случае ошибки
 */
int serial_port_write(int fd, const char *command);

/**
 * Читает ответ из порта до символа '\r' или до таймаута
 * @param fd - файловый дескриптор порта
 * @param response - буфер для ответа
 * @param response_size - размер буфера
 * @param timeout_ms - таймаут в миллисекундах
 * @return количество прочитанных байт или -1 в случае ошибки
 */
int serial_port_read_line(int fd, char *response, size_t response_size, int timeout_ms);

/**
 * Закрывает последовательный порт
 * @param fd - файловый дескриптор порта
 */
void serial_port_close(int fd);

#ifdef __cplusplus
}
#endif

#endif // SERIAL_PORT_H