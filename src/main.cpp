#include <Arduino.h>
#ifdef WIRE
#include <Wire.h>
#else
#include <TinyWireS.h>
#endif

#define LED_EXISTS

#define I2C_BCAST_ADDR 0
#define I2C_MASTER_ADDR 1
#define TOKEN_RECV_PIN 3
#define TOKEN_SEND_PIN 1
#ifdef LED_EXISTS
#define LED_PIN 4
#endif

#define BUTTON_PIN 4

#define REQUESTS

uint8_t addr = I2C_BCAST_ADDR;

enum btn_state_e
{
	BTN_STATE_RELEASED = 0,
	BTN_STATE_PRESSED
};

static enum btn_state_e state = BTN_STATE_RELEASED;

#ifdef LED_EXISTS

void blink(int delayMs)
{
	digitalWrite(LED_PIN, HIGH);
	delay(delayMs);
	digitalWrite(LED_PIN, LOW);
}

#endif

#ifdef REQUESTS

void addrRecvCb(uint8_t num)
{
	addr = TinyWireS.receive();
}

bool ackSent = false;

void addrEchoCb()
{
	if (state == BTN_STATE_PRESSED)
	{
		TinyWireS.send(addr | 1);
	}
	else
	{
		TinyWireS.send(addr);
	}
	ackSent = true;
}

void reportStatus()
{
	if (state == BTN_STATE_PRESSED)
	{
		TinyWireS.send(addr | 1);
	}
	else
	{
		TinyWireS.send(addr);
	}
}

#endif
#ifndef WIRE
#ifndef TWI_RX_BUFFER_SIZE
#define TWI_RX_BUFFER_SIZE (16)
#endif
#include <TinyWireS.h>
#endif

void initI2CAddr()
{

	// Wait for the token
	while (digitalRead(TOKEN_RECV_PIN) == LOW)
		;

	// Act initially as SLAVE
	TinyWireS.begin(I2C_BCAST_ADDR);

#ifdef REQUESTS
	// Register broadcast receive
	TinyWireS.onReceive(addrRecvCb);

#ifdef WIRE
	// Request address assign
	TinyWireS.requestFrom(I2C_MASTER_ADDR, 1);
#endif

	delay(1000);

#ifdef LED_EXISTS

	blink(1000);
#endif
	// Wait for master to send data
	while ((addr == I2C_BCAST_ADDR) || (TinyWireS.available()))
	{
		TinyWireS_stop_check();
	}

	TinyWireS.onReceive(NULL);

	// Reset address
	TinyWireS.begin(addr);

	// Now setup echo response after addr is set
	TinyWireS.onRequest(addrEchoCb);

	while (!ackSent)
	{
		TinyWireS_stop_check();
	}
#else
	while (!TinyWireS.available())
		;

	// Read it
	addr = (uint8_t)TinyWireS.read();
#endif

	// Mark as done
	digitalWrite(TOKEN_SEND_PIN, HIGH);
#ifdef LED_EXISTS
	// Blink the address I got
	for (int i = 0; i < addr; ++i)
	{
		blink(500);
		delay(500);
	}
#endif
}

void setup()
{
#ifdef LED_EXISTS
	pinMode(LED_PIN, OUTPUT);
#else
	pinMode(BUTTON_PIN, INPUT);
#endif
	pinMode(TOKEN_RECV_PIN, INPUT);
	pinMode(TOKEN_SEND_PIN, OUTPUT);

	// Reset TOKEN_SEND
	digitalWrite(TOKEN_SEND_PIN, LOW);

#ifdef LED_EXISTS
	blink(2000);
#endif

	initI2CAddr();

	pinMode(BUTTON_PIN, INPUT);
}

void loop()
{
	while (true)
	{
		if (digitalRead(BUTTON_PIN) == HIGH)
		{
			state = BTN_STATE_RELEASED;
		}
		else
		{
			state = BTN_STATE_PRESSED;
		}

		TinyWireS_stop_check();
	}
}