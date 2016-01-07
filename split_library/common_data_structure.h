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
#include <lapackpp/gmd.h>
#include <lapackpp/lavli.h>
#include <lapackpp/laslv.h>
#include <lapackpp/blas3pp.h>

#ifndef ZERO
#define ZERO 0.00001
#endif

class point {
public:

    static std::shared_ptr<point> findPoint(double x, double y, double z);


#ifdef TEST
    static void printAllPoint();
#endif

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
    double operator*(const point &p) const {
        return x * p.x + y * p.y + z * p.z;
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

//    point operator+(const point &p) const {
//        return point(x + p.x, y + p.y, z + p.z);
//    }


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

    friend std::ostream &operator<<(std::ostream &os, const point &p) {
        os << "point id:"<< p.id << " x:" << p.x << " y:" << p.y << " z:" << p.z << std::endl;
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

    bool equal(double x, double y, double z) {
        return fabs(x - this->x) < ZERO
               && fabs(y - this->y) < ZERO
               && fabs(z - this->z) < ZERO;
    }

    static const std::vector<std::shared_ptr<point>> &getPointPool() {
        return pointPool;
    }

    static void clearPointPool() {
        pointPool.clear();
        max_id = 0;
        return;
    }

    point() { }

    point(double x, double y, double z)
            : x(x), y(y), z(z), l(sqrt(x * x + y * y + z * z)){ }

    void setX(double x) {
        point::x = x;
    }

    void setY(double y) {
        point::y = y;
    }

    void setZ(double z) {
        point::z = z;
    }

    /**
     * get rotate triangle use following formula:
     * m*r = r*cos(theta) + v x r*sin(theme) + (v * r) * v(1-cos(theme));*/

    point rotate(point v, double sin, double cos) {
//        point p = *this * cos + v.crossProduct(*this) * sin
//                + v * (v * (*this)) * (1 - cos);
//        x = p.x;
//        y = p.y;
//        z = p.z;
        double oneminscos = 1 - cos;
        double xx = v.x * v.x;
        double yy = v.y * v.y;
        double zz = v.z * v.z;
        double xy = v.x * v.y;
        double yz = v.y * v.z;
        double zx = v.z * v.x;
        double newX = (cos + oneminscos * xx) * x +
                      (oneminscos * xy - sin * v.z) * y +
                      (oneminscos * zx + sin * v.y) * z;

        double newY = (oneminscos * xy + sin * v.z) * x +
                      (cos + oneminscos * yy) * y +
                      (oneminscos * yz - sin * v.x) * z;

        double newZ = (oneminscos * zx - sin * v.y) * x +
                      (oneminscos * yz + sin * v.x) * y +
                      (cos + oneminscos * zz) * z;

        x = newX;
        y = newY;
        z = newZ;

        return *this;
    }

    unsigned long getId() const {
        return id;
    }

    void setId() {
        point::id = max_id++;
    }

private:
    unsigned long id;
    double x = 0, y = 0, z = 0;
    double l = 0.0;
    static std::vector<std::shared_ptr<point>> pointPool;
    static unsigned long max_id;
};

using vector3d = point;
using vector3dSharePtr = std::shared_ptr<vector3d>;
using normal = point;
using pointSharePtr = std::shared_ptr<point>;

class edge {
public:

    edge() { };

    edge(pointSharePtr p1, pointSharePtr p2) : p1(p1), p2(p2),
                                               v12(*p2 - *p1), v21(*p1 - *p2),
                                               length(v12.getLength()) { }

    edge operator-() {
        return edge(p2, p1);
    }

    friend std::ostream &operator<<(std::ostream &os, const edge &e) {
        os << "edge\npoint1:" << *e.p1 << "point2:" << *e.p2;
        return os;
    }

    pointSharePtr getP1() const {
        return p1;
    }

    pointSharePtr getP2() const {
        return p2;
    }

    vector3d getV12() const {
        return v12;
    }

    vector3d getV21() const {
        return v21;
    }

    const double getLength() const {
        return length;
    }


private:
    pointSharePtr p1, p2;
    vector3d v12, v21;
    double length = 0.0;
};

using edge_ptr = std::unique_ptr<edge>;
using edge_share_ptr = std::shared_ptr<edge>;

class triangle {
public:
    triangle(pointSharePtr p1, pointSharePtr p2, pointSharePtr p3)
            : p1(p1), p2(p2), p3(p3),
              e12(p1, p2), e23(p2, p3), e31(p3, p1) {

        //构造矩阵用于求逆
//        this->auxMatrixForContain = LaGenMatDouble(3, 3);
//        LaVectorLongInt piv(3);
//
//        auxMatrixForContain(0, 0) = p1->getX();
//        auxMatrixForContain(1, 0) = p1->getY();
//        auxMatrixForContain(2, 0) = 1;
//
//        auxMatrixForContain(0, 1) = p2->getX();
//        auxMatrixForContain(1, 1) = p2->getY();
//        auxMatrixForContain(2, 1) = 1;
//
//        auxMatrixForContain(0, 2) = p3->getX();
//        auxMatrixForContain(1, 2) = p3->getY();
//        auxMatrixForContain(2, 2) = 1;
//
//        LUFactorizeIP(auxMatrixForContain, piv);
//        LaLUInverseIP(auxMatrixForContain, piv);

    }

    bool containPoint(point *p) const {
        LaGenMatDouble result(3, 1);
        result(0, 0) = p->getX();
        result(1, 0) = p->getY();
        result(2, 0) = 1;

//        std::cout << "containPoint:\n" << result << std::endl;
//        std::cout << auxMatrixForContain << std::endl;

        LaGenMatDouble abc(3, 1);
        Blas_Mat_Mat_Mult(auxMatrixForContain, result, abc, false, false);
//        std::cout << auxMatrixForContain << std::endl;
//        std::cout << abc << std::endl;
        return abc(0, 0) - 1 < ZERO && abc(0, 0) - 0 > ZERO
               && abc(1, 0) - 1 < ZERO && abc(1, 0) - 0 > ZERO
               && abc(2, 0) - 1 < ZERO && abc(2, 0) - 0 > ZERO;
    }

    void rotate();

    vector3d getNormal();

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

    point getExcentre() const;

    const LaGenMatDouble &getAuxMatrixForContain() const {
        return auxMatrixForContain;
    }

    friend std::ostream &operator<<(std::ostream &os, const triangle &e) {
        os << "triangle\n" << e.p1->getId() << " " << e.p2->getId()<< " " << e.p3->getId();
        return os;
    }

private:
    pointSharePtr p1, p2, p3;
    edge e12, e23, e31;
    LaGenMatDouble auxMatrixForContain;

};

using TriangleSharePtr = std::shared_ptr<triangle>;


#endif //PROJECT_COMMON_DATA_STRUCTURE_H
