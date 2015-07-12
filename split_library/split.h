//
// Created by ac on 6/17/15.
//

#ifndef PROJECT_SPLIT_H
#define PROJECT_SPLIT_H
#include <vector>
#include "common_data_structure.h"


void addTriangle(pointSharePtr p1, pointSharePtr p2, pointSharePtr p3,
                 std::vector<TriangleSharePtr> &result);

std::vector<TriangleSharePtr > split1(const triangle &t, const double &l);

std::vector<TriangleSharePtr > split1(const point &p1, const point &p2, const point &p3, const double &l);

void split1Iter(std::vector<pointSharePtr> p1, std::vector<pointSharePtr> p2, const double &l,
                std::vector<TriangleSharePtr > &result);

void splitOneLevel(edge edge1, edge edge2, double l, std::vector<TriangleSharePtr > &result);

std::vector<pointSharePtr> splitEdge(const edge &e, const double &l);

std::vector<triangle> split2(const triangle &t, const double &l);

std::vector<triangle> split2(pointSharePtr p1, pointSharePtr p2, pointSharePtr p3, const double &l);

#endif //PROJECT_SPLIT_H
