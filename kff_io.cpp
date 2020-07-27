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

void Kff_file::set_encoding(uint8_t a, uint8_t c, uint8_t g, uint8_t t) {
	// Value masking
	a &= 0b11; c &= 0b11; g &= 0b11; t &= 0b11;

	// Verification of the differences.
	assert(a != c); assert(a != g); assert(a != t);
	assert(c != g); assert(g != t);
	assert(g != t);
}
