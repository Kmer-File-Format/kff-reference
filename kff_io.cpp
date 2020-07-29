#include <iostream>
#include <cassert>

#include "kff_io.hpp"

using namespace std;


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
	cout << "kff file " << filename << " opened in mode " << mode << endl;

	// Write the version at the beginning of the file
	if (this->is_writer) {
		this->fs << (char)KFF_VERSION_MAJOR << (char)KFF_VERSION_MINOR;
	}

	// Read the header
	else if (this->is_reader) {
		this->fs >> this->major_version >> this->minor_version;
		cout << "version " << (uint64_t)this->major_version << "." << (uint64_t)this->minor_version << endl;

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

void Kff_file::write_metadata(size_t size, char * data) {
	this->fs << (uint32_t)size;
	this->fs.write(data, size);
}

uint32_t Kff_file::read_metadata(char * data) {
	uint32_t meta_size;
	this->fs >> meta_size;

	this->fs.read(data, meta_size);

	return meta_size;
}

















