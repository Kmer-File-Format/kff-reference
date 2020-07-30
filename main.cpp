#include <iostream>
#include <cassert>

#include "kff_io.hpp"

using namespace std;


void encode_sequence(char * sequence, size_t size, uint8_t * encoded);

int main(int argc, char * argv[]) {
	// --- header writing ---
	Kff_file file = Kff_file("test.kff", "w");
	// Set encoding   A  C  G  T
	file.write_encoding(0, 1, 3, 2);
	// Set metadata
	file.write_metadata(11, "D@rK W@99ic");

	// --- global variable write ---

	// Set global variables
	Section_GV sgv = file.open_section_GV();
	sgv.write_var("k", 10);
	sgv.write_var("max", 255);
	sgv.write_var("data_size", 1);
	sgv.close();

	// 2-bit sequence encoder
	// uint8_t encoded[1024];
	// encode_sequence("GGATGGGGG", 6, encoded);
	// cout << (uint64_t)encoded[0] << " " << (uint64_t)encoded[1] << endl;

	// Close and end writing of the file.
	file.close();



	// --- header reading ---
	file = Kff_file("test.kff", "r");
	file.read_encoding();
	cout << (uint64_t)file.encoding[0] << " " << (uint64_t)file.encoding[1] << " " << (uint64_t)file.encoding[2] << " " << (uint64_t)file.encoding[3] << endl;
	char metadata[1024];
	uint32_t size = file.size_metadata();
	file.read_metadata(size, metadata);
	metadata[size] = '\0';
	cout << metadata << endl;

	// // --- Global variable read ---
	char section_name = file.read_section_type();
	cout << "New section: " << section_name << endl;
	sgv = file.open_section_GV();
	for (auto & elem : sgv.vars) {
		cout << elem.first << " = " << elem.second << endl;
	}

	// file.close();

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

