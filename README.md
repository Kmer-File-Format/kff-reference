# KFF 1 - Kmer File Format v1

This repository defines a universal file format to store DNA kmers and their associated data.
The associated data for a kmer can be anything as long as the size is constant across kmers.
For example, an integer count between 0 and 255 can be stored in a byte associated to each kmer.

For I/O APIs and tools, please have a look at the [KFF organization repositories](https://github.com/Kmer-File-Format)


# Overview

A KFF file is composed of two parts (and 2 watermarks):
* Watermark 1: 3 bytes "KFF" for file integrity checks.
* **Header**: defines some constants.
 
For example, it contains the file format version and the binary encoding of nucleotides.

* **List of sections**

As described in the following parts, there are multiple types of sections and most of them are representing sequences in different ways.
Each representation has its own set of advantages.
For instance, overlapping kmers can be represented as sequences to save space.
For example with k=3, a sequence ACTTG will represent the set of kmers {ACT, CTT, TTG}.

* Watermark 2: 3 bytes "KFF" for file integrity checks.


## Byte alignment

For faster processing, all values are stored on multiples of 8 bits to match a byte.
So, if a DNA sequence needs 12 bits to be represented, 16 bits will be used and the 4 most significant bits can be set arbitrarily.
All the values (sequences and integers) are stored as big endian.


## Convention

In the diagrams below, we will use the following graphical elements:
```
     +---+
     |   |   one byte  
     +---+
     
     +--+--+
     |     |  two bytes
     +--+--+
     
     +--+--+--+--+
     |           |  four bytes (etc..)
     +--+--+--+--+
     
     +==============+
     |              |  a variable number of bytes
     +==============+

     +=====================+
     | ascii(Hello world!) |  the ascii representation of a string (here, "Hello world!") plus a null character
     +=====================+

     +=============+
     | 2bit(ACCTG) |   a DNA sequence (here, ACCTG) in 2 bit encoding, big endian byte order.
     +=============+
```

# Header

The file header defines values that are valid for the whole file.
Some variables, such as k, are not defined in the header but in a section (called 'global values declarations').
That way, if multiple k values are used, the file can redefine k on the fly between sections.


Header structure (required elements):
* marker: The 3 letters "KFF" to know that we a reading a KFF file.
* version: the file format version x.y where x is the first byte and y is the second byte (total: 2 bytes)
* encoding: how A, C, G, and T are encoded (total: 2 bits/nucl - 1 byte).
For example encoding=00101101 means that A=0, C=2, G=3, T=1. The 4 values need to be different.
* unique kmers: A Byte set to 0 if you can encounter multiple time the same kmer, any other value if all the kmers are unique.
* canonical kmers: A Byte set to 0 if you can encounter a kmer and its reverse complement in the same file, any other value if the file contains only one of the two.
* free_size: The size of the next field in bytes (total: 4 bytes)
* free_block: A field for additional metadata. Only use it for basic metadata and user comments.
If you need more section types, please contact us to extend the file format (for a future version) in a parsable and consistent way.

```
+---------------+---------------+----------+--+--+--+--+============+
| Major version | Minor version | Encoding | Free size | Free block |
+---------------+---------------+----------+--+--+--+--+============+
```

Example:
```
+----+----+----+--+--+--+--+=====================+
| 02 | 04 | 2d | 0000000d  | ascii(Hello world!) |
+----+----+----+--+--+--+--+=====================+
```

* Version number is 2.4
* Encoding 0x2d == 0b00101101, so A=0, C=2, G=3, T=1
* 12 Bytes in the free section
* ascii -> "Hello world!"

# Sections

The main part of the file is a succession of sections.
These sections can be of different types.
The most important two are the 'values declarations' and the 'raw data' sections.
Other sections are used in particular contexts to store sequences more efficiently than raw data sections or to augment the file with more information (like an index).

The first byte of each section defines its type.

## Section: value declarations

This type of section can be seen as a scope that ends with the next value section, where we define values.
The other sections need the definition of some variables (the k value for example).
A list of needed values for other sections is given in this specification.
Each value definition is a (name,value) pair where a name is a ASCII text ending with a '\0' character, and a value is a 64 bits field.
After the end of the score, each value is reset to undefined.

Section contents:
* type: char 'v' (1 Byte)
* nb_vars: The number of values declared in this section (8 bytes).
* vars: A succession of nb_vars pairs composed as follow:
  * name: Plain text name ended with '\0' (variable size).
  * value: 64 bits value (8 bytes).

```
+-------+--+--+--+--+--+--+--+--+========+--+--+--+--+--+--+--+--+===+=========+--+--+--+--+--+--+--+--+
| type  |         nb_vars       | name 1 |         value 1       | … | name X |         value X        |
+-------+--+--+--+--+--+--+--+--+========+--+--+--+--+--+--+--+--+===+=========+--+--+--+--+--+--+--+--+
```

Example:

```
+----------+--+--+--+--+--+--+--+--+==========+--+--+--+--+--+--+--+--+============+--+--+--+--+--+--+--+--+==================+--+--+--+--+--+--+--+--+
| ascii(v) |            3          | ascii(k) |          A            | ascii(max) |           ff          | ascii(data_size) |             1         |
+----------+--+--+--+--+--+--+--+--+==========+--+--+--+--+--+--+--+--+============+--+--+--+--+--+--+--+--+==================+--+--+--+--+--+--+--+--+
```

* ascii(v) -> declare a value section
* 3 -> declare 3 values
  * ascii(k) -> name 1
  * 10 -> "k" value
  * ascii(max) -> name 2
  * 255 -> "max" value
  * ascii(data_size) ->  name 3
  * 1 -> "data_size" value

## Section: raw sequences

This section is composed of a list of sequence/data pairs.
A sequence S of size s represents a list of n kmers where n=s-k+1.
The data linked to S is of size data_size * n.
We call each of these pairs a block.
The sequences are represented in a compacted way with 2 bits per nucleotide.

Values requirement:
* k: the kmer size for this section.
* max: The maximum **number of kmers** per block.
* data_size: The max size (in bytes) of a piece of data for one kmer.
Can be 0 for "no data".

Section contents:
* type: char 'r' (1 Byte)
* nb_blocks: The number blocks in this section (8 Bytes).
* blocks: A list of blocks where each block is composed as follow:
  * n: The number of kmers stored in the block. Must be <= max, (field size: lg(max) bits).
If max have been set to 1 in the header, this value is absent (save 1 Byte per block).
  * seq: The DNA sequence using 2 bits / nucleotide with respect to the encoding set in the header. It must be composed of n+k-1 characters.
  * data: An array of n\*data_size bits containing the data associated with each kmer (empty if data_size=0).

Plain text example (k=10, max=255, data_size=1 (counts), A=0, C=2, G=3, T=1):
```
ACTAAACTGATT [32, 47, 1] : sequence translation 0b001001000000100111000101 or 0x2409c5
AAACTGATCG [12]          : sequence translation 0b00000010011100011011 or 0x0271b
CTAAACTGATT [1, 47]      : sequence translation 0b1001000000100111000101 or 0x2409c5
```

Same example translated as a raw sequence section:
```
+----------+--+--+--+--+--+--+--+--+---+===+====================+========+---+====================+====+---+===================+===========+
| ascii(r) |            3              | 3 | 2bit(ACTAAACTGATT) | 202f01 | 1 | 2bit(AAACTGATCG)   | 0c | 2 | 2bit(CTAAACTGATT) |    012f   |
+----------+--+--+--+--+--+--+--+--+---+===+====================+========+---+====================+====+---+===================+===========+
```

* ascii(r) -> declare a raw sequences section
* 3 blocks declared:
  * 3: block 1 - 3 kmers declared
  * 2bit(ACTAAACTGATT): block 1 - sequence ACTAAACTGATT
  * 202f01: block 1 - counters [32, 47, 1]
  * 1: block 2 - 1 kmers declared
  * 2bit(AAACTGATCG): block 2 - sequence AAACTGATCG (+4 bits padding)
  * 0c: block 2 - counters [12]
  * 2: block 3 - 2 kmers declared
  * 2bit(CTAAACTGATT): block 3 - sequence CTAAACTGATT (+2 bits padding)
  * 012f: block 3 - counters [1, 47]


## Section: sequences with minimizers

In many applications, kmers are bins using minimizer technics.
It means that a succession of kmers or superkmers share a common substring.
In this file format, in the case where you know sets of sequences that share this kind of substring, you can use minimizer sections.
The common minimizer is stored at the beginning of the section and deleted in the sequences.
A index is joined to the sequences to recall the minimize position and be able to reconstruct all the kmers.

Values needed:
* k: the kmer size for this section.
* m: the minimizer size.
* max: The maximum **number of kmer** per block.
* data_size: The max size (in Bytes) of a piece of data for one kmer.
Can be 0 for "no data".

Section contents:
* type: char 'm' (1 Byte)
* mini: The sequence of the minimizer 2 bits/char (lg(m) bits).
* nb_blocks: The number blocks in this section (8 Bytes).
* blocks: A list of blocks where each block is composed as follow:
  * n: The number of kmers stored in the block. Must be <= max, (field size: lg(max) bits).
If max have been set to 1 in the header, this value is absent (save 1 Byte per block).
  * m_idx: The index of the minimizer in the sequence (min(lg(k+max-1), 64) bits).
  * seq: The DNA sequence without the minimizer using 2 bits / nucleotide with respect to the encoding set in the header (with a padding to fill a Byte multiple). It must be composed of n+k-1-m characters (2\*(n+k-1-m) bits).
  * data: An array of n\*data_size bits containing the data associated with each kmer (empty if data_size=0).

Plain text example (k=10, m=8, max=255, data_size=1 (counts), A=0, C=2, G=3, T=1):
```
ACTAAACTGATT [32, 47, 1] : sequence translation 0b001001000000100111000101 or 0x2409c5
AAACTGATCG [12]          : sequence translation 0b00000010011100011011 or 0x0271b
CTAAACTGATT [1, 47]      : sequence translation 0b1001000000100111000101 or 0x2409c5
```

Same example translated as a minimizer sequence section:
```
+----------+================+--+--+--+--+--+--+--+--+--+===+===+============+========+===+===+==========+====+===+===+===========+======+
| ascii(m) | 2bit(AAACTGAT) |             3            | 3 | 3 | 2bit(ACTT) | 202f01 | 1 | 0 | 2bit(CG) | 0c | 2 | 2 | 2bit(CTT) | 012f |
+----------+================+--+--+--+--+--+--+--+--+--+===+===+============+========+===+===+==========+====+===+===+===========+======+
```

* ascii(m) -> declare a minimizer section
* 2bit(AACTGAT) -> minimizer of this block is AACTGAT
* 3 blocks declared:
  * 3: block 1 - 3 kmers declared
  * 3: block 1 - minimizer at index 3
  * 2bit(ACTT): block 1 - blocks sequence without minimizer
  * 202f01: block 1 - counters [32, 47, 1]
  * 1: block 2 - 1 kmers declared
  * 0: block 2 - minimizer at index 0
  * 2bit(CG): block 2 - blocks sequence without minimizer
  * 0c: block 2 - counters [12]
  * 2: block 3 - 2 kmers declared
  * 2: block 3 - minimizer at index 2
  * 2bit(CTT): block 3 - blocks sequence without minimizer
  * 012f: block 3 - counters [1, 47]

This small example have been reduced in size from 23 bytes to 22 bytes using minimizer blocks instead of raw blocks.



## Section: index

An index section will remind the relative positions of a certain amount of other sections.
The index also keep as last value, the position of the next index section (0 if last).
A file is called indexed when each section position is present in one index section.
Because of the index pointer to the next index, the full index of a file can be distributed along the file.

Values needed: None.

Section contents:
* type: char 'i' (1 Byte)
* nb_sections: The number of section indexed (8 Bytes).
* index_pairs: A list of nb_sections pairs (section, position):
  * section: The type of section (1 Byte).
  * position: Relative position to the end of this index section (8 Bytes). The value 0 means that the section starts at the first byte after the current one. Position values can be negative.
* next_index: Relative position of the next index section (8 Bytes). Cannot be the following byte. Here 0 means "no following index section".

Example:
```
+----------+--+--+--+--+--+--+--+--+----------+--+--+--+--+--+--+--+--+----------+--+--+--+--+--+--+--+--+----------+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
| ascii(i) |           3           | ascii(v) |           0           | ascii(m) |          243          | ascii(r) |         -1463         |          3265         |
+----------+--+--+--+--+--+--+--+--+----------+--+--+--+--+--+--+--+--+----------+--+--+--+--+--+--+--+--+----------+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
```

* ascii(i) -> declare an index section
* 3 sections indexed:
  * ascii(v): index 1: Value section
  * 0:        index 1: The section starts 0 byte after the end of the index.
  * ascii(m): index 2: Minimizer section
  * 243:      index 2: The section starts 243 byte after the end of the index.
  * ascii(r): index 3: Raw sequence section
  * -1463:    index 3: The section starts 1463 byte BEFORE the end of the index (-1 is the last byte of this section).
* 3265: Relative position of the next index block.


# Good practices

Here we introduce good practices that are recommended but not mandatory.

## Footer

When we are looking for kmer sets, we often need some global statistics and we would like to have them without parsing a whole KFF file.
For example, we can need the number of distinct kmer inside of the file.
We recommend to add such statistics in a footer 'v' section.
To know where this footer starts, the last value must be "footer_size" and its corresponding value.

To read the statistics, go to 23 Bytes before the end of the file [3 (watermark) + 8 (value size) + 12 (footer_size string size)].
Then read the 12 bytes name of the value and if it corresponds to "footer_size", read the 8 bytes following value.
Finally go back of this value + 3 (watermark) from the end of the file.
You can now read the footer.

## First index block

If your KFF file is fully index a good practice is to have the first section of the file that is a index section.
If it is not possible or not practical in your case, you also can indicate its position in the file declaring a value "first_index" in a footer.
