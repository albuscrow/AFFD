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
    point p1(3,0,0);
    point p2(0,1,0);
    point p3(0,0,0);
    triangle t(p1, p2, p3);

    split1(t, 0.2);
    system("./showme result.poly");
}
