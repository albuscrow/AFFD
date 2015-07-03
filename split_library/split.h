//
// Created by ac on 6/17/15.
//

#ifndef PROJECT_SPLIT_H
#define PROJECT_SPLIT_H
#include <vector>
#include "common_data_structure.h"

std::vector<triangle> split1(const triangle &t, const double &l);

std::vector<triangle> split1(const point &p1, const point &p2, const point &p3, const double &l);

void split1Iter(std::vector<point> p1, std::vector<point> p2, const double &l, bool isOriginal, std::vector<triangle> &result);

void splitOneLevel(edge edge1, edge edge2, double l, std::vector<triangle> &result);

std::vector<point> splitEdge(const edge &e, const double &l);

std::vector<triangle> split2(const triangle &t, const double &l);

std::vector<triangle> split2(const point &p1, const point &p2, const point &p3, const double &l);

#endif //PROJECT_SPLIT_H
