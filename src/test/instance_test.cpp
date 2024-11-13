#include "instance/instance.hpp"
#include <cassert>
#include <fstream>
#include <iostream>

int main() {
    std::ifstream ifs;
    mopop::Instance instance;

    const std::string filename = ""
    std::cout << filename << std::endl;

    ifs.open(filename);

    assert(ifs.is_open());

    ifs >> instance;

    ifs.close();

    assert(instance.num_dimensions >= 2);
    assert(instance.num_dimensions <= 4);
    assert(instance.num_items >= 100);
    assert(instance.num_items <= 1000);
    assert(instance.is_valid());

    std::cout << std::endl << "Instance Test PASSED" << std::endl;

    return 0;
}
