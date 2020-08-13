import unittest
import os

import cobs_index as cobs

mydir = os.path.dirname(os.path.realpath(__file__))
datadir = os.path.realpath(mydir + "/../../tests/data")

cobs.disable_cache()

class MainTest(unittest.TestCase):
    # read a directory containing FastA files
    def test_doc_list(self):
        l1 = cobs.DocumentList(datadir + "/fasta")
        self.assertEqual(l1.size(), 7)

        l2 = cobs.DocumentList()
        l2.add_recursive(datadir + "/fasta")
        self.assertEqual(l2.size(), 7)

    # construct classic index and run queries
    def test_classic_construct_query(self):
        index_file = datadir + "/python_test.cobs_classic"

        # construct classic index
        p = cobs.ClassicIndexParameters()
        p.clobber = True
        cobs.classic_construct(
            input=datadir + "/fasta",
            out_file=index_file,
            index_params=p)
        self.assertTrue(os.path.isfile(index_file))

        # run queries
        s = cobs.Search(index_file)
        r = s.search("AGTCAACGCTAAGGCATTTCCCCCCTGCCTCCTGCCTGCTGCCAAGCCCT")
        #print(r)
        self.assertEqual(len(r), 7)
        self.assertEqual(r[0].doc_name, "sample1")
        self.assertEqual(r[0].score, 20)

    # construct compact index and run queries
    def test_compact_construct_query(self):
        index_file = datadir + "/python_test.cobs_compact"

        # construct compact index
        p = cobs.CompactIndexParameters()
        p.clobber = True
        cobs.compact_construct(
            input=datadir + "/fasta",
            out_file=index_file,
            index_params=p)
        self.assertTrue(os.path.isfile(index_file))

        # run queries
        s = cobs.Search(index_file)
        r = s.search("AGTCAACGCTAAGGCATTTCCCCCCTGCCTCCTGCCTGCTGCCAAGCCCT")
        #print(r)
        self.assertEqual(len(r), 7)
        self.assertEqual(r[0].doc_name, "sample1")
        self.assertEqual(r[0].score, 20)

if __name__ == '__main__':
    unittest.main()
