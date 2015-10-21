#pragma once

//  All released versions as major<<16 + minor

#define FILE_VERSION_1_0  0x00010000U
#define FILE_VERSION_1_2  0x00010002U
#define FILE_VERSION_1_3  0x00010003U
#define FILE_VERSION_1_4  0x00010004U
#define FILE_VERSION_1_5  0x00010005U
#define FILE_VERSION_1_6  0x00010006U
#define FILE_VERSION_1_7  0x00010007U
#define FILE_VERSION_1_8  0x00010008U
#define FILE_VERSION_1_9  0x00010009U
#define FILE_VERSION_1_10 0x0001000AU
#define CURRENT_VERSION  FILE_VERSION_1_10

inline const char *version_string(unsigned v)
{
	static char buf[32];
	snprintf(buf, 31, "%u.%u", v >> 16, v & 0xFFFF);
	return buf;
}
