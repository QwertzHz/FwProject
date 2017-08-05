/*
	Logan Pulley
	Library for use in the firework launching system.
*/

#ifndef _LoadStore_H_
#define _LoadStore_H_

#include "Arduino.h"

class LoadStore
{
	public:
		LoadStore();
		void clearLoad(uint8_t load);
		void clearAll();
		void setLoad(uint8_t load, uint8_t delayPreset);
		void setFromByte(int b, uint8_t in);
		uint8_t getByte(int b);
		uint8_t getLoadRaw(uint8_t load);
		uint8_t getLoadState(uint8_t load);
		uint8_t getLoadDelayPreset(uint8_t load);
		bool hasNoneLoaded();
	private:
		uint8_t _loads[8];
};

#endif