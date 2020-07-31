#include <iostream>
#include <cassert>
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
	// Save current position
	fstream &	 fs = this->file->fs;
	long position = fs.tellp();
	// Go write the number of variables in the correct place
	fs.seekp(this->begining + 1);
	write_value(nb_vars, fs);
	fs.seekp(position);
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

	this->nb_blocks += 1;
}

uint64_t Section_Raw::read_compacted_sequence(uint8_t* seq, uint8_t* data) {
	uint64_t nb_kmers_in_block = 0;
	// 1 - Read the number of kmers in the sequence
	file->fs.read((char*)&nb_kmers_in_block, this->nb_kmers_bytes);
	// 2 - Read the sequence
	size_t seq_size = nb_kmers_in_block + file->global_vars["k"] - 1;
	size_t nb_bytes = bytes_from_bit_array(2, seq_size);
	file->fs.read((char*)seq, nb_bytes);
	// 3 - Read the data
	return nb_kmers_in_block;
}

void Section_Raw::close() {
	// Save current position
	fstream &	 fs = this->file->fs;
	long position = fs.tellp();
	// Go write the number of variables in the correct place
	fs.seekp(this->begining + 1);
	write_value(nb_blocks, fs);
	fs.seekp(position);
}


















