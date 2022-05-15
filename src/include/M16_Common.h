#pragma once

#include <iostream>
#include <string>
#include <format>
#include <vector>
#include <unordered_map>
#include <stack>

namespace m16 {
	using byte = unsigned __int8;
	using word = unsigned __int16;

	constexpr bool IS_DEBUG = true;
	constexpr word MAX_MEM_SIZE = (1 << 16) - 1;

	enum class Register {
		R0,
		R1,
		R2,
		R3,
		R4,
		R5,
		R6, SP = 6,
		R7, LR = 7,
	};
}