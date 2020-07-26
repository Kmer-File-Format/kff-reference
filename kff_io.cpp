#include <iostream>

#include "kff_io.hpp"

using namespace std;


Kff_file::Kff_file(const char * filename, const char * mode) {
	cout << "kff file " << filename << " openned in mode " << mode << endl;
}
