#include <iostream>
#include <cassert>
#include <cstring>
#include <sstream>
#include <math.h>

#include <map>

#include "kff_io.hpp"

using namespace std;


// Utils

template<typename T>
void write_value(T val, fstream & fs) {
	fs.write((char*)&val, sizeof(T));
}
template<typename T>
void read_value(T & val, fstream & fs) {
	fs.read((char*)&val, sizeof(T));
}

uint64_t bytes_from_bit_array(uint64_t bits_per_elem, uint64_t nb_elem) {
	return ((bits_per_elem * nb_elem - 1) / 8) + 1;
}


// ----- Open / Close functions -----

Kff_file::Kff_file(const char * filename, const char * mode) {
	// Variable init
	this->is_writer = false;
	this->is_reader = false;
	auto streammode = fstream::binary;

	// Determine the mode and open the file
	if (mode[0] == 'w') {
		this->is_writer = true;
		streammode |= fstream::out;
	} else if (mode[0] == 'r') {
		this->is_reader = true;
		streammode |= fstream::in;
	} else {
		cerr << "Unsuported mode " << mode << endl;
		exit(0);
	}

	// Open the file
	this->fs.open(filename, streammode);

	// Write the version at the beginning of the file
	if (this->is_writer) {
		this->fs << (char)KFF_VERSION_MAJOR << (char)KFF_VERSION_MINOR;
	}

	// Read the header
	else if (this->is_reader) {
		this->fs >> this->major_version >> this->minor_version;

		if (KFF_VERSION_MAJOR < this->major_version or (KFF_VERSION_MAJOR == this->major_version and KFF_VERSION_MINOR < this->minor_version)) {
			cerr << "The software version " << KFF_VERSION_MAJOR << "." << KFF_VERSION_MINOR << " can't read files writen in version " << this->major_version << "." << this->minor_version << endl;
		}
	}

}


void Kff_file::close() {
	if (this->fs.is_open())
		this->fs.close();
	this->is_writer = false;
	this->is_reader = false;

	cout << endl;
}


// ----- Header functions -----

void Kff_file::write_encoding(uint8_t a, uint8_t c, uint8_t g, uint8_t t) {
	// Value masking
	a &= 0b11; c &= 0b11; g &= 0b11; t &= 0b11;

	// Verification of the differences.
	assert(a != c); assert(a != g); assert(a != t);
	assert(c != g); assert(g != t);
	assert(g != t);

	// set values
	this->encoding[0] = a;
	this->encoding[1] = c;
	this->encoding[2] = g;
	this->encoding[3] = t;

	// Write to file
	uint8_t code = (a << 6) | (c << 4) | (g << 2) | t;
	this->fs << code;
}

void Kff_file::read_encoding() {
	uint8_t code, a, c, g, t;
	// Get code values
	this->fs >> code;

	// Split each nucleotide encoding
	this->encoding[0] = a = (code >> 6) & 0b11;
	this->encoding[1] = c = (code >> 4) & 0b11;
	this->encoding[2] = g = (code >> 2) & 0b11;
	this->encoding[3] = t = code & 0b11;

	// Verification of the encoding
	assert(a != c); assert(a != g); assert(a != t);
	assert(c != g); assert(g != t);
	assert(g != t);
}

void Kff_file::write_metadata(uint32_t size, char * data) {
	write_value(size, fs);
	this->fs.write(data, size);
}

uint32_t Kff_file::size_metadata() {
	uint32_t meta_size;
	read_value(meta_size, fs);
	return meta_size;
}

void Kff_file::read_metadata(uint32_t size, char * data) {
	this->fs.read(data, size);
}


// ----- Sections -----

char Kff_file::read_section_type() {
	char type;
	this->fs >> type;
	this->fs.seekp(this->fs.tellp() - 1l);
	return type;
}


// ----- Global variables sections -----

Section_GV Kff_file::open_section_GV() {
	return Section_GV(this);
}

Section_GV::Section_GV(Kff_file * file) {
	this->file = file;
	this->begining = file->fs.tellp();
	this->nb_vars = 0;

	if (file->is_reader) {
		this->read_section();
	}

	if (file->is_writer) {
		fstream & fs = file->fs;
		fs << 'v';
		write_value(nb_vars, fs);
	}
}

void Section_GV::write_var(const string & var_name, uint64_t value) {
	auto & fs = this->file->fs;
	fs << var_name << '\0';
	write_value(value, fs);
	this->nb_vars += 1;

	this->vars[var_name] = value;
	this->file->global_vars[var_name] = value;
}

void Section_GV::read_section() {
	char type;
	file->fs >> type;
	assert(type == 'v');

	read_value(this->nb_vars, file->fs);
	for (auto i=0 ; i<nb_vars ; i++) {
		this->read_var();
	}
}

void Section_GV::read_var() {
	// Name reading
	stringstream ss;
	char c = 'o';
	this->file->fs >> c;
	while (c != '\0') {
		ss << c;
		this->file->fs >> c;
	}

	// Value reading
	uint64_t value;
	read_value(value, file->fs);

	// Saving
	string name = ss.str();
	this->vars[name] = value;
	this->file->global_vars[name] = value;
}

void Section_GV::close() {
	if (file->is_writer) {
		// Save current position
		fstream &	 fs = this->file->fs;
		long position = fs.tellp();
		// Go write the number of variables in the correct place
		fs.seekp(this->begining + 1);
		write_value(nb_vars, fs);
		fs.seekp(position);
	}
}


// ----- Raw sequence section -----

Section_Raw Kff_file::open_section_raw() {
	assert(this->global_vars.find("k") != this->global_vars.end());
	assert(this->global_vars.find("max") != this->global_vars.end());
	assert(this->global_vars.find("data_size") != this->global_vars.end());
	return Section_Raw(this, this->global_vars["k"], this->global_vars["max"], this->global_vars["data_size"]);
}

Section_Raw::Section_Raw(Kff_file * file, uint64_t k, uint64_t max, uint64_t data_size) {
	this->file = file;
	this->begining = file->fs.tellp();
	this->nb_blocks = 0;

	this->k = k;
	this->max = max;
	this->data_size = data_size;

	// Computes the number of bytes needed to store the number of kmers in each block
	uint64_t nb_bits = static_cast<uint64_t>(ceil(log2(max)));
	this->nb_kmers_bytes = static_cast<uint8_t>(bytes_from_bit_array(nb_bits, 1));

	if (file->is_reader) {
		this->read_section_header();
	}

	if (file->is_writer) {
		fstream & fs = file->fs;
		fs << 'r';
		write_value(nb_blocks, fs);
	}
}

uint32_t Section_Raw::read_section_header() {
	fstream & fs = file->fs;

	char type;
	fs >> type;
	assert(type == 'r');

	read_value(nb_blocks, fs);

	return nb_blocks;
}

void Section_Raw::write_compacted_sequence(uint8_t* seq, uint64_t seq_size, uint8_t * data_array) {
	// 1 - Write nb kmers
	uint64_t nb_kmers = seq_size - k + 1;
	file->fs.write((char*)&nb_kmers, this->nb_kmers_bytes);
	// 2 - Write sequence
	uint64_t seq_bytes_needed = bytes_from_bit_array(2, seq_size);
	this->file->fs.write((char *)seq, seq_bytes_needed);
	// 3 - Write data
	uint64_t data_bytes_needed = bytes_from_bit_array(data_size*8, nb_kmers);
	this->file->fs.write((char *)data_array, data_bytes_needed);

	this->nb_blocks += 1;
}

uint64_t Section_Raw::read_compacted_sequence(uint8_t* seq, uint8_t* data) {
	uint64_t nb_kmers_in_block = 0;
	// 1 - Read the number of kmers in the sequence
	file->fs.read((char*)&nb_kmers_in_block, this->nb_kmers_bytes);
	// 2 - Read the sequence
	size_t seq_size = nb_kmers_in_block + k - 1;
	size_t seq_bytes_needed = bytes_from_bit_array(2, seq_size);
	file->fs.read((char*)seq, seq_bytes_needed);
	// 3 - Read the data.
	uint64_t data_bytes_used = bytes_from_bit_array(data_size*8, nb_kmers_in_block);
	file->fs.read((char*)data, data_bytes_used);
	return nb_kmers_in_block;
}

void Section_Raw::close() {
	if (file->is_writer) {
		// Save current position
		fstream &	 fs = this->file->fs;
		long position = fs.tellp();
		// Go write the number of variables in the correct place
		fs.seekp(this->begining + 1);
		write_value(nb_blocks, fs);
		fs.seekp(position);
	}
}



// ----- Minimizer sequence section -----

Section_Minimizer Kff_file::open_section_minimizer() {
	assert(this->global_vars.find("k") != this->global_vars.end());
	assert(this->global_vars.find("m") != this->global_vars.end());
	assert(this->global_vars.find("max") != this->global_vars.end());
	assert(this->global_vars.find("data_size") != this->global_vars.end());
	return Section_Minimizer(this, this->global_vars["k"], this->global_vars["m"], this->global_vars["max"], this->global_vars["data_size"]);
}

Section_Minimizer::Section_Minimizer(Kff_file * file, uint64_t k, uint64_t m, uint64_t max, uint64_t data_size) {
	this->file = file;
	this->begining = file->fs.tellp();
	this->nb_blocks = 0;

	this->k = k;
	this->m = m;
	this->max = max;
	this->data_size = data_size;

	// Computes the number of bytes needed to store the number of kmers in each block
	uint64_t nb_bits = static_cast<uint64_t>(ceil(log2(max)));
	this->nb_kmers_bytes = static_cast<uint8_t>(bytes_from_bit_array(nb_bits, 1));
	this->nb_bytes_mini = static_cast<uint8_t>(bytes_from_bit_array(2, m));
	this->minimizer = new uint8_t[nb_bytes_mini];
	uint64_t mini_pos_bits = static_cast<uint8_t>(ceil(log2(k+max-1)));
	this->mini_pos_bytes = bytes_from_bit_array(mini_pos_bits, 1);

	if (file->is_reader) {
		this->read_section_header();
	}

	if (file->is_writer) {
		fstream & fs = file->fs;
		fs << 'm';
		this->write_minimizer(this->minimizer);
		file->fs.seekp(file->fs.tellp()+(long)nb_bytes_mini);
		write_value(nb_blocks, fs);
	}
}

void Section_Minimizer::write_minimizer(uint8_t * minimizer) {
	uint64_t pos = file->fs.tellp();
	file->fs.seekp(this->begining+1);
	file->fs.write((char *)minimizer, this->nb_bytes_mini);
	this->minimizer = minimizer;
	file->fs.seekp(pos);
	
}

uint32_t Section_Minimizer::read_section_header() {
	fstream & fs = file->fs;

	// Verify section type
	char type;
	fs >> type;
	assert(type == 'm');

	// Read the minimizer
	file->fs.read((char *)this->minimizer, this->nb_bytes_mini);

	// Read the number of following blocks
	read_value(nb_blocks, fs);
	return nb_blocks;
}

void Section_Minimizer::write_compacted_sequence_without_mini(uint8_t* seq, uint64_t seq_size, uint64_t mini_pos, uint8_t * data_array) {
	// 1 - Write nb kmers
	uint64_t nb_kmers = seq_size + m - k + 1;
	file->fs.write((char*)&nb_kmers, this->nb_kmers_bytes);
	// 2 - Write minimizer position
	file->fs.write((char *)&mini_pos, this->mini_pos_bytes);
	// 3 - Write sequence with chopped minimizer
	uint64_t seq_bytes_needed = bytes_from_bit_array(2, seq_size);
	this->file->fs.write((char *)seq, seq_bytes_needed);
	// 4 - Write data
	uint64_t data_bytes_needed = bytes_from_bit_array(data_size*8, nb_kmers);
	this->file->fs.write((char *)data_array, data_bytes_needed);

	this->nb_blocks += 1;
}

uint64_t Section_Minimizer::read_compacted_sequence_without_mini(uint8_t* seq, uint8_t* data, uint64_t & mini_pos) {
	uint64_t nb_kmers_in_block = 0;
	// 1 - Read the number of kmers in the sequence
	file->fs.read((char*)&nb_kmers_in_block, this->nb_kmers_bytes);
	// 2 - Read the minimizer position
	uint64_t tmp_mini_pos = 0;
	file->fs.read((char *)&tmp_mini_pos, this->mini_pos_bytes);
	mini_pos = tmp_mini_pos;
	// 3 - Read the sequence
	size_t seq_size = nb_kmers_in_block + k - m - 1;
	size_t seq_bytes_needed = bytes_from_bit_array(2, seq_size);
	file->fs.read((char*)seq, seq_bytes_needed);
	// 4 - Read the data
	uint64_t data_bytes_needed = bytes_from_bit_array(data_size*8, nb_kmers_in_block);
	file->fs.read((char*)data, data_bytes_needed);
	return nb_kmers_in_block;
}

/* Bitshift to the left all the bits in the array with a maximum of 7 bits.
 * Overflow on the left will be set into the previous cell.
 */
void leftshift8(uint8_t * bitarray, size_t length, size_t bitshift) {
	assert(bitshift < 8);

	for (uint64_t i=0 ; i<length-1 ; i++) {
		bitarray[i] = (bitarray[i] << bitshift) | (bitarray[i+1] >> (8-bitshift));
	}
	bitarray[length-1] <<= bitshift;
}

/* Similar to the previous function but on the right */
void rightshift8(uint8_t * bitarray, size_t length, size_t bitshift) {
	assert(bitshift < 8);

	for (uint64_t i=length-1 ; i>0 ; i--) {
		bitarray[i] = (bitarray[i-1] << (8-bitshift)) | (bitarray[i] >> bitshift);
	}
	bitarray[0] >>= bitshift;
}

/* Fusion to bytes into one.
 * The merge_index higher bits are from left_bits the others from right_bits
 */
inline uint8_t fusion8(uint8_t left_bits, uint8_t right_bits, size_t merge_index) {
	uint8_t mask = 0xFF << (8-merge_index);
	return (left_bits & mask) | (right_bits & ~mask);
}

void Section_Minimizer::write_compacted_sequence (uint8_t* seq, uint64_t seq_size, uint64_t mini_pos, uint8_t * data_array) {
	// Compute all the bit and byte quantities needed.
	uint64_t seq_bytes = bytes_from_bit_array(2, seq_size);

	uint64_t bit_mini_start = 2 * mini_pos;
	uint64_t byte_mini_start = bit_mini_start / 8;
	uint8_t offset_mini_start = bit_mini_start % 8;

	uint64_t bit_mini_stop = bit_mini_start + 2 * m;
	uint64_t byte_mini_stop = bit_mini_stop / 8;
	uint8_t offset_mini_stop = bit_mini_stop % 8;

	// Prepare space for sequence manipulation
	size_t copy_size = seq_bytes - byte_mini_stop + 1;
	uint8_t * seq_copy = new uint8_t[copy_size];
	// Copy the suffix of the sequence and bitshift it to align with the prefix
	if (offset_mini_start < offset_mini_stop) {
		memcpy(seq_copy + 1, seq + byte_mini_stop, seq_bytes - byte_mini_stop);
		leftshift8(seq_copy, copy_size, offset_mini_stop - offset_mini_start);
	} else if (offset_mini_start > offset_mini_stop) {
		memcpy(seq_copy, seq + byte_mini_stop, seq_bytes - byte_mini_stop);
		rightshift8(seq_copy, copy_size, offset_mini_start - offset_mini_stop);
	} else {
		memcpy(seq_copy, seq + byte_mini_stop, seq_bytes - byte_mini_stop);
	}

	// Merge the shifted suffix with the prefix to compact the sequence
	seq[byte_mini_start] = fusion8(seq[byte_mini_start], seq_copy[0],offset_mini_start);
	for (uint64_t i=1 ; i<seq_bytes-byte_mini_stop ; i++) {
		seq[byte_mini_start+i] = seq_copy[i];
	}

	// Write the compacted sequence into file.
	this->write_compacted_sequence_without_mini(seq, seq_size-m, mini_pos, data_array);

	delete seq_copy;
}

uint64_t Section_Minimizer::read_compacted_sequence(uint8_t* seq, uint8_t* data) {
	// Read the block
	uint64_t mini_pos;
	uint64_t nb_kmers_in_block = this->read_compacted_sequence_without_mini(seq, data, mini_pos);
	
	uint64_t seq_size = nb_kmers_in_block + k - 1;
	uint64_t seq_bytes = bytes_from_bit_array(2, seq_size);
	uint64_t size_no_mini = seq_size - m;
	uint64_t bytes_no_mini = bytes_from_bit_array(2, size_no_mini);
	leftshift8(seq, bytes_no_mini, (8 - size_no_mini * 2) % 8);

	// Compute usefull quantities
	uint64_t bit_mini_start = 2 * mini_pos;
	uint64_t byte_mini_start = bit_mini_start / 8;
	uint8_t offset_mini_start = bit_mini_start % 8;

	uint64_t bit_mini_stop = bit_mini_start + 2 * m;
	uint64_t byte_mini_stop = bit_mini_stop / 8;
	uint8_t offset_mini_stop = bit_mini_stop % 8;

	// Copy the suffix to shift it
	uint64_t size_suffix = size_no_mini - mini_pos;
	uint64_t suffix_bytes = bytes_from_bit_array(2, size_suffix+1);
	uint8_t * suffix = new uint8_t[suffix_bytes + 1];

	// Add the suffix
	uint64_t nb_bytes_suffix_used = suffix_bytes + 1;
	if (offset_mini_start < offset_mini_stop) {
		memcpy(suffix, seq+byte_mini_start, suffix_bytes);
		rightshift8(suffix, suffix_bytes+1, offset_mini_stop-offset_mini_start);
	} else if (offset_mini_start > offset_mini_stop) {
		memcpy(suffix+1, seq+byte_mini_start, suffix_bytes);
		leftshift8(suffix, suffix_bytes+1, offset_mini_start-offset_mini_stop);
	} else {
		memcpy(suffix, seq+byte_mini_start, suffix_bytes+1);
		nb_bytes_suffix_used -= 1;
	}

	seq[byte_mini_stop] = fusion8(seq[byte_mini_stop], suffix[0], offset_mini_stop);
	for (auto i=1 ; i<nb_bytes_suffix_used ; i++) {
		seq[byte_mini_stop+i] = suffix[i];
	}

	// Prepare the minimizer
	uint8_t * mini_shifted = new uint8_t[this->nb_bytes_mini+1];
	memcpy(mini_shifted, this->minimizer, this->nb_bytes_mini);
	uint64_t mini_offset = (8 - ((2 * m) % 8)) % 8;
	leftshift8(mini_shifted, this->nb_bytes_mini, mini_offset);
	rightshift8(mini_shifted, this->nb_bytes_mini+1, offset_mini_start);
	// Add the minimizer inside the sequence
	seq[byte_mini_start] = fusion8(seq[byte_mini_start], mini_shifted[0], offset_mini_start);
	for (auto i=1 ; i<byte_mini_stop-byte_mini_start ; i++) {
		seq[byte_mini_start+i] = mini_shifted[i];
	}

	seq[byte_mini_stop] = fusion8(mini_shifted[this->nb_bytes_mini], seq[byte_mini_stop], offset_mini_stop);

	rightshift8(seq, seq_bytes, (8 - 2 * (seq_size % 4)) % 8);

	delete suffix;
	delete mini_shifted;
	return nb_kmers_in_block;
}

void Section_Minimizer::close() {
	if (file->is_writer) {
		// Save current position
		fstream &	 fs = this->file->fs;
		long position = fs.tellp();
		// Go write the number of variables in the correct place
		fs.seekp(this->begining + 1 + this->nb_bytes_mini);
		write_value(nb_blocks, fs);
		fs.seekp(position);
	}
}


















