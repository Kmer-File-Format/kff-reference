# KFF 1 - Kmer File Format v1

This readme define a universal file format to store DNA kmers and their associate data.
The associate data for a kmer can be everything if its size is constant from a kmer to another.
For example, a count with a maximum of 255 can be store on a byte associated to each kmer.


# Overview

The file is composed of two parts:
* **A header**: It describes the different constant values.
For example, it contains the k value and the size of kmer associated data.
* **A content part**: It is composed of a list of sections.
As described in the following parts, sections can be filled with raw data blocks or with grammar based blocks.
In each case the blocks contain information on compacted kmers and their associated data.
We say compacted kmers because more than one kmer can appear in a sequence.
For example with k=3, a block with ACTTG will represent the set of kmer {ACT, CTT, TTG}.

Need for a schema here

## Byte multiple

To be fast to read and write, all the values are stored on multiple of 8 bits to match a Byte.
So, if a DNA sequence needs 12 bits to be represented, 16 will be used and the 4 most significant bits will be set randomly.


# File header

Ordered values in the header:
* version: the file format version x.y where x is the first byte y the second (2 Bytes)
* k: The kmer size (1 Byte)
* encoding: ACGT encoding (2 bits/nucl - 1 Byte).
For example the Byte 00101101 means that in the 2 bits encoding A=0, T=1, C=2, G=3.
The 4 values need to be different.
* max: The maximum number of kmer allowed in one block (8 Bytes).
Max have to be non zero.
* data_size: The size of the data associated to each kmer (8 Bytes)
This value can be 0 if you don't want to associate any piece of data.
* free_size: The size of the next field in Bytes (4 Bytes)
* free_block: A field for additional metadata. Only use it for basic metadata and user comments.
If you need more sections, please contact us to extend the file format in a parsable way.

# Section

Ordered values in the section:
* type: the type of section. 'r' for raw, 'g' for grammar (1 Byte).
* If grammar block:
  * nt: The number of non terminal symbols for this section (up to 256 different) (1 Byte)
  * nt_blocks: Definition of the non terminal values.
The first nt described here has the identifiyer 0, the second, 1, etc.
* nb_blocks: the number of raw/grammar blocks that will follow in this section.

# Raw Block

Each raw block is composed of 3 values:
* n: The number of kmers stored in the block <= max (lg(max) bits (ceiled on a 8 multiple)).
If max have been set to 1 in the header, this value is absent (save 1 Byte per block).
* seq: The DNA sequence using 2 bits / nucleotide with respect to the encoding set in the header (with a padding to fill a Byte multiple)
* data: An array of n\*data_size bits containing the data associated with each kmer (empty if data_size=0).

# Grammar blocks

Composition:
* nb_t: The number of terminal characters in the block <= (max+k-1) (lg(max+k-1) bits (ceiled on a 8 multiple)).
* seq: The raw DNA sequence without the characters encoded in the non terminal symbols.
It's using 2 bits / nucleotide with respect to the encoding set in the header.
* nb_nt: The number of non terminal used in the block (1 Byte).
* nt_pos: A list of the non terminal used symbols and their positions in the raw sequence:
  * nt_id: Id of the nt used (1 Byte).
  * nt_pos: the position to insert it in the raw sequence (lg(max+k-1) bits).
* data: The data linked to the kmers. If the completely extended sequence (after application of the non terminal extentions) is of size s, this array must be of size (s-k+1)\*data_size bits.
