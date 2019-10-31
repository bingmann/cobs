# Compact Bit-Sliced Signature Index (COBS)

COBS (COmpact Bit-sliced Signature index) is a cross-over between an inverted index and Bloom filters. Our target application is to index k-mers of DNA samples or q-grams from text documents and process **approximate pattern matching** queries on the corpus with a user-chosen coverage threshold. Query results may contain a number of false positives which decreases exponentially with the query length and the false positive rate of the index determined at construction time.
COBS' compact but simple data structure outperforms other indexes in construction time and query performance with Mantis by Pandey et al. in second place.
However, unlike Mantis and other previous work, COBS does not need the complete index in RAM and is thus designed to scale to larger document sets.

![cobs-architecture](https://user-images.githubusercontent.com/2604907/58323540-91b52100-7e24-11e9-933d-98b9b24ae041.png)

More information about COBS is presented in [our current research paper](https://arxiv.org/abs/1905.09624):
Timo Bingmann, Phelim Bradley, Florian Gauger, and Zamin Iqbal.
"COBS: a Compact Bit-Sliced Signature Index".
In: *26th International Symposium on String Processing and Information Retrieval (SPIRE)*. pages 285-303. Spinger. October 2019.
preprint arXiv:1905.09624.

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

COBS can read FASTA files (`*.fa`, `*.fasta`, `*.fa.gz`, `*.fasta.gz`), FASTQ files (`*.fq`, `*.fastq`, `*.fq.gz.`, `*.fastq.gz`), McCortex files (`*.ctx`), or text files (`*.txt`).

You can either recursively scan a directory for all files matching any of these files, or pass a `*.list` file which lists all paths COBS should index.

To check the document list to be indexed, run for example
```
src/cobs doc-list tests/data/fasta/
```

To construct a compact COBS index from these seven example documents run
```
src/cobs compact-construct tests/data/fasta/ example.cobs_compact
```
Check `--help` for many options.
Maybe the most important is `--canonicalize` to enable k-mer DNA canonicalization.

## Query an Index

COBS has a simple command line query tool:
```
src/cobs query -i example.cobs_compact AGTCAACGCTAAGGCATTTCCCCCCTGCCTCCTGCCTGCTGCCAAGCCCT
```
or a fasta file of queries with
```
src/cobs query -i example.cobs_compact -f query.fa
```

# Experimental Results

In our paper we compare COBS against seven other k-mer indexing software packages.
These are the main results, scaling by number of documents in the index, and in the second diagram shown per document.

![cobs-experiments-scaling](https://user-images.githubusercontent.com/2604907/58323544-94b01180-7e24-11e9-8c3a-be998eb790a4.png)
![cobs-experiments-scaling-per-documents](https://user-images.githubusercontent.com/2604907/58323546-9679d500-7e24-11e9-9fed-636889628050.png)
