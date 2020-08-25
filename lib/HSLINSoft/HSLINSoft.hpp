#ifndef _HSLINSoft_
#define _HSLINSoft_ 1

/**
 * Author:		Artem Litvin (c)2020
 * Email:		hiser@mail.ru
 * WhatsApp:	+79836064928
 * License:		MIT
 */

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

#include "Arduino.h"

/* Arduino Nano, Uno */
#if defined(__AVR_ATmega168__) || defined(__AVR_ATmega328P__)

#else
#error Board not supported
#endif

typedef uint8_t uint8;
typedef uint16_t uint16;

class HSLINSoft
{
public:

	typedef enum : uint8
	{
		erWait = 0,
		erOk,
		erTimeout,
		erNoQuery
	} eResponse;

	static void begin(uint8 pinTX, uint8 pinRX, uint16 baud = 19200, uint8 breakBits = 13, uint16 timeoutBits = 65535);
	static void end();

	static inline bool query(uint8 pid, uint8 responseLength) { return __query(pid, responseLength, NULL, 0); }
	static inline bool query(uint8 pid, uint8* data, uint8 dataLength) { return __query(pid, 0, data, dataLength); }
	static eResponse response(uint8* pid, uint8* data = NULL, uint8* length = NULL);

private:

	const static uint8 LINSyncData		= 0x55;
	const static uint8 LINDataMax		= 8;

	typedef enum : uint8
	{
		esIdle = 0,
		esBreak,
		esData,
		esResponse,
		esComplete,
		esTimeout
	} eState;

	typedef struct
	{
		eState state;
		uint8 buffer[1 + 1 + LINDataMax + 1]; /* SYNC + PID + DATA + CRC */
		uint8 length;
		uint8 index;
		uint8 responseLength;
		uint8 bitCount;
		uint8 data;
		uint16 timeoutCount;
	} tQuery;

	typedef struct
	{
		struct
		{
			uint8 tx;
			uint8 rx;
		} pin;
		uint16 ticks;
		uint16 timeoutBits;
		uint8 breakBits;
		tQuery query;
	} tVar;
	
	static tVar var;

	static void __vector_pcint();
	static void __vector_compa();
	static void __vector_compb();
	
	static bool __query(uint8 pid, uint8 responseLength, uint8* data, uint8 dataLength);

	static inline void writeDirect(uint8 pin, bool high)
	{
		if (pin < 8) {
			if (high)
				DDRD |= (1 << pin);
			else
				DDRD &= ~(1 << pin);
		} else {
			if (high)
				DDRB |= (1 << (pin - 8));
			else
				DDRB &= ~(1 << (pin - 8));
		}
	}
	
	static inline void writePort(uint8 pin, bool high)
	{
		if (pin < 8)
		{
			if (high)
				PORTD |= (1 << pin);
			else
				PORTD &= ~(1 << pin);
		} else {
			if (high)
				PORTB |= (1 << (pin - 8));
			else
				PORTB &= ~(1 << (pin - 8));
		}
	}

	static inline bool readPort(uint8 pin)
	{
		if (pin < 8)
		{
			return ((PIND & (1 << pin)) != 0);
		} else {
			return ((PINB & (1 << (pin - 8))) != 0);
		}
	}
};

#endif
