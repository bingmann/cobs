import cobs_index as cobs

assert cobs.add(1, 2) == 3
assert cobs.subtract(1, 2) == -1

list = cobs.doc_list('/home/tb/2/cobs-experiments')
list.sort_by_path()
for f in list:
    print(f.size)

#index_params = cobs.ClassicIndexParameters()
#index_params.term_size = 20

#cobs.classic_construct("hello", "output.cobs_classic", index_params, clobber=True)
