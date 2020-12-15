#include "utility.h"
#include <string>
#include <iostream>

int fake_error(const std::string & err) {
    std::cout << err << std::endl;
    return 0;
}
int print_error(const std::string &err)
{
    std::cout << err << std::endl;
    return 1;
}