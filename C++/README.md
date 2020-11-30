# C++ lib for i/o kff files

Welcome in the C++ library description for Reading or Writing kff files.
This code is not guaranty to be 100% bug-free, so please submit issues if encounter one.

The library is divided into a low level and a high level APIs.
High level API is designed to be easy to use by hidding the section aspect of the file.
The low level API let you read and write sections as you want.
You are entirely free of your kmer organization inside of the file.
Nothing is optimized and a few things are checked, so when you write, you can produce wrong kff files.


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

Finaly the metadata size is automatically read but not the metadata content.
The size is available via the the file property *metadata_size*.
You can either read the metada content as follow or let the file skip it when you read the first section.

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
You can use *read_section_type* to get this char and then use the dedicated function to open the coresponding section.

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

## File writer



