//
// Created by ac on 6/17/15.
//

#include "split.h"
#include <algorithm>

using std::vector;
using std::cout;
using std::cin;
using std::endl;

std::vector<triangle> split1(const triangle &t, const double &l) {

    //取得三角形中两个较长的边
//    vector<edge> edgeVector{t.getE12(), t.getE23(), t.getE31()};
//
//    std::sort(edgeVector.begin(), edgeVector.end(), [](const edge edge1, const edge edge2) {
//        return edge1.getLength() > edge2.getLength();
//    });
//
//    edge &edgeLonger = edgeVector[0];
//    edge &edgeShorter = edgeVector[1];
//
//    //调整三角形一条边的指向，使得两条边都从公共点出发
//    if (edgeLonger.getP1() == edgeShorter.getP2()) {
//        edgeShorter = -edgeShorter;
//    } else {
//        edgeLonger = -edgeLonger;
//    }
//
//#ifdef TEST
//    if (edgeLonger.getP1() == edgeShorter.getP1()) {
//        cout << "test adjust direct of edge ok" << endl;
//    }
//#endif

    edge_ptr firstEdge, secondEdge;


    vector<point> pointVector1 = splitEdge(edgeLonger, l);
    vector<point> pointVector2 = splitEdge(edgeShorter, l);

    vector<triangle> triangleVector;

    for (int i = 0; i < pointVector1.size() - 1 && i < pointVector2.size() - 1; ++i) {

    }

    cout <<  "test" << ceil((pointVector1[0] - pointVector2[0]).getLength() / l) << endl;

    return std::vector<triangle>();
}

std::vector<point> splitEdge(const edge &e, const double &l) {
    int n = (int) ceil(e.getLength() / l);
    vector3d delta = e.getV12() / n;
    vector<point> pointVector;
    pointVector.reserve((unsigned long) (n + 1));
    for (int i = 0; i < n + 1; ++i) {
        const point &point = e.getP1();
        pointVector.push_back(point + delta * i);
    }
#ifdef TEST
    cout << "check split edge" << endl;
    cout << e << endl;

    for (point p : pointVector) {
        cout << p;
    }
#endif
    return pointVector;
}

std::vector<triangle> split2(const triangle &t, const double &l) {
}

std::vector<triangle> split1(const point &p1, const point &p2, const point &p3, const double &l) {
    split1(triangle(p1, p2, p3), l);
}

std::vector<triangle> split2(const point &p1, const point &p2, const point &p3, const double &l) {
    split2(triangle(p1, p2, p3), l);
}