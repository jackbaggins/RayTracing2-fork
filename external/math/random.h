#ifndef RANDOM_H
#define RANDOM_H

float random(unsigned int& state)
{
	state = state * 747796405u + 2891336453u;
	unsigned int result = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
	result = (result >> 22u) ^ result;
	return result /  4294967295.0; // 2^32 - 1
}

float randomFloat(float min, float max, unsigned int& state)
{
    return min + (max - min) * random(state);
}

int random_int(int min, int max, unsigned int& state)
{
    return floor(randomFloat(min, max, state));
}

#endif