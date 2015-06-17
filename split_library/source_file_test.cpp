//
// Created by ac on 6/17/15.
//
#include <iostream>
#include "common_data_structure.h"
#include <string>
#include <vector>

using std::cout;
using std::endl;
using std::vector;
int main() {
    point p(1,2,3);
    point p2(2,3,4);
//    point p3 = p.crossProduct(p);
//    cout << &p3;

    vector<int> test = p.test();
    cout << &test <<endl;

}
