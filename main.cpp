#include <iostream>
#include <cassert>
#include <sstream>
#include <string>

#include "kff_io.hpp"

using namespace std;


void encode_sequence(char * sequence, size_t size, uint8_t * encoded);
string decode_sequence(uint8_t * encoded, size_t size);

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
	sgv.write_var("max", 240);
	sgv.write_var("data_size", 1);
	sgv.close();

	// --- Write a raw sequence bloc ---
	Section_Raw sr = file.open_section_raw();
	// 2-bit sequence encoder
	uint8_t encoded[1024];
	uint8_t counts[255];
	encode_sequence("ACTAAACTGATT", 12, encoded);
	counts[0]=32;counts[1]=47;counts[2]=1;
	sr.write_compacted_sequence(encoded, 12, counts);
	encode_sequence("AAACTGATCG", 10, encoded);
	counts[0]=12;
	sr.write_compacted_sequence(encoded, 10, counts);
	encode_sequence("CTAAACTGATT", 11, encoded);
	counts[0]=1;counts[1]=47;
	sr.write_compacted_sequence(encoded, 11, counts);
	sr.close();

	sgv = file.open_section_GV();
	sgv.write_var("m", 8);
	sgv.close();

	// --- write a minimizer sequence block ---
	Section_Minimizer sm = file.open_section_minimizer();
	encode_sequence("AAACTGAT", 8, encoded);
	sm.write_minimizer(encoded);

	encode_sequence("ACTT", 4, encoded);
	counts[0]=32;counts[1]=47;counts[2]=1;
	sm.write_compacted_sequence_without_mini(encoded, 4, 2, counts);

	sm.close();

	// Close and end writing of the file.
	file.close();



	// --- header reading ---
	file = Kff_file("test.kff", "r");
	file.read_encoding();
	char metadata[1024];
	uint32_t size = file.size_metadata();
	file.read_metadata(size, metadata);

	// --- Global variable read ---
	char section_name = file.read_section_type();
	cout << "Read section " << section_name << endl;
	sgv = file.open_section_GV();

	uint64_t k = file.global_vars["k"];
	uint64_t max = file.global_vars["max"];
	uint64_t data_size = file.global_vars["data_size"];
	cout << endl;

	// --- Read Raw Block ---
	section_name = file.read_section_type();
	cout << "Read section " << section_name << endl;
	sr = file.open_section_raw();
	cout << "nb blocks: " << sr.nb_blocks << endl;

	uint8_t * seq = new uint8_t((max + k) / 8 + 1);
	uint8_t * data = new uint8_t(max * data_size);
	for (auto i=0 ; i<sr.nb_blocks ; i++) {
		cout << "bloc " << (i+1) << ": ";
		uint32_t nb_kmers = sr.read_compacted_sequence(seq, data);
		cout << nb_kmers << " kmers : ";
		cout << decode_sequence(seq, nb_kmers + k - 1) << " - ";
		for (auto i=0 ; i<nb_kmers ; i++)
			cout << (uint64_t)data[i] << ", ";
		cout << endl;
	}
	cout << endl;

	// --- Read variables to load m ---
	section_name = file.read_section_type();
	cout << "Read section " << section_name << endl;
	sgv = file.open_section_GV();
	uint64_t m = file.global_vars["m"];
	cout << endl;

	// --- Read Minimizer block ---
	sm = file.open_section_minimizer();
	cout << "Minimizer: " << decode_sequence(sm.minimizer, m) << endl;



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


void uint8_unpacking(uint8_t packed, char * decoded, size_t size);
string decode_sequence(uint8_t * encoded, size_t size) {
	stringstream ss;
	char tmp_chars[4];

	// Decode the truncated first compacted 8 bits
	size_t remnant = size % 4;
	if (remnant > 0) {
		uint8_unpacking(encoded[0], tmp_chars, remnant);
		for (size_t i=0 ; i<remnant ; i++) {
			ss << tmp_chars[i];
		}
		encoded += 1;
	}

	// Decode all the 8 bits packed
	size_t nb_uint_used = size / 4;
	for (size_t i=0 ; i<nb_uint_used ; i++) {
		uint8_unpacking(encoded[i], tmp_chars, 4);
		for (size_t i=0 ; i<4 ; i++) {
			ss << tmp_chars[i];
		}
	}

	return ss.str();
}

char const_nucleotides[4] = {'A', 'C', 'T', 'G'};
void uint8_unpacking(uint8_t packed, char * decoded, size_t size) {
	assert(size <= 4);

	size_t offset = 4 - size;
	for (size_t i=0 ; i<size ; i++) {
		decoded[i] = const_nucleotides[(packed >> ((3-i-offset) * 2)) & 0b11];
	}
}

