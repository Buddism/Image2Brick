#include "pch.h"

template<typename T>
void fast_vector_remove(std::vector<T>& vector, size_t index)
{
	if(vector.size() > 0)
	{
		vector[index] = vector[vector.size() - 1];
		vector.pop_back();
	}
}
unsigned pal[64];
int numPalColors = 0;