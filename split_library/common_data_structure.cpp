//
// Created by ac on 6/17/15.
//

#include "common_data_structure.h"

std::vector<pointSharePtr> point::pointPool = std::vector<pointSharePtr>();

std::shared_ptr<point> point::findPoint(double x, double y, double z) {
    for (std::shared_ptr<point> ptr : pointPool) {
        if (ptr->equal(x, y, z)){
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

