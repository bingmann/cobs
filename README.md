# Compact Bit-Sliced Signature Index (COBS)

COBS (COmpact Bit-sliced Signature index) is a cross-over between an inverted index and Bloom filters. Our target application is to index k-mers of DNA samples or q-grams from text documents and process **approximate pattern matching** queries on the corpus with a user-chosen coverage threshold. Query results may contain a number of false positives which decreases exponentially with the query length and the false positive rate of the index determined at construction time.
COBS' compact but simple data structure outperforms other indexes in construction time and query performance with Mantis by Pandey et al. in second place.
However, unlike Mantis and other previous work, COBS does not need the complete index in RAM and is thus designed to scale to larger document sets.

![cobs-architecture](https://user-images.githubusercontent.com/2604907/58323540-91b52100-7e24-11e9-933d-98b9b24ae041.png)

COBS has two interfaces: (
[![Build Status](https://travis-ci.org/bingmann/cobs.svg?branch=master)](https://travis-ci.org/bingmann/cobs)
[![Coverage Status](https://coveralls.io/repos/github/bingmann/cobs/badge.svg?branch=master)](https://coveralls.io/github/bingmann/cobs?branch=master)
)

- a command line tool in C++ called `cobs` (see below)
- a Python interface to the C++ library [![PyPI version](https://badge.fury.io/py/cobs-index.svg)](https://badge.fury.io/py/cobs-index) (see [bingmann.github.io/cobs-python-docs/](https://bingmann.github.io/cobs-python-docs/))


More information about COBS is presented in [our current research paper](https://arxiv.org/abs/1905.09624):
Timo Bingmann, Phelim Bradley, Florian Gauger, and Zamin Iqbal.
"COBS: a Compact Bit-Sliced Signature Index".
In: *26th International Symposium on String Processing and Information Retrieval (SPIRE)*. pages 285-303. Spinger. October 2019.
preprint arXiv:1905.09624.

If you use COBS in an academic context or publication, please cite our paper
```
@InProceedings{bingmann2019cobs,
  author =       {Timo Bingmann and Phelim Bradley and Florian Gauger and Zamin Iqbal},
  title =        {{COBS}: a Compact Bit-Sliced Signature Index},
  booktitle =    {26th International Conference on String Processing and Information Retrieval (SPIRE)},
  year =         2019,
  series =       {LNCS},
  pages =        {285--303},
  month =        oct,
  organization = {Springer},
  note =         {preprint arXiv:1905.09624},
}
```

# Installation and First Steps

## Installation

COBS requires CMake, a C++17 compiler or the Boost.Filesystem library.

To download and install COBS run:
```
git clone --recursive https://github.com/bingmann/cobs.git
mkdir cobs/build
cd cobs/build
cmake ..
make -j4
```
and optionally run `make test` to check the build.

## Building an Index

COBS can read FASTA files (`*.fa`, `*.fasta`, `*.fna`, `*.ffn`, `*.faa`, `*.frn`, `*.fa.gz`, `*.fasta.gz`, `*.fna.gz`, `*.ffn.gz`, `*.faa.gz`, `*.frn.gz`), FASTQ files (`*.fq`, `*.fastq`, `*.fq.gz.`, `*.fastq.gz`), "Multi-FASTA" and "Multi-FASTQ" files (`*.mfasta`, `*.mfastq`), McCortex files (`*.ctx`), or text files (`*.txt`). 
See below on [details how they are parsed](#file-types-and-how-they-are-parsed).

You can either recursively scan a directory for all files matching any of these files, or pass a `*.list` file which lists all paths COBS should index.

To check the document list to be indexed, run for example
```
src/cobs doc-list tests/data/fasta/
```

To construct a compact COBS index from these seven example documents run
```
src/cobs compact-construct tests/data/fasta/ example.cobs_compact
```

Or construct a compact COBS index from a list of documents by running
```
src/cobs compact-construct tests/data/fasta_files.list example.cobs_compact
```
The paths in the file list can be absolute or relative to the file list's path.
Note that `*.txt` files are read as verbatim text files.
You can force COBS to read a `.txt` file as a file list using `--file-type list`.

Check `--help` for many options.

## Query an Index

COBS has a simple command line query tool:
```
src/cobs query -i example.cobs_compact AGTCAACGCTAAGGCATTTCCCCCCTGCCTCCTGCCTGCTGCCAAGCCCT
```
or a fasta file of queries with
```
src/cobs query -i example.cobs_compact -f query.fa
```
Multiple indices can be queried at once by adding more `-i` parameters.

## Python Interface

COBS also has a Python frontend interface which can be used to construct and query an index.
See https://bingmann.github.io/cobs-python-docs/ for a tutorial.

# Experimental Results

In our paper we compare COBS against seven other k-mer indexing software packages.
These are the main results, scaling by number of documents in the index, and in the second diagram shown per document.

![cobs-experiments-scaling](https://user-images.githubusercontent.com/2604907/58323544-94b01180-7e24-11e9-8c3a-be998eb790a4.png)
![cobs-experiments-scaling-per-documents](https://user-images.githubusercontent.com/2604907/58323546-9679d500-7e24-11e9-9fed-636889628050.png)

# More Details

## File Types and How They Are Parsed

COBS can read FASTA files (`*.fa`, `*.fasta`, `*.fa.gz`, `*.fasta.gz`), FASTQ files (`*.fq`, `*.fastq`, `*.fq.gz.`, `*.fastq.gz`), "Multi-FASTA" and "Multi-FASTQ" files (`*.mfasta`, `*.mfastq`), McCortex files (`*.ctx`), or text files (`*.txt`). 
Each file type is parsed slightly differently into q-grams or k-mers.

FASTA files are parsed as one document each.
If a FASTA file contains multiple sequences or reads then they are combined into one document.
Multiple sequences (separated by comments) are NOT concatenated trivially, instead the k-mers are extracted separately from each sequence.
This means there are no erroneous k-mers from the beginning or end of crossing sequences.
All newlines within a sequence are removed.

The k-mers from DNA sequences are automatically canonicalized (the lexicographically smaller is indexed).
By adding the flag `--no-canonicalize` this process can be skipped.
With canonicalization only ACGT letters are indexed, every other letter is mapped to binary zeros and index with the other data.
A warning per FASTA/FASTQ file containing a non-ACGT letter is printed, but processing continues.
With the flag `--no-canonicalize` any letters or text can be indexed.

FASTQ files are also parsed as one document each.
The quality information is dropped and effectively everything is parsed identical to FASTA files.

Multi-FASTA or Multi-FASTQ files are parsed as many documents.
Each sequence in the FASTA or FASTQ file is considered a separate document in the COBS index.
Their names are append with `_###` where ### is the index of the subdocument.

McCortex files (`*.ctx`) contain a list of k-mers and these k-mers are indexes individually.
The graph information is ignored.
Only k=31 is currently supported.

Text files (`*.txt`) are parsed as verbatim binary documents.
All q-grams are extracted, including newlines and other whitespace.


