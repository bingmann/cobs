import cobs

assert cobs.add(1, 2) == 3
assert cobs.subtract(1, 2) == -1

index_params = cobs.ClassicIndexParameters()
index_params.term_size = 20

cobs.construct_classic("hello", "output", index_params)
