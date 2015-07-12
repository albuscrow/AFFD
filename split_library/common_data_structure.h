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

    static std::shared_ptr<point> findPoint(double x, double y, double z);

    static std::shared_ptr<point> getPoint(double x, double y, double z);

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

    std::shared_ptr<point> operator-(const point &p) const {
        return std::shared_ptr<point>(new point(x - p.x, y - p.y, z - p.z));
    }

//    point operator+(const point &p) const {
//        return point(x + p.x, y + p.y, z + p.z);
//    }


    std::shared_ptr<point> operator+(const point &p) const {
        return std::shared_ptr<point>(new point(x + p.x, y + p.y, z + p.z));
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

    bool equal (double x, double y, double z) {
        return fabs(x - this->x) < ZERO
                && fabs(y - this->y) < ZERO
                && fabs(z - this->z) < ZERO;
    }

    static const std::vector<std::shared_ptr<point>> &getPointPool() {
        return pointPool;
    }

private:
    double x = 0, y = 0, z = 0;
    double l = 0.0;
    static std::vector<std::shared_ptr<point>> pointPool;
    point() { }
    point(double x, double y, double z)
            : x(x), y(y), z(z), l(sqrt(x * x + y * y + z * z)) { }
};

using vector3d = point;
using vector3dSharePtr = std::shared_ptr<vector3d>;
using normal = point;
using pointSharePtr = std::shared_ptr<point>;

class edge {
public:

    edge(){};

    edge(pointSharePtr p1, pointSharePtr p2) : p1(p1), p2(p2),
                                             v12(*p2 - *p1), v21(*p1 - *p2),
                                             length(v12->getLength()) { }

    edge operator-() {
        return edge(p2, p1);
    }

    friend std::ostream& operator<<(std::ostream &os, const edge &e) {
        os << "edge\npoint1:" << e.p1 << "point2:" << e.p2;
        return os;
    }

    pointSharePtr getP1() const {
        return p1;
    }

    pointSharePtr getP2() const {
        return p2;
    }

    vector3dSharePtr getV12() const {
        return v12;
    }

    vector3dSharePtr getV21() const {
        return v21;
    }

    const double getLength() const {
        return length;
    }


private:
    pointSharePtr p1, p2;
    vector3dSharePtr v12, v21;
    double length = 0.0;
};

using edge_ptr = std::unique_ptr<edge>;
using edge_share_ptr = std::shared_ptr<edge>;

class triangle {
public:
    triangle(pointSharePtr p1, pointSharePtr p2, pointSharePtr p3)
            : p1(p1), p2(p2), p3(p3),
              e12(p1, p2), e23(p2, p3), e31(p3, p1) {
    }

    bool containPoint(point *) {

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

    pointSharePtr getP1() const {
        return p1;
    }

    pointSharePtr getP2() const {
        return p2;
    }

    pointSharePtr getP3() const {
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
    pointSharePtr p1, p2, p3;
    edge e12, e23, e31;

};

using TriangleSharePtr = std::shared_ptr<triangle>;


#endif //PROJECT_COMMON_DATA_STRUCTURE_H
