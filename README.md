# HSLINSoft
	Arduino LIN software interface
# Example
```c++
#include "HSLINSoft.hpp"

HSLINSoft lin;

timeoutBits = 0 - отключить таймаут
timeoutBits = 65535 - таймаут будет равет baud, т.е. ~1сек.
lin.begin(pinTX, pinRX[, baud = 19200[, breakBits = 13[, timeoutBits = 65535]]]);

lin.query(pid, responseLength); // запрос, pid - адрес, responseLength - длина ответа включая CRC
uint8 buffer[9];
uint8 pid, length;
switch (lin.response(&pid, &buffer[0], &length))
{
	case lin.erWait: break; // ответ на запрос ещё не пришел
	case lin.erOk:
		// pid		- pid запроса (адрес)
		// buffer	- данные ответа включая CRC
		// length	- длина данных включая CRC;
		break;
	case lin.erTimeout: break; // таймаут ответа
	case lin.erNoQuery: break; // небыло запроса
}

lin.query(pid, data, dataLength); // отправка данных, pid - адрес, data - данные включая CRC, dataLength - длина данных включая CRC
uint8 pid;
switch (lin.response(&pid))
{
	case lin.erWait: break; // данные ещё отправляются
	case lin.erOk:
		// pid		- pid запроса (адрес)
		break;
	case lin.erNoQuery: break; // небыло запроса
}
```
