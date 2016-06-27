//
// Created by ac on 6/17/15.
// #include <iostream>
#include "common_data_structure.h"
#include "split.h"
#include <vector>
#include <fstream>
#include <cmath>
#include <stdlib.h>


using std::cout;
using std::endl;
using std::vector;
using std::min;
using std::max;
using std::fabs;

using param = struct parameter {
    double u, v, w;
};
#define ZERO 0.00001

void outputParam(param parameter);

void testCode(vector<size_t> triangleOffset, vector<size_t> indexes, vector<param> points, size_t table[20][20][20]);

void genSplitFile();

size_t getIndex(vector<param> &points, param point) {
    for (size_t i = 0; i < points.size(); ++i) {
        param p = points[i];
        if (fabs(p.u - point.u) < ZERO && fabs(p.v - point.v) < ZERO && fabs(p.w - point.w) < ZERO) {
            return i;
        }
    }
    points.push_back(point);
    return points.size() - 1;
}

param toParam(const triangle &t, point &p) {

    LaGenMatDouble result(3, 1);
    result(0, 0) = p.getX();
    result(1, 0) = p.getY();
    result(2, 0) = 1;


    LaGenMatDouble abc(3, 1);
    Blas_Mat_Mat_Mult(t.getAuxMatrixForContain(), result, abc, false, false);
    param r;
    r.u = abc(0, 0);
    r.v = abc(1, 0);
    r.w = abc(2, 0);
    return r;

//    SplitResultPoint splitResultPoint;
//    splitResultPoint.bary.x(abc(0, 0));
//    splitResultPoint.bary.y(abc(1, 0));
//    splitResultPoint.bary.z(abc(2, 0));

}

triangle genTriangle(float a, float b, float c) {
    if (a + b == c) {
        a += 0.05;
        b += 0.05;
    }

    pointSharePtr p1 = point::getPoint(0, 0, 0);
    pointSharePtr p2 = point::getPoint(a, 0, 0);
    double t = (a + b + c) / 2;
    double s = (float) sqrt(t * (t - a) * (t - b) * (t - c));
    double y = 2 * s / a;
    double x = (float) sqrt(c * c - y * y);
    double d = (x + a) * (x + a) + y * y;
    double d1 = fabs(d - b * b);
    if (d1 < ZERO) {
        x = -x;
    }
    pointSharePtr p3 = point::getPoint(x, y, 0);
    triangle tt(p1, p2, p3);
    tt.rotate();
    return tt;
}

void genSplitSchemeForOneTriangle() {
    int index = 0;
    std::__cxx11::string s1 = "./showme result";
    std::__cxx11::string s2 = ".poly";

    size_t factor = 30;
    vector<param> points;
    vector<size_t> indexes;
    vector<size_t> triangleOffset;
    size_t table[factor][factor][factor];
    int i = 5;
    int j = 6;
    int k = 7;

//    int i = 6;
//    int j = 7;
//    int k = 8;
    const triangle &t = genTriangle(i, j, k);
    auto triangles = split1(t, 1, true);
    auto currentPoints = point::getPointPool();
    table[i][j][k] = triangleOffset.size();
    triangleOffset.push_back(indexes.size() / 3);
    triangleOffset.push_back(triangles.size());
    for (auto smallTriangle : triangles) {
        indexes.push_back(getIndex(points, toParam(t, *smallTriangle->getP1())));
        indexes.push_back(getIndex(points, toParam(t, *smallTriangle->getP2())));
        indexes.push_back(getIndex(points, toParam(t, *smallTriangle->getP3())));
    }
    system((s1 + std::__cxx11::to_string(index ++) + s2).c_str());
}




int main() {
//    genSplitFile();
    cout << "begin split test " << endl;
    genSplitSchemeForOneTriangle();
}

void genSplitFile() {
    int index = 0;
    std::__cxx11::string s1 = "./showme result";
    std::__cxx11::string s2 = ".poly";

    size_t factor = 30;
    vector<param> points;
    vector<size_t> indexes;
    vector<size_t> triangleOffset;
    size_t table[factor][factor][factor];
    for (int i = 1; i <= factor; ++i) {
        cout << i << endl;
        for (int j = i; j <= factor; ++j) {
            for (int k = j; k <= factor; ++k) {
                if (i + j >= k) {
                    const triangle &t = genTriangle(i, j, k);
                    auto triangles = split1(t, 1, true);
//                    cout << "handle triangle #"<< index << endl;
                    auto currentPoints = point::getPointPool();
                    table[i][j][k] = triangleOffset.size();
                    triangleOffset.push_back(indexes.size() / 3);
                    triangleOffset.push_back(triangles.size());
                    for (auto smallTriangle : triangles) {
                        indexes.push_back(getIndex(points, toParam(t, *smallTriangle->getP1())));
                        indexes.push_back(getIndex(points, toParam(t, *smallTriangle->getP2())));
                        indexes.push_back(getIndex(points, toParam(t, *smallTriangle->getP3())));
                    }
//                    cout << s1 + std::to_string(index ++) + s2;
                    system((s1 + std::__cxx11::to_string(index ++) + s2).c_str());
                }
            }
        }
    }
    std::ofstream of(std::__cxx11::to_string(factor + 1) + ".txt");

    of << (factor + 1) << endl;
    for (int i = 0; i < triangleOffset.size(); ++i) {
        of << triangleOffset[i] << " ";
    }
    of << endl;
    int counter = 0;
    for (int i = 0; i < indexes.size(); ++i) {
        of << indexes[i] << " ";
        if (2 == counter) {
            of << "-1 ";
            counter = 0;
        } else {
            counter++;
        }
    }
    of << endl;
    for (int i = 0; i < points.size(); ++i) {
        of << points[i].u << " " << points[i].v << " " << points[i].w << " " << "0 ";
    }

    of.close();


//    testCode(triangleOffset, indexes, points, table);
    //    for (size_t i = 0; i < triangleOffset.size(); i += 2) {
//        size_t offset = triangleOffset[i * 2];
//        size_t number = triangleOffset[i * 2 + 1];
//        size_t end = offset + number;
//        std::ofstream of("debug.poly");
//        of << number * 3 << " 2 0 0" << endl;
//        for (size_t j = offset; j < end; ++j) {
//            auto p1 = points[indexes[offset * 3]];
//            auto p2 = points[indexes[offset * 3 + 1]];
//            auto p3 = points[indexes[offset * 3 + 2]];
//
//            for (int i = 0; i < number; ++i) {
//                of << i * 3 + 1 << " " << triangles[i]->getP1()->getX() << " " << triangles[i]->getP1()->getY() << endl;
//                of << i * 3 + 2 << " " << triangles[i]->getP2()->getX() << " " << triangles[i]->getP2()->getY() << endl;
//                of << i * 3 + 3 << " " << triangles[i]->getP3()->getX() << " " << triangles[i]->getP3()->getY() << endl;
//            }
//
//
//            of << number * 3 << " 0" << endl;
//            for (int i = 0; i < number; ++i) {
//                of << i * 3 + 1 << " " << i * 3 + 1 << " " << i * 3 + 2 << endl;
//                of << i * 3 + 2 << " " << i * 3 + 2 << " " << i * 3 + 3 << endl;
//                of << i * 3 + 3 << " " << i * 3 + 3 << " " << i * 3 + 1 << endl;
//            }
//
//            of << "0" << endl;
//            of.close();
//
//        }
//    }
}


//vector<param> points;
//vector<size_t> indexes;
//vector<size_t> triangleOffset;
//size_t table[20][20][20];
void testCode(vector<size_t> triangleOffset, vector<size_t> indexes, vector<param> points, size_t table[20][20][20]) {
//    cout << "poing size: " << points.size() << " index size: " << indexes.size() << " triangle size: " <<
//    triangleOffset.size() / 2;
    srand(10);
    for (size_t ii = 0; ii < 100; ++ii) {
//        cout << "test" << ii << endl;
        int i = rand() % 10;
        int j = rand() % 10;
        int k = rand() % 10;
//        int i = 3;
//        int j = 8;
//        int k = 6;

        if (i + j > k && abs(i - j) < k) {
            triangle t = genTriangle(i, j, k);
            double l12 = t.getE12().getLength();
            double l23 = t.getE23().getLength();
            double l31 = t.getE31().getLength();
            pointSharePtr p1, p2, p3;
            if (l12 < l23 && l12 < l31) {
                if (l23 < l31) {
                    p1 = t.getP1();
                    p2 = t.getP2();
                    p3 = t.getP3();
                } else {
                    p1 = t.getP2();
                    p2 = t.getP1();
                    p3 = t.getP3();
                }
            } else if (l23 < l31) {
                if (l12 < l31) {
                    p1 = t.getP3();
                    p2 = t.getP2();
                    p3 = t.getP1();
                } else {
                    p1 = t.getP2();
                    p2 = t.getP3();
                    p3 = t.getP1();
                }
            } else {
                if (l23 < l12) {
                    p1 = t.getP1();
                    p2 = t.getP3();
                    p3 = t.getP2();
                } else {
                    p1 = t.getP3();
                    p2 = t.getP1();
                    p3 = t.getP2();
                }
            }
            int t1 = min(min(i, j), k);
            int t3 = max(max(i, j), k);
            int t2 = i + j + k - t1 - t3;
            size_t offset = table[t1][t2][t3];
            size_t base = triangleOffset[offset];
            size_t number = triangleOffset[offset + 1];

            std::ofstream of("debug.poly");
//            number += 1;
            of << number * 3 << " 2 0 0" << endl;
//            of << 1 << " " << p1->getX() << " " << p1->getY() << endl;
//            of << 2 << " " << p2->getX() << " " << p2->getY() << endl;
//            of << 3 << " " << p3->getX() << " " << p3->getY() << endl;
            for (size_t iii = 0; iii < number; ++iii) {
                param pp0 = points[indexes[(base + iii) * 3]];
                param pp1 = points[indexes[(base + iii) * 3 + 1]];
                param pp2 = points[indexes[(base + iii) * 3 + 2]];
                outputParam(pp0);
                outputParam(pp1);
                outputParam(pp2);
                point sp0 = *p1 * pp0.u + *p2 * pp0.v + *p3 * pp0.w;
                point sp1 = *p1 * pp1.u + *p2 * pp1.v + *p3 * pp1.w;
                point sp2 = *p1 * pp2.u + *p2 * pp2.v + *p3 * pp2.w;
                of << iii * 3 + 1 << " " << sp0.getX() << " " << sp0.getY() << endl;
                of << iii * 3 + 2 << " " << sp1.getX() << " " << sp1.getY() << endl;
                of << iii * 3 + 3 << " " << sp2.getX() << " " << sp2.getY() << endl;
            }
            of << number * 3 << " 0" << endl;
            for (int ijk = 0; ijk < number; ++ijk) {
                of << ijk * 3 + 1 << " " << ijk * 3 + 1 << " " << ijk * 3 + 2 << endl;
                of << ijk * 3 + 2 << " " << ijk * 3 + 2 << " " << ijk * 3 + 3 << endl;
                of << ijk * 3 + 3 << " " << ijk * 3 + 3 << " " << ijk * 3 + 1 << endl;
            }

            of << "0" << endl;
            of.close();
            system("./showme debug.poly");

        }
    }
}

void outputParam(param parameter) {
    cout << parameter.u << " " << parameter.v << " " << parameter.w << endl;

}

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

//    /**/pointSharePtr p1 = point::getPoint(-0.40000000000000002, -1, 0.14999999999999999);
//    pointSharePtr p2 = point::getPoint(0.40000000000000002, -1, 0.14999999999999999);
//    pointSharePtr p3 = point::getPoint(-0.40000000000000002, 1, 0.14999999999999999);
//
//    triangle t(p1, p2, p3);
//    t.rotate();
//
////    cout << *t.getP1() << endl;
////    cout << *t.getP2() << endl;
////    cout << *t.getP3() << endl;
////
////    cout << t.getE12().getLength() << endl;
////    cout << t.getE23().getLength() << endl;
////    cout << t.getE31().getLength() << endl;
//
//
//    auto triangles = split1(t, 0.2);
//    point::printAllPoint();
//    int index = 0;
//
//    LaGenMatDouble result(3, 1);
//    for (auto p : point::getPointPool()) {
//        result(0, 0) = p->getX();
//        result(1, 0) = p->getY();
//        result(2, 0) = 1;
//
//
//        LaGenMatDouble abc(3, 1);
//        Blas_Mat_Mat_Mult(t.getAuxMatrixForContain(), result, abc, false, false);
//        cout << abc << endl;
//    SplitResultPoint splitResultPoint;
//    splitResultPoint.bary.x(abc(0, 0));
////    splitResultPoint.bary.y(abc(1, 0));
////    splitResultPoint.bary.z(abc(2, 0));
//
//    }
//
//    index = 0;
//    for (auto triangle : triangles) {
//        cout << index++ << " " << *triangle << endl;
//    }
