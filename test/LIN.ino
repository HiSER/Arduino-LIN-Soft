#include "HSLINSoft.hpp"

/**
 * #include "HSLINSoft.hpp"
 *
 * HSLINSoft lin;
 *
 * timeoutBits = 0 - отключить таймаут
 * timeoutBits = 65535 - таймаут будет равет baud, т.е. ~1сек.
 * lin.begin(pinTX, pinRX[, baud = 19200[, breakBits = 13[, timeoutBits = 65535]]]);
 *
 * lin.query(pid, responseLength); // запрос, pid - адрес, responseLength - длина ответа включая CRC
 * uint8 buffer[9];
 * uint8 pid, length;
 * switch (lin.response(&pid, &buffer[0], &length))
 * {
 * 	case lin.erWait: break; // ответ на запрос ещё не пришел
 * 	case lin.erOk:
 * 		// pid		- pid запроса (адрес)
 * 		// buffer	- данные ответа включая CRC
 * 		// length	- длина данных включая CRC;
 * 		break;
 *	case lin.erTimeout: break; // таймаут ответа
 *	case lin.erNoQuery: break; // небыло запроса
 * }
 *
 * lin.query(pid, data, dataLength); // отправка данных, pid - адрес, data - данные включая CRC, dataLength - длина данных включая CRC
 * uint8 pid;
 * switch (lin.response(&pid))
 * {
 * 	case lin.erWait: break; // данные ещё отправляются
 * 	case lin.erOk:
 * 		// pid		- pid запроса (адрес)
 * 		break;
 *	case lin.erNoQuery: break; // небыло запроса
 * }
 */

typedef enum : uint8
{
	esWait = 0,
	esQuery,
	esResponse,
} eState;

HSLINSoft lin;
eState state;

void setup()
{
	Serial.begin(19200);
	/*
	 * TX - D8
	 * RX - D9
	 * baud - 9600 bit per second
	 * break - 13 bits
	 * timeout - 1s
	 */
	lin.begin(8, 9);
	state = esWait;
}

void loop()
{
	switch (state)
	{
		case esWait:
			Serial.println("... wait ...");
			delay(1000);
			state = esQuery;
			break;
		case esQuery:
			Serial.println("query 0xC1");
			if (lin.query(0xC1, 4))
			{
				state = esResponse;
			}
			else
			{
				Serial.println("query error");
				state = esWait;
			}
			break;
		case esResponse:
			uint8 pid;
			uint8 buffer[4];
			uint8 length;
			switch (lin.response(&pid, &buffer[0], &length))
			{
				case lin.erOk:
					Serial.print("response ");
					Serial.print(pid, HEX);
					Serial.print(" ");
					Serial.print(length);
					Serial.print(" -");
					for (uint8 i = 0; i < length; i++) {
						Serial.print(" ");
						Serial.print(buffer[i], HEX);
					}
					Serial.println();
					state = esWait;
					break;
				case lin.erTimeout:
					Serial.println("response timeout");
					state = esWait;
					break;
				case lin.erNoQuery:
					Serial.println("response no query");
					state = esWait;
					break;
			}
			break;
	}
}
