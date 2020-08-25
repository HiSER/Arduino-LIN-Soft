/**
 * Author:		Artem Litvin (c)2020
 * Email:		hiser@mail.ru
 * WhatsApp:	+79836064928
 * License:		MIT
 */

#include "HSLINSoft.hpp"

static HSLINSoft::tVar HSLINSoft::var;

void HSLINSoft::begin(uint8 pinTX, uint8 pinRX, uint16 baud, uint8 breakBits, uint16 timeoutBits)
{
	end();
	var.query.state = esIdle;
	var.pin.tx = pinTX;
	var.pin.rx = pinRX;
	var.timeoutBits = ((timeoutBits == 65535) ? baud : timeoutBits);
	var.breakBits = breakBits;
	var.ticks = F_CPU / baud - 1;
	writeDirect(var.pin.tx, true);
	writePort(var.pin.tx, true);
	writeDirect(var.pin.rx, false);
	writePort(var.pin.rx, true);
	PCMSK0 = 0;
	PCMSK2 = 0;
	if (pinTX < 8)
		PCMSK2 |= (1 << pinTX);
	else
		PCMSK0 |= (1 << (pinTX - 8));
	if (pinRX < 8)
		PCMSK2 |= (1 << pinRX);
	else
		PCMSK0 |= (1 << (pinRX - 8));
}

void HSLINSoft::end()
{
	TCCR1A = 0;
	TCCR1B = 0;
	TIMSK1 = 0;
	PCICR &= ~((1 << PCIE2) | (1 << PCIE0));
	PCMSK0 = 0;
	PCMSK2 = 0;
	writeDirect(var.pin.tx, false);
	writePort(var.pin.tx, false);
	writeDirect(var.pin.rx, false);
	writePort(var.pin.rx, false);
}

ISR(PCINT0_vect) __attribute__((alias("_ZN9HSLINSoft14__vector_pcintEv")));
ISR(PCINT2_vect) __attribute__((alias("_ZN9HSLINSoft14__vector_pcintEv")));
__attribute__((signal, __INTR_ATTRS))
void HSLINSoft::__vector_pcint()
{
	if (!readPort(var.pin.rx))
	{
		TCCR1B = 0;
		TCNT1 = 0;
		TIFR1 = (1 << OCF1A);
		TIMSK1 = (1 << OCIE1A);
		TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS10);
		PCICR &= ~((1 << PCIE2) | (1 << PCIE0));
		var.query.bitCount = 0;
	}
}

ISR(TIMER1_COMPA_vect) __attribute__((alias("_ZN9HSLINSoft14__vector_compaEv")));
__attribute__((signal, __INTR_ATTRS))
void HSLINSoft::__vector_compa()
{
	if (var.query.state == esResponse)
	{
		if (var.query.bitCount == 0)
		{
			var.query.data = 0;
		}
		else if (var.query.bitCount < 9)
		{
			var.query.data >>= 1;
			var.query.data |= (readPort(var.pin.rx) ? 0x80 : 0);
		}
		var.query.bitCount++;
		if (var.query.bitCount == 9)
		{
			TCCR1B = 0;
			var.query.buffer[var.query.index] = var.query.data;
			var.query.index++;
			if ((var.query.index - 2) < var.query.responseLength)
			{
				PCIFR |= (1 << PCIF2) | (1 << PCIF0);
				PCICR |= (1 << PCIE2) | (1 << PCIE0);
				if (var.timeoutBits > 0)
				{
					TIFR1 = (1 << OCF1B);
					TIMSK1 = (1 << OCIE1B);
					TCNT1 = 0;
					TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS10);
					var.query.timeoutCount = 0;
				}
			}
			else
			{
				var.query.state = esComplete;
				TIMSK1 = 0;
			}
		}
	}
	else if (var.query.state == esData)
	{
		if (var.query.bitCount == 0)
		{
			writePort(var.pin.tx, false);
		}
		else if (var.query.bitCount < 9)
		{
			writePort(var.pin.tx, (var.query.data & 0x01) != 0);
			var.query.data >>= 1;
		}
		else if (var.query.bitCount == 9)
		{
			writePort(var.pin.tx, true);
		}
		var.query.bitCount++;
		if (var.query.bitCount > 9)
		{
			var.query.index++;
			if (var.query.index >= var.query.length)
			{
				TCCR1B = 0;
				if (var.query.responseLength == 0)
				{
					var.query.state = esComplete;
					TIMSK1 = 0;
				}
				else
				{
					var.query.state = esResponse;
					var.query.index = 2;
					PCIFR |= (1 << PCIF2) | (1 << PCIF0);
					PCICR |= (1 << PCIE2) | (1 << PCIE0);
					OCR1A = var.ticks / 2 - 55;
					if (var.timeoutBits > 0)
					{
						TIFR1 = (1 << OCF1B);
						TIMSK1 = (1 << OCIE1B);
						TCNT1 = 0;
						TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS10);
						var.query.timeoutCount = 0;
					}
				}
			}
			else
			{
				var.query.data = var.query.buffer[var.query.index];
			}
			var.query.bitCount = 0;
		}
	}
	else if (var.query.state == esBreak)
	{
		if (var.query.bitCount == 0)
		{
			writePort(var.pin.tx, false);
		}
		else if (var.query.bitCount == var.breakBits)
		{
			writePort(var.pin.tx, true);
		}
		var.query.bitCount++;
		if (var.query.bitCount > var.breakBits)
		{
			var.query.data = var.query.buffer[var.query.index];
			var.query.state = esData;
			var.query.bitCount = 0;
		}
	}
}

ISR(TIMER1_COMPB_vect) __attribute__((alias("_ZN9HSLINSoft14__vector_compbEv")));
__attribute__((signal, __INTR_ATTRS))
void HSLINSoft::__vector_compb()
{
	if (var.query.timeoutCount >= var.timeoutBits)
	{
		TCCR1B = 0;
		TIMSK1 = 0;
		PCICR &= ~((1 << PCIE2) | (1 << PCIE0));
		var.query.state = esTimeout;
	}
	var.query.timeoutCount++;
}

bool HSLINSoft::__query(uint8 pid, uint8 responseLength, uint8* data, uint8 dataLength)
{
	if (var.query.state == esIdle && responseLength <= (LINDataMax + 1) && dataLength <= (LINDataMax + 1))
	{
		var.query.state = esBreak;
		var.query.buffer[0] = LINSyncData;
		var.query.buffer[1] = pid;
		var.query.length = 2;
		var.query.index = 0;
		var.query.responseLength = responseLength;
		if (data != NULL)
		{
			memcpy(&var.query.buffer[2], &data[0], dataLength);
			var.query.length += dataLength;
			var.query.responseLength = 0;
		}
		else
		{
			var.query.responseLength = responseLength;
		}
		var.query.bitCount = 0;
		ICR1 = var.ticks;
		OCR1A = 1;
		OCR1B = 0;
		TCNT1 = 0;
		TIFR1 = (1 << OCF1A);
		TIMSK1 = (1 << OCIE1A);
		TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS10);
		return true;
	}
	return false;
}

HSLINSoft::eResponse HSLINSoft::response(uint8* pid, uint8* data, uint8* length)
{
	pid[0] = var.query.buffer[1];
	switch (var.query.state)
	{
		case esComplete:
			if (length != NULL)
			{
				if (var.query.responseLength > 0)
				{
					memcpy(&data[0], &var.query.buffer[2], var.query.responseLength);
				}
				length[0] = var.query.responseLength;
			}
			var.query.state = esIdle;
			return erOk;
		case esTimeout:
			var.query.state = esIdle;
			return erTimeout;
		case esIdle:
			return erNoQuery;
		default:
			return erWait;
	}
}
