/*
	Logan Pulley
	Library for use in the firework launching system.
*/

#include "Arduino.h"
#include "LoadStore.h"

LoadStore::LoadStore()
{
	clearAll();
}

void LoadStore::clearLoad(uint8_t load)
{
	load = load & 0xF;
	if (load % 2 == 0)
	{
		int index = load / 2;
		_loads[index] = _loads[index] & 0xF0;
	}
	else
	{
		int index = (load - 1) / 2;
		_loads[index] = _loads[index] & 0x0F;
	}
}

void LoadStore::clearAll()
{
	for (int i = 0; i < 8; i++)
	{
		_loads[i] = 0;
	}
}

void LoadStore::setLoad(uint8_t load, uint8_t delayPreset)
{
	load = load & 0xF;
	delayPreset = delayPreset & 0x7;
	clearLoad(load);
	uint8_t set = delayPreset<<1 | 0x1;
	if (load % 2 == 0)
	{
		int index = load / 2;
		_loads[index] = _loads[index] | set;
	}
	else
	{
		int index = (load - 1) / 2;
		_loads[index] = _loads[index] | set<<4;
	}
}

void LoadStore::setFromByte(int b, uint8_t in)
{
	b = b & 0x7;
	_loads[b] = in;
}

uint8_t LoadStore::getByte(int b)
{
	b = b & 0x7;
	return _loads[b];
}

uint8_t LoadStore::getLoadRaw(uint8_t load)
{
	load = load & 0xF;
	if (load % 2 == 0)
	{
		int index = load / 2;
		return (_loads[index] & 0xF);
	}
	else
	{
		int index = (load - 1) / 2;
		return (_loads[index]>>4);
	}
}

uint8_t LoadStore::getLoadState(uint8_t load)
{
	load = load & 0xF;
	return (getLoadRaw(load) & 0x1);
}

uint8_t LoadStore::getLoadDelayPreset(uint8_t load)
{
	load = load & 0xF;
	return (getLoadRaw(load)>>1 & 0x7);
}

bool LoadStore::hasNoneLoaded()
{
	for (int i = 0; i < 16; i++)
	{
		if (getLoadState(i))
		{
			return false;
		}
	}
	return true;
}