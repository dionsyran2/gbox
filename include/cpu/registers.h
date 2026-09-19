#pragma once
#include <stdint.h>

#define F_CARRY			(1 << 4)
#define F_HALF_CARRY	(1 << 5)
#define F_SUBTRACTION	(1 << 6)
#define F_ZERO			(1 << 7)

struct registers_t {
	union {
		struct {
			uint8_t F; // Flags (Not an actual seperate register). Bottom 4 bits must be 0
			uint8_t A; // Accumulator
		};
		uint16_t AF; // Both
	};

	union {
		struct {
			uint8_t C;
			uint8_t B;
		};
		uint16_t BC;
	};

	union {
		struct {
			uint8_t E;
			uint8_t D;
		};
		uint16_t DE;
	};

	union {
		struct {
			uint8_t L;
			uint8_t H;
		};
		uint16_t HL;
	};

	uint16_t SP; // Stack Pointer
	uint16_t PC; // Program Counter

	bool IME; // Interrupt Master Enable


	inline bool get_flag(uint8_t flag_mask) const {
		return (this->F & flag_mask) != 0;
	}

	inline void set_flag(uint8_t flag_mask, bool condition) {
		if (condition) {
			this->F |= flag_mask;
		}
		else {
			this->F &= ~flag_mask;
		}
	}
};