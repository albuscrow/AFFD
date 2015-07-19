//
// Created by ac on 6/17/15.
// #include <iostream>
#include "common_data_structure.h"
#include "split.h"

using std::cout;
using std::endl;
using std::vector;

int main() {
    pointSharePtr p1 = point::getPoint(2, 4, 4);
    pointSharePtr p2 = point::getPoint(0, 3, -10);
    pointSharePtr p3 = point::getPoint(5, -6, 8);
    triangle t(p1, p2, p3);
    t.rotate();

    cout << *t.getP1() << endl;
    cout << *t.getP2() << endl;
    cout << *t.getP3() << endl;

    cout << t.getE12().getLength() << endl;
    cout << t.getE23().getLength() << endl;
    cout << t.getE31().getLength() << endl;


    split1(t, 0.9);
    system("./showme result.poly");
}
