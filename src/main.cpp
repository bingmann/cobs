#include <boost/filesystem/path.hpp>
#include <bloom_filter.hpp>

int main(int argc, char** argv) {
    boost::filesystem::path in_dir = "/users/flo/projects/thesis/data/bin";
    boost::filesystem::path out_dir = "/users/flo/projects/thesis/data/bloom";
    genome::bloom_filter::process_all_in_directory(in_dir, out_dir);


    int a = 0;
}
