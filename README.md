# KFF 1 - Kmer File Format v1

This repository define a universal file format to store DNA kmers and their associated data.
The associated data for a kmer can be everything if its size is constant from a kmer to another.
For example, a count with a maximum of 255 can be store on a byte associated to each kmer.


# Overview

The file is composed of two parts:
* **A header**: It describes the different constant values.
For example, it contains the file format version of the nucletide encoding.
* **A content part**: It is composed of a list of sections.
As described in the following parts, there are multiple type of sections and most of them are representing sequences in different ways.
Each representation have compaction advantages.
Overlapping kmers are represented as compacted sequences to save space.
For example with k=3, a sequence ACTTG will represent the set of kmer {ACT, CTT, TTG}.

## Byte multiple

To be fast to read and write, all the values are stored on multiple of 8 bits to match a Byte.
So, if a DNA sequence needs 12 bits to be represented, 16 will be used and the 4 most significant bits will be set randomly.


# File header

The file header define values that are shared by the whole file regardless of the kmer set.
Some values as k are not defined in the header but in the global number declaration sections.
In that way, if multiple k values are used, the file can redefine it on the fly.

List of the values needed in the header:
* version: the file format version x.y where x is the first byte y the second (2 Bytes)
* encoding: ACGT encoding (2 bits/nucl - 1 Byte).
For example the Byte 00101101 means that in the 2 bits encoding A=0, C=2, G=3, T=1.
The 4 values need to be different.
* free_size: The size of the next field in Bytes (4 Bytes)
* free_block: A field for additional metadata. Only use it for basic metadata and user comments.
If you need more section types, please contact us to extend the file format in a parsable and consistent way.

Example:

```
0204              : Version number 2 x 1 Byte. Here version 2.4
2d                : Encoding 0x2d == 0b00101101, so A=0, C=2, G=3, T=1
0000000c          : 12 Bytes in the free section
48656c6c 6f20776f 
726c6421          : ascii -> "Hello world!"
```

# Sections

The main part of the file is a succession of sections.
These sections can be of different types.
The most important two are the global declaration of variables and the raw data sections.
Other sections are used in particular contexts to store sequences more efficiently than raw data sections.

The first Byte of each section define its type.

## Section: global variable declaration

This kind of section can be seen as a zone of global scope variable definition.
The other sections need the definition of some variables (the k value for example).
A list of needed values for other sections is given in their documentation.
The variables are pairs of name/value where names are ascii texts ended with a '\0' character and values are 64 bits fields.

Section values:
* type: char 'v' (1 Byte)
* nb_vars: The number of numbers declared in this section (8 Bytes).
* vars: A succession of nb_vars number composed as follow:
  * name: Plain text name ended with '\0' (variable size).
  * value: 64 bits value (8 Bytes).

Example:

```
76                : 'v' -> declare a global variable section
00000000 00000003 : 2 variables following
6b00              : ascii name "k"
00000000 0000000A : k <- 10
6d617800          : ascii name "max"
00000000 000000ff : max <- 255
64617461 5f73697a
6500              : ascii name "data_size"
00000000 00000001 : data_size <- 1
```

## Section: raw sequences

This section is composed of a list of sequence/data pairs.
A sequence S of size s represents a list of n kmers where n=s-k+1.
The data linked to S is of size data_size * n.
We call each of these pairs a block.
The sequences are represented in a compacted way with 2 bits per nucleotide.

Global variable requierment:
* k: the kmer size for this section.
* max: The maximum **number of kmer** per block.
* data_size: The max size of a piece of data for one kmer.
Can be 0 for "no data".

Section values:
* type: char 'r' (1 Byte)
* nb_blocks: The number blocks in this section (4 Bytes).
* blocks: A list of blocks where each block is composed as follow:
  * n: The number of kmers stored in the block. Must be <= max (lg(max) bits).
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
72       : 'r' -> declare a raw sequences section
00000003 : 3 blocks declared
03       : block 1 - 3 kmers declared
2409c5   : block 1 - sequence ACTAAACTGATT
202f01   : block 1 - counters [32, 47, 1]
01       : block 2 - 1 kmer declared
00271b   : block 2 - sequence AAACTGATCG (+4 bits padding)
0c       : block 2 - counter [12]
02       : block 3 - 2 kmer declared
2409c5   : block 3 - sequence CTAAACTGATT (+2 bits padding)
012F     : block 3 - counters [1, 47]
```


## Section: sequences with minimizers

In many applications, kmers are bins using minimizer technics.
It means that a succession of kmers or superkmers share a common substring.
In this file format, in the case where you know sets of sequences that share this kind of substring, you can use minimizer sections.
The common minimizer is stored at the beginning of the section and deleted in the sequences.
A index is joined to the sequences to recall the minimize position and be able to reconstruct all the kmers.

Global variable needed:
* k: the kmer size for this section.
* m: the minimizer size.
* max: The maximum **size of a sequence** (without minimizer) per block.
* data_size: The max size of a piece of data for one kmer.
Can be 0 for "no data".

Section values:
* type: char 'm' (1 Byte)
* mini: The sequence of the minimizer 2 bits/char (lg(m) bits).
* nb_blocks: The number blocks in this section (4 Bytes).
* blocks: A list of blocks where each block is composed as follow:
  * n: The number of kmers stored in the block. Must be <= max (lg(max) bits).
If max have been set to 1 in the header, this value is absent (save 1 Byte per block).
  * m_idx: The index of the minimizer in the sequence (lg(max) bits).
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
6d       : 'm' -> declare a minimizer section
0271     : minimizer AAACTGAT
00000003 : 3 blocks declared
03       : block 1 - 3 kmers declared
03       : block 1 - minimizer at index 3
25       : block 1 - sequence ACTT
202f01   : block 1 - counters [32, 47, 1]
01       : block 2 - 1 kmer declared
00       : block 2 - minimizer at index 0
07       : block 2 - sequence CG (+4 bits padding)
0c       : block 2 - counter [12]
02       : block 3 - 2 kmers declared
02       : block 3 - minimizer at index 2
25       : block 3 - sequence CTT (+2 bits padding)
012F     : block 3 - counters [1, 47]
```

This small example have been reduced in size from 23 Bytes to 22 Bytes using minimizer blocks instead of raw blocks.

## Section: sequences as context free grammar (not used yet)

We would like to represent sequences as grammar to compact futher in some cases.
This section is still under construction.
