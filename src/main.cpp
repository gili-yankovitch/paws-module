#include <Arduino.h>
#ifdef WIRE
#include <Wire.h>
#else
#include <TinyWireS.h>
#include "TinyI2CMaster.h"
#endif

#define LED_EXISTS

#define I2C_BCAST_ADDR 0
#define I2C_MASTER_ADDR 1
#define TOKEN_RECV_PIN 1
#define TOKEN_SEND_PIN 3
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

	// Wait a bit
	delay(100);

	// Mark as done
	digitalWrite(TOKEN_SEND_PIN, HIGH);

	TinyWireS.onRequest(NULL);
}

void waitBoardStupFinish()
{
	while (digitalRead(TOKEN_RECV_PIN) == HIGH)
		;

	TinyWireS.stop();

	digitalWrite(TOKEN_SEND_PIN, LOW);
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
	blink(500);
#endif

	initI2CAddr();

	pinMode(BUTTON_PIN, INPUT);

	waitBoardStupFinish();
}

void loop()
{
	if (digitalRead(BUTTON_PIN) == HIGH)
	{
		if (state != BTN_STATE_RELEASED)
		{
			digitalWrite(TOKEN_SEND_PIN, HIGH);

			state = BTN_STATE_RELEASED;
			// Send status
			TinyI2C.init();
			TinyI2C.start(I2C_BCAST_ADDR, 0);
			TinyI2C.write(addr);
			TinyI2C.stop();
			TinyWireS.begin(addr);
		}
	}
	else
	{
		if (state != BTN_STATE_PRESSED)
		{
			state = BTN_STATE_PRESSED;

			// Send status
			TinyI2C.init();
			TinyI2C.start(I2C_BCAST_ADDR, 0);
			TinyI2C.write(addr | 0b10000000);
			TinyI2C.stop();

			// Send again just in case?
			TinyI2C.init();
			TinyI2C.start(I2C_BCAST_ADDR, 0);
			TinyI2C.write(addr | 0b10000000);
			TinyI2C.stop();
			TinyWireS.begin(addr);
		}
	}
}