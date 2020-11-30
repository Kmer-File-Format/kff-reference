# C++ lib for i/o kff files

Welcome in the C++ library description for Reading or Writing kff files.
This code is not guaranty to be 100% bug-free, so please submit issues if encounter one.

The library is divided into a low level and a high level APIs.
High level API is designed to be easy to use by hiding the section aspect of the file.
The low level API let you read and write sections as you want.
You are entirely free of your kmer organization inside of the file.

The lib is not yet optimized, so it can be slow for big files.
Also, the low level API does not guaranty the file integrity.
It means that if you use the functions in the wrong order, you can write wrong kff files.


# High level reader API

Documentation not yet available


# Low level API

## Open/close a kff file

Kff_file is the main object in the library.
This is the object needed to manipulate a binary kff file.

```C++
  // Open a kff file to read it
  Kff_file infile("path/to/file/myfile.kff", "r");
  // [...]
  infile.close();

  // Open a kff file to write inside of it.
  // Path/to/file/ must exist prior to opening.
  Kff_file outfile("path/to/file/myfile.kff", "w");
  // [...]
  outfile.close();
```

## File reader

### Header

When you open a file to read it, the software version is automatically read and compared to the file version.
The software version must be equal or greater than the file version.

The encoding of the nucleotides is also automatically read.
It is accessible via the file property *encoding*.

```C++
  // Nucleotide encoding
  uint8_t encoding[4] = infile.encoding;
```

Finally the metadata size is automatically read but not the metadata content.
The size is available via the the file property *metadata_size*.
You can either read the metadata content as follow or let the file skip it when you read the first section.

```C++
  // Get the metadate size
  uint32_t size = infile.metadata_size;
  // Allocate memory and get the metadata content
  uint8_t * metadata = new uint8_t[size];
  infile.read_metadata(metadata);
  // Use metadata [...]
  delete[] metadata;
```

### Detect section

Each section start with a special char.
You can use *read_section_type* to get this char and then use the dedicated function to open the corresponding section.

```C++
  char type = infile.read_section_type();
```

### Global variable section

When 'v' char is detected, you can call the file to open a Section_GV.
The creation of the section on a file will automatically read the variables inside of the section.
The variables are accessible as a std::map<string, uint64_t> with the public section property vars or inside the file property global_vars.

```C++
  // Open the section (automatically read the variables)
  Section_GV sgv = infile.open_section_GV();
  // Read the variables from the section map
  for (auto it : sgv.vars)
    std::cout << it.first << ": " << it.second << std::endl;
  // Read the variables from the global map
  for (auto it : infile.global_vars)
    std::cout << it.first << ": " << it.second << std::endl;
```

### Raw sequences section

When 'r' char is detected, you can call the file to open a Section_Raw.
At the section creation, the number of blocks inside of it is automatically read and stored in the property *nb_blocks*.
The number of remaining blocks in the section is also available in the property *remaining_blocks*.

```C++
  // Get the global variables needed
  uint64_t k = infile.global_vars["k"];
  uint64_t max = infile.global_vars["max"];
  uint64_t ds = infile.global_vars["data_size"];

  // Open the raw section
  Section_Raw sr = infile.open_section_raw();
  cout << sr.remaining_blocks << "/" << sr.nb_blocks << endl;

  // Prepare buffers for sequences and data
  uint8_t * sequence_buffer = new uint8_t[(k + max -1) / 4 + 1];
  uint8_t * data_buffer = new uint8_t[max * ds];

  // Read all the blocks of the section
  for (uint64_t i=0 ; i<sr.nb_blocks ; i++) {
    uint64_t nb_kmers = sr.read_compacted_sequence(sequence_buffer, data_buffer);
    // [...] Use sequences and data
    cout << sr.remaining_blocks << "/" << sr.nb_blocks << endl;
  }

  // Close the raw section
  delete[] sequence_buffer;
  delete[] data_buffer;
  sr.close();
```

**NB**: You can close the section even if you did not read all the blocks of it.
It is triggering a function that will skip the end of the section.


**Warning**: You can directly interact with the file pointer under the hood of Kff_file objects.
If you do so, variables like *remaining_blocks* and automatic readings procedures on section closing can become wild.
So, please don't rely on them after a direct file pointer usage or manually update them.


### Minimizer sequences section

When 'm' char is detected, you can call the file to open a Section_Minimizer.
This section is very similar from the raw section.
In fact, you can use the exact same code after the opening.
But where raw blocks reading function uses direct file reading, the one here hide more computation.
This computation overhead is due to the fact that minimizer are not stored inside of the blocks but at the beginning of the section.

To avoid the computation, you can directly read sequences using minimizer section specialized function that read the sequence without the minimizer.
The minimizer is accessible in the section attribute *minimizer*.

```C++
  // [...] Same previous variables
  uint64_t m = infile.global_vars["m"];

  // Open the minimizer section
  Section_Minimizer sm = infile.open_section_minimizer();
  uint8_t * mini = new uint8_t[m / 4 + 1];
  memcpy(mini, sm.minimizer, m%4==0 ? m/4 : m/4+1);
  // [...] Use the minimizer

  // Read all the sequences
  for (uint64_t i=0 ; i<sm.nb_blocks ; i++) {
    uint64_t minimizer_position;
    uint64_t nb_kmers = sm.read_compacted_sequence_without_mini(sequence_buffer, data_buffer, minimizer_position);
    // [...] Use sequence and data
    cout << "minimizer position: " << minimizer_position << endl;
  }

  // Close the section (and detroy the section minimizer memory!)
  sm.close();
  delete[] mini;
```

**Warning**: The minimizer memory is deleted on section closing.
So, if needed, remember to copy the value before.


### Reading section

Because Raw sections and Minimizer sections share a lot of their design for reading functions, they both inherit from a class called *Block_section_reader*.
This class contains all the properties and functions described in the raw sequence section part.

These sections are created using a static function that returns a nullptr if the file cannot recognize a raw or minimizer section.

```C++
  // Construct a block reader
  Block_section_reader * br = Block_section_reader::construct_section(infile);

  if (br == nullptr) {
    std::cerr("Next section is not a sequence section");
  } else {
    // [...] Do stuff here
  }
```

### Skipping blocks or sections


## File writer



