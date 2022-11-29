#include "pch.h"

inline uint8_t pix_r(uint32_t p)
{
	return p >> 16;
}
inline uint8_t pix_g(uint32_t p)
{
	return (p >> 8) & 0xFF;
}
inline uint8_t pix_b(uint32_t p)
{
	return p & 0xFF;
}