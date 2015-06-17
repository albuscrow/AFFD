//
// Created by ac on 6/17/15.
//

#ifndef PROJECT_COMMON_DATA_STRUCTURE_H
#define PROJECT_COMMON_DATA_STRUCTURE_H

#include <cmath>
#include <utility>
#include <iostream>
#include <vector>

using std::move;

class point {
public:
    point() { }

//    point(const point & p) {
//        std::cout << &p;
//    }

//    point(point && p) {
//        *this = std::move(p);
//    }

//    point &operator=(const point &p){
//        this->x = p.x;
//        this->y = p.y;
//        this->z = p.z;
//    }
//
//    point &operator=(point &&p){
//        *this = std::move(p);
//        return *this;
//    }

    point(double x, double y, double z) : x(x), y(y), z(z) { }

    point &&operator*(const point &p) {
        return move(point(x * p.x, y * p.y, z * p.z));
    }

    point &&crossProduct(const point &p) {
        point t(y * p.z - z * p.y,
                                  z * p.x - x * p.z,
                                  x * p.y - y * p.x);
        std::cout << &t;
        return move(t);
    }

    point &&normalize() {
        double length = this->length();
        return move(point(x / length, y / length, z / length));
    }

    double length() {
        return sqrt(x * x + y * y + z * z);
    }

    std::vector<int> &&test() {
        std::vector<int> v;
        std::cout << &v << std::endl;
        return std::move(v);
    }

private:
    double x = 0, y = 0, z = 0;


};

#endif //PROJECT_COMMON_DATA_STRUCTURE_H
