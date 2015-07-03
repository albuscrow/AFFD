//
// Created by ac on 6/17/15.
//

#ifndef PROJECT_COMMON_DATA_STRUCTURE_H
#define PROJECT_COMMON_DATA_STRUCTURE_H

#include <cmath>
#include <utility>
#include <iostream>
#include <vector>
#include <memory>
#include <assert.h>

#define ZERO 0.0000001

class point {
public:

    point() { }

    point(double x, double y, double z)
            : x(x), y(y), z(z), l(sqrt(x * x + y * y + z * z)) { }

    double getX() const {
        return x;
    }

    double getY() const {
        return y;
    }

    double getZ() const {
        return z;
    }


    //method for vector
    point operator*(const point &p) const {
        return point(x * p.x, y * p.y, z * p.z);
    }

    point operator*(const double &d) const {
        return point(x * d, y * d, z * d);
    }

    point operator/(const double &d) const {
        assert(d != 0.0);
        return point(x / d, y / d, z / d);
    }

    point operator-(const point &p) const {
        return point(x - p.x, y - p.y, z - p.z);
    }

    point operator+(const point &p) const {
        return point(x + p.x, y + p.y, z + p.z);
    }

    bool operator==(const point &p) const {
        return fabs(x - p.x) < ZERO
               && fabs(y - p.y) < ZERO
               && fabs(z - p.z) < ZERO;
    }

    point operator-() const {
        return point(-x, -y, -z);
    }

    friend std::ostream& operator<<(std::ostream &os, const point &p) {
        os << "point x:" << p.x << " y:" << p.y << " z:" << p.z << std::endl;
        return os;
    }

    point crossProduct(const point &p) {
        return point(y * p.z - z * p.y,
                     z * p.x - x * p.z,
                     x * p.y - y * p.x);
    }

    point normalize() {
        return point(x / l, y / l, z / l);
    }

    const double getLength() const {
        return l;
    }

private:
    double x = 0, y = 0, z = 0;
    double l = 0.0;
};

using vector3d = point;
using normal = point;

class edge {
public:
    edge() { }

    edge(const point &p1, const point &p2) : p1(p1), p2(p2),
                                             v12(p2 - p1), v21(p1 - p2),
                                             length(v12.getLength()) { }

    edge operator-() {
        return edge(p2, p1);
    }

    friend std::ostream& operator<<(std::ostream &os, const edge &e) {
        os << "edge\npoint1:" << e.p1 << "point2:" << e.p2;
        return os;
    }

    const point &getP1() const {
        return p1;
    }

    const point &getP2() const {
        return p2;
    }

    const vector3d &getV12() const {
        return v12;
    }

    const vector3d &getV21() const {
        return v21;
    }

    const double getLength() const {
        return length;
    }


private:
    point p1, p2;
    vector3d v12, v21;
    double length = 0.0;
};

using edge_ptr = std::unique_ptr<edge>;
using edge_share_ptr = std::shared_ptr<edge>;

class triangle {
public:
    triangle() { }

    triangle(const point &p1, const point &p2, const point &p3)
            : p1(p1), p2(p2), p3(p3),
              e12(p1, p2), e23(p2, p3), e31(p3, p1) {
    }

    const edge &getSmallestEdge() const {
        if (e12.getLength() < e23.getLength()
            && e12.getLength() < e31.getLength()) {
            return e12;
        } else if (e23.getLength() < e31.getLength()) {
            return e23;
        } else {
            return e31;
        }
    }

    const point &getP1() const {
        return p1;
    }

    const point &getP2() const {
        return p2;
    }

    const point &getP3() const {
        return p3;
    }

    const edge &getE12() const {
        return e12;
    }

    const edge &getE23() const {
        return e23;
    }

    const edge &getE31() const {
        return e31;
    }

private:
    point p1, p2, p3;
    edge e12, e23, e31;

};


#endif //PROJECT_COMMON_DATA_STRUCTURE_H
