//
// Created by ac on 6/17/15.
// #include <iostream>
#include "common_data_structure.h"
#include "split.h"

using std::cout;
using std::endl;
using std::vector;

int main() {
//    pointSharePtr p1 = point::getPoint(-0.69761817628181499, -0.99835324650227397, 0.072072300249879262);
//    pointSharePtr p2 = point::getPoint(-0.47942909984517917, -0.77803370304274133, 0.13533818800086106);
//    pointSharePtr p3 = point::getPoint(-0.32247721013422681, -0.23306591490631048, 0.072072300249879262);

//    pointSharePtr p1 = point::getPoint(0, 0.111111, 0.22222165000000002);
//    pointSharePtr p2 = point::getPoint(0.019272000000000001, 0.109427, 0.22222165000000002);
//    pointSharePtr p3 = point::getPoint(0.038544000000000002, 0.21885399999999999, 0.22222165000000002);

//    pointSharePtr p1 = point::getPoint(  0.14067897347908023, -0.14840247047809549, 0.10515355580574483);
//    pointSharePtr p2 = point::getPoint(  0.19492616904977458, -0.14886549214936226, 0.22095804355425094);
//    pointSharePtr p3 = point::getPoint(0.25261311170632139, -0.14056308022437641, 0.20683362205566624);

//    pointSharePtr p1 = point::getPoint(0.18831400000000001, 0.48289599999999999, -0.058966600000000001);
//    pointSharePtr p2 = point::getPoint(0.151925, 0.38725500000000002, -0.059879500000000002);
//    pointSharePtr p3 = point::getPoint(0.18415200000000001, 0.43068099999999998, -0.061342000000000001)

    pointSharePtr p1 = point::getPoint(-0.40000000000000002, -1, 0.14999999999999999);
    pointSharePtr p2 = point::getPoint(0.40000000000000002, -1, 0.14999999999999999);
    pointSharePtr p3 = point::getPoint(-0.40000000000000002, 1, 0.14999999999999999);

    triangle t(p1, p2, p3);
    t.rotate();

    cout << *t.getP1() << endl;
    cout << *t.getP2() << endl;
    cout << *t.getP3() << endl;

    cout << t.getE12().getLength() << endl;
    cout << t.getE23().getLength() << endl;
    cout << t.getE31().getLength() << endl;


    split1(t, 0.04);
    system("./showme result0.poly");
}
