#include "pch.h"

static void VerifyResult(uint8_t got, uint8_t want)
{
	EXPECT_EQ(got, want);
}

TEST(BabaSolver, RandomTesting)
{
	uint8_t i = 4;
	int8_t delta_i = -1;
	VerifyResult(i + delta_i, 3);
	i = 0;
	VerifyResult(i + delta_i, 255);
}
