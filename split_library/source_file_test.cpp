//
// Created by ac on 6/17/15.
//
#include <iostream>
#include "common_data_structure.h"
#include "split.h"

using std::cout;
using std::endl;
using std::vector;
int main() {
    pointSharePtr p1 = point::getPoint(3, 0, 0);
    pointSharePtr p2 = point::getPoint(0, 1, 0);
    pointSharePtr p3 = point::getPoint(0, 0, 0);
    triangle t(p1, p2, p3);

    split1(t, 0.8);
    system("./showme result.poly");
}
