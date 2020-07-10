# KFF 1 - Kmer File Format v1

This readme define a universal file format to store DNA kmers and their associate data.
The associate data for a kmer can be everything if its size is constant from a kmer to another.
For example, a count with a maximum of 255 can be store on a byte associated to each kmer.


# Overwiew

The file is composed of two sections:
* **An header section**: It describes the different constant values.
For example, it contains the k value and the size of kmer associated data.
* **A content section**: The content section is a list of blocks of different sizes that contained a DNA sequence and the kmer data associated.
Each block can contains more than one kmer in a compacted way.
So for example with k=3, a block with ACTTG will represent the set of kmer {ACT, CTT, TTG}.

Need of a schema here

## Byte multiple

To be fast to read and write, all the values are stored on multiple of 8 bits to match a Byte.
So, if a DNA sequence needs 12 bits to be represented, 16 will be used and the 4 most significant bits will be set randomly.


# File description

## Header

Ordered values in the header:
* k: The kmer size (1 Byte)
* encoding: ACGT encoding (2 bits/nucl - 1 Byte).
For example the Byte 00101101 means that in the 2 bits encoding A=0, T=1, C=2, G=3.
The 4 values need to be different.
* max: The maximum number of kmer allowed in one block (8 Bytes).
Max have to be non zero.
* data_size: The size of the data associated to each kmer (8 Bytes)
This value can be 0 if you don't want to associate any piece of data.

## Block

Each block is composed of 3 values:
* n: The number of kmers stored in the block <= max (lg(max) bits (ceiled on a 8 multiple)).
If max have been set to 1 in the header, this value is absent (save 1 Byte per block).
* seq: The DNA sequence using 2 bits / nucleotide with respect to the encoding set in the header (with a padding to fill a Byte multiple)
* data: An array of n\*data_size bits containing the data associated with each kmer (empty if data_size=0).
