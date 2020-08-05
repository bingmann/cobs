# Compact Bit-Sliced Signature Index (COBS)

COBS (COmpact Bit-sliced Signature index) is a cross-over between an inverted index and Bloom filters. Our target application is to index k-mers of DNA samples or q-grams from text documents and process **approximate pattern matching** queries on the corpus with a user-chosen coverage threshold. Query results may contain a number of false positives which decreases exponentially with the query length and the false positive rate of the index determined at construction time.
COBS' compact but simple data structure outperforms other indexes in construction time and query performance with Mantis by Pandey et al. in second place.
However, unlike Mantis and other previous work, COBS does not need the complete index in RAM and is thus designed to scale to larger document sets.

![cobs-architecture](https://user-images.githubusercontent.com/2604907/58323540-91b52100-7e24-11e9-933d-98b9b24ae041.png)

COBS has two interfaces:

- a command line tool in C++ called `cobs` (see below)
- a Python interface to the C++ library [![PyPI version](https://badge.fury.io/py/cobs-index.svg)](https://badge.fury.io/py/cobs-index) (see https://bingmann.github.io/cobs-python-docs/)

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

## Python Interface

COBS also has a Python frontend interface which can be used to construct and query an index.
See https://bingmann.github.io/cobs-python-docs/ for a tutorial.

# Experimental Results

In our paper we compare COBS against seven other k-mer indexing software packages.
These are the main results, scaling by number of documents in the index, and in the second diagram shown per document.

![cobs-experiments-scaling](https://user-images.githubusercontent.com/2604907/58323544-94b01180-7e24-11e9-8c3a-be998eb790a4.png)
![cobs-experiments-scaling-per-documents](https://user-images.githubusercontent.com/2604907/58323546-9679d500-7e24-11e9-9fed-636889628050.png)
