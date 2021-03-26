# KFF 1 - Kmer File Format v1

This repository defines a universal file format to store DNA kmers and their associated data.
The associated data for a kmer can be anything as long as the size is constant across kmers.
For example, an integer count between 0 and 255 can be stored in a byte associated to each kmer.

For I/O APIs and tools, please have a look at the [KFF organization repositories](https://github.com/Kmer-File-Format)


# Overview

A KFF file is composed of two parts:
* **Header**: defines some constants.
 
For example, it contains the file format version and the binary encoding of nucleotides.

* **List of sections**

As described in the following parts, there are multiple types of sections and most of them are representing sequences in different ways.
Each representation has its own set of advantages.
For instance, overlapping kmers can be represented as sequences to save space.
For example with k=3, a sequence ACTTG will represent the set of kmers {ACT, CTT, TTG}.

## Byte alignment

For faster processing, all values are stored on multiples of 8 bits to match a byte.
So, if a DNA sequence needs 12 bits to be represented, 16 bits will be used and the 4 most significant bits can be set arbitrarily.


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
Some variables, such as k, are not defined in the header but in a section (called 'global variables declarationss').
That way, if multiple k values are used, the file can redefine k on the fly between sections.


Header structure (required elements):
* version: the file format version x.y where x is the first byte and y is the second byte (total: 2 bytes)
* encoding: how A, C, G, and T are encoded (total: 2 bits/nucl - 1 byte).
For example encoding=00101101 means that A=0, C=2, G=3, T=1. The 4 values need to be different.
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
| 02 | 04 | 2d | 0000000c  | ascii(Hello wordl!) |
+----+----+----+--+--+--+--+=====================+
```

* Version number is 2.4
* Encoding 0x2d == 0b00101101, so A=0, C=2, G=3, T=1
* 12 Bytes in the free section
* ascii -> "Hello world!"

# Sections

The main part of the file is a succession of sections.
These sections can be of different types.
The most important two are the 'global variables declarations' and the 'raw data' sections.
Other sections are used in particular contexts to store sequences more efficiently than raw data sections.

The first byte of each section defines its type.

## Section: global variables declarations

This type of section can be seen as a zone of global-scope variables definitions.
The other sections need the definition of some variables (the k value for example).
A list of needed values for other sections is given in this specification.
Each variable is a (name,value) pair where a name is a ASCII text ending with a '\0' character, and a value is a 64 bits field.
A variable can be redefined within the same file. Its value will then be the last one encountered.

Section contents:
* type: char 'v' (1 Byte)
* nb_vars: The number of numbers declared in this section (8 bytes).
* vars: A succession of nb_vars number composed as follow:
  * name: Plain text name ended with '\0' (variable size).
  * value: 64 bits value (8 bytes).

```
+-------+--+--+--+--+--+--+--+--+===========+--+--+--+--+--+--+--+--+===+============+--+--+--+--+--+--+--+--+
| type  |         nb_vars       | name var1 |       value var1      | … | name var X |      value var X      |
+-------+--+--+--+--+--+--+--+--+===========+--+--+--+--+--+--+--+--+===+============+--+--+--+--+--+--+--+--+
```

Example:

```
+----------+--+--+--+--+--+--+--+--+==========+--+--+--+--+--+--+--+--+============+--+--+--+--+--+--+--+--+==================+--+--+--+--+--+--+--+--+
| ascii(v) |             3         | ascii(k) |          A            | ascii(max) |           ff          | ascii(data_size) |             1         |
+----------+--+--+--+--+--+--+--+--+==========+--+--+--+--+--+--+--+--+============+--+--+--+--+--+--+--+--+==================+--+--+--+--+--+--+--+--+
```

* ascii(v) -> declare a globad variable section
* 3 -> declare 3 variable
  * ascii(k) -> name var 1
  * 10 -> value of variable k
  * ascii(max) -> name var2
  * 255 -> value of variable max
  * ascii(data_size) ->  name var3
  * 1 -> value of variable data_size

## Section: raw sequences

This section is composed of a list of sequence/data pairs.
A sequence S of size s represents a list of n kmers where n=s-k+1.
The data linked to S is of size data_size * n.
We call each of these pairs a block.
The sequences are represented in a compacted way with 2 bits per nucleotide.

Global variables requierment:
* k: the kmer size for this section.
* max: The maximum **number of kmers** per block.
* data_size: The max size (in bytes) of a piece of data for one kmer.
Can be 0 for "no data".

Section contents:
* type: char 'r' (1 Byte)
* nb_blocks: The number blocks in this section (4 Bytes).
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
+----------+--+--+--+--+---+===+====================+========+---+====================+====+---+===================+===========+
| ascii(r) |      3        | 3 | 2bit(ACTAAACTGATT) | 202f01 | 1 | 2bit(AAACTGATCG)   | 0c | 2 | 2bit(CTAAACTGATT) |    012f   |
+----------+--+--+--+--+---+===+====================+========+---+====================+====+---+===================+===========+
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

Global variables needed:
* k: the kmer size for this section.
* m: the minimizer size.
* max: The maximum **number of kmer** per block.
* data_size: The max size (in Bytes) of a piece of data for one kmer.
Can be 0 for "no data".

Section contents:
* type: char 'm' (1 Byte)
* mini: The sequence of the minimizer 2 bits/char (lg(m) bits).
* nb_blocks: The number blocks in this section (4 Bytes).
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
+----------+================+--+--+--+--+--+===+===+============+========+===+===+==========+====+===+===+===========+======+
| ascii(m) | 2bit(AAACTGAT) |       3      | 3 | 3 | 2bit(ACTT) | 202f01 | 1 | 0 | 2bit(CG) | 0c | 2 | 2 | 2bit(CTT) | 012f |
+----------+================+--+--+--+--+--+===+===+============+========+===+===+==========+====+===+===+===========+======+
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
