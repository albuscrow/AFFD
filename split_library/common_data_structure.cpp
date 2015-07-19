//
// Created by ac on 6/17/15.
//

#include "common_data_structure.h"

std::vector<pointSharePtr> point::pointPool = std::vector<pointSharePtr>();

std::shared_ptr<point> point::findPoint(double x, double y, double z) {
    for (std::shared_ptr<point> ptr : pointPool) {
        if (ptr->equal(x, y, z)) {
            return ptr;
        }
    }
    return std::shared_ptr<point>();
}

std::shared_ptr<point> point::getPoint(double x, double y, double z) {
    std::shared_ptr<point> result = findPoint(x, y, z);
    if (!result) {
        result = std::shared_ptr<point>(new point(x, y, z));
        pointPool.push_back(result);
    }
    return result;
}

/**
 * 计算三角形外心
 */
point triangle::getExcentre() const {
    double xa = p1->getX();
    double xb = p2->getX();
    double xc = p3->getX();

    double ya = p1->getY();
    double yb = p2->getY();
    double yc = p3->getY();

    double aux1 = xa * xa + ya * ya
                  - xb * xb - yb * yb;

    double aux2 = xa * xa + ya * ya
                  - xc * xc - yc * yc;

    double x = (aux1 * (ya - yc) - aux2 * (ya - yb))
               / (2 * ((ya - yc) * (xa - xb)
                       - (ya - yb) * (xa - xc)));

    double y = (aux1 * (xa - xc) - aux2 * (xa - xb))
               / (2 * ((ya - yb) * (xa - xc)
                       - (ya - yc) * (xa - xb)));

    return point(x, y, 0);
}

void triangle::rotate() {
    vector3d normal = getNormal().normalize();
    if (normal.equal(0, 0, 1)) {
        return;
    }

    vector3d zAxis(0, 0, 1);

    vector3d v = normal.crossProduct(zAxis).normalize();
    std::cout << "rotate v" << v << std::endl;
    double cos = zAxis * normal;
    double sin = sqrt(1 - cos * cos);
    std::cout << acos(cos) / 3.1415 * 180 << std::endl;

    p1->rotate(v, sin, cos);
    p2->rotate(v, sin, cos);
    p3->rotate(v, sin, cos);
    e12 = edge(p1, p2);
    e23 = edge(p2, p3);
    e31 = edge(p3, p1);


    this->auxMatrixForContain = LaGenMatDouble(3, 3);
    LaVectorLongInt piv(3);

    auxMatrixForContain(0, 0) = p1->getX();
    auxMatrixForContain(1, 0) = p1->getY();
    auxMatrixForContain(2, 0) = 1;

    auxMatrixForContain(0, 1) = p2->getX();
    auxMatrixForContain(1, 1) = p2->getY();
    auxMatrixForContain(2, 1) = 1;

    auxMatrixForContain(0, 2) = p3->getX();
    auxMatrixForContain(1, 2) = p3->getY();
    auxMatrixForContain(2, 2) = 1;

    LUFactorizeIP(auxMatrixForContain, piv);
    LaLUInverseIP(auxMatrixForContain, piv);

    return;
}

vector3d triangle::getNormal() {
    point v1 = *p1 - *p2;
    point v2 = *p2 - *p3;

    return v1.crossProduct(v2);

}
