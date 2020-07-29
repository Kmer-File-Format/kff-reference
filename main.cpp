#include <iostream>
#include <cassert>

#include "kff_io.hpp"

using namespace std;


void encode_sequence(char * sequence, size_t size, uint8_t * encoded);

int main(int argc, char * argv[]) {
	Kff_file file = Kff_file("test.kff", "w");
	// Set encoding   A  C  G  T
	file.set_encoding(0, 1, 3, 2);
	// 2-bit sequence encoder
	uint8_t encoded[1024];
	encode_sequence("GGATGGGGG", 6, encoded);
	cout << (uint64_t)encoded[0] << " " << (uint64_t)encoded[1] << endl;
	// Close and end writing of the file.
	file.close();

	file = Kff_file("test.kff", "r");
	file.close();

}



// ---- DNA encoding functions [ACTG] -----

uint8_t uint8_packing(char * sequence, size_t size);
/* Encode the sequence into an array of uint8_t packed sequence slices.
 * The encoded sequences are organised in big endian order.
 */
void encode_sequence(char * sequence, size_t size, uint8_t * encoded) {
	// Encode the truncated first 8 bits sequence
	size_t remnant = size % 4;
	if (remnant > 0) {
		encoded[0] = uint8_packing(sequence, remnant);
		encoded += 1;
	}

	// Encode all the 8 bits packed
	size_t nb_uint_needed = size / 4;
	for (size_t i=0 ; i<nb_uint_needed ; i++) {
		encoded[i] = uint8_packing(sequence + remnant + (i<<2), 4);
	}
}


/* Transform a char * sequence into a uint8_t 2-bits/nucl
 * Encoding ACTG
 * Size must be <= 4
 */
uint8_t uint8_packing(char * sequence, size_t size) {
	assert(size <= 4);

	uint8_t val = 0;
	for (size_t i=0 ; i<size ; i++) {
		val <<= 2;
		val += (sequence[i] >> 1) & 0b11;
	}

	return val;
}

