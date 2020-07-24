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
For example the Byte 00101101 means that in the 2 bits encoding A=0, T=1, C=2, G=3.
The 4 values need to be different.
* free_size: The size of the next field in Bytes (4 Bytes)
* free_block: A field for additional metadata. Only use it for basic metadata and user comments.
If you need more section types, please contact us to extend the file format in a parsable and consistent way.

Here is an example header:

```
0204          : Version number 2 x 1 Byte. Here version 2.4
2d            : Encoding 0x2d == 0b00101101, so A=0, C=2, G=3, T=1
0000000c      : 12 Bytes in the free section
48656c6c6f20  : ascii -> "Hello "
776f726c6421  : ascii -> "world!"
```

# Sections

The sections can of 4 different types that are global number definitions, raw data section, minimizer section or grammar section (not yet described).
The first Byte of each section is a char to define the type (n, r, m or g respectively).

## Global number declaration

These sections can be seen as a zone of global scope variable definition.
The numbers are pairs of name/value where names are plain text ended with a '\0' character and values are 64 bits numbers.
For example, this section is used to define a minimizer size value that is used for the minimizer sections.
A list of needed values for other sections is given in their documentation.

Section values:
* type: char 'n' (1 Byte)
* nb_vars: The number of numbers declared in this section (8 Bytes).
* vars: A succession of nb_vars number structures as follow:
  * name: Plain text name ended with '\0' (variable size).
  * value: 64 bits value (8 Bytes).

## Section: raw sequences 

These sections are representing compacted sequences of raw kmers and the data linked to them.
Each sequence and its data are defined as a block.
The sequences are represented in a compacted way with 2 bits per nucleotide.

Global variable needed:
* k: the kmer size for this section.
* max: The maximum **number of kmer** per block.
* data_size: The max size of a piece of data for one kmer.
Can be 0 for "no data".

Section values:
* type: char 'r' (1 Byte)
* nb_blocks: The number blocks in this section (4 Bytes).
* blocks:
  * n: The number of kmers stored in the block. Must be <= max (lg(max) bits).
If max have been set to 1 in the header, this value is absent (save 1 Byte per block).
  * seq: The DNA sequence using 2 bits / nucleotide with respect to the encoding set in the header. It must be composed of n+k-1 characters.
  * data: An array of n\*data_size bits containing the data associated with each kmer (empty if data_size=0).


## Section: sequences with minimizers 

Most of the kmer technics take advantage of minimizers to partition the kmer space into smaller buckets.
The minimizers are shared between multiple kmers/sequences ans we can take advantage of that to decrease space usage.
This section represent a set of sequences that share the same minimizer.

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
* blocks:
  * n: The size of the sequence (without minimizer) stored in the block. Must be <= max (lg(max) bits).
If max have been set to 1 in the header, this value is absent (save 1 Byte per block).
  * m_idx: The index of the minimizer in the sequence (lg(max) bits).
  * seq: The DNA sequence without the minimizer using 2 bits / nucleotide with respect to the encoding set in the header (with a padding to fill a Byte multiple). It must be composed of n+k-1-m characters.
  * data: An array of n\*data_size bits containing the data associated with each kmer (empty if data_size=0).


## Section: sequences as context free grammar (not used yet)

This section is still under intense reflection on how to represent it succinctly using advantage of genomic texts.
If you have any suggestions or papers to read, please use the issues to tell us.
