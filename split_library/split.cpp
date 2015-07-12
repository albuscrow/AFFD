//
// Created by ac on 6/17/15.
//

#include "split.h"
#include <algorithm>
#include <fstream>
#include <map>


void outputAsFile(std::vector<TriangleSharePtr > &triangles);

void cvt(std::map<point *, std::vector<TriangleSharePtr>> map, const triangle &t);

using std::vector;
using std::cout;
using std::cin;
using std::endl;

std::map<point *, std::vector<TriangleSharePtr>> pointTriangleMap =
        std::map<point *, std::vector<TriangleSharePtr>>();

std::vector<TriangleSharePtr > split1(const triangle &t, const double &l) {

    //取得三角形中两个较长的边
    vector<edge> edgeVector{t.getE12(), t.getE23(), t.getE31()};

    std::sort(edgeVector.begin(), edgeVector.end(), [](const edge edge1, const edge edge2) {
        return edge1.getLength() > edge2.getLength();
    });

    edge &e1 = edgeVector[0];
    edge &e2 = edgeVector[1];


    edge firstEdge, secondEdge;
    //按照三角形的走向，区分前后边
    if (e1.getP1() == e2.getP2()) {
        firstEdge = e2;
        secondEdge = e1;
    } else {
        firstEdge = e1;
        secondEdge = e2;
    }

    //调整前一条边的走向，另两条边都是从公共顶点出发
    firstEdge = -firstEdge;
#ifdef TEST
    if (firstEdge.getP1() == secondEdge.getP1()) {
        cout << "test adjust direct of edge ok" << endl;
    }
#endif

    vector<pointSharePtr> firstEdgePoints = splitEdge(firstEdge, l);
    vector<pointSharePtr> secondEdgePoints = splitEdge(secondEdge, l);
    vector<TriangleSharePtr> result;

    //先把最顶上的三角形分割出来
    addTriangle(firstEdgePoints[1], firstEdgePoints[0], secondEdgePoints[1], result);
    firstEdgePoints.erase(firstEdgePoints.begin());
    secondEdgePoints.erase(secondEdgePoints.begin());

    split1Iter(firstEdgePoints, secondEdgePoints, l, result);

//    result.push_back(t);
    outputAsFile(result);


#ifdef TEST
//    cout << point::getPointPool().size() << endl;
//    cout << pointTriangleMap.size() << endl;
//    for (auto iter = pointTriangleMap.begin(); iter != pointTriangleMap.end(); ++iter) {
//        cout << iter->second.size() << endl;
//    }
#endif
    cvt(pointTriangleMap);

    return result;
}

void cvt(std::map<point *, vector<TriangleSharePtr>> map, const triangle &t) {
    for (auto iter = pointTriangleMap.begin(); iter != pointTriangleMap.end(); ++iter) {
        if (t.containPoint(iter->first)) {

        }
        cout << iter->second.size() << endl;
    }

}


void split1Iter(vector<pointSharePtr> firstEdgePoints, vector<pointSharePtr> secondEdgePoints, const double &l,
                vector<TriangleSharePtr > &triangleVector) {
    if (firstEdgePoints.size() == 2 | secondEdgePoints.size() == 2) {
        return;
    }

    int i;
    for (i = 0; i < firstEdgePoints.size() - 2 && i < secondEdgePoints.size() - 2; ++i) {
        splitOneLevel(edge(firstEdgePoints[i], secondEdgePoints[i]),
                      edge(firstEdgePoints[i + 1], secondEdgePoints[i + 1]),
                      l, triangleVector);
    }

    if (firstEdgePoints.size() == secondEdgePoints.size()) {
        splitOneLevel(edge(firstEdgePoints[i], secondEdgePoints[i]),
                      edge(firstEdgePoints[i + 1], secondEdgePoints[i + 1]),
                      l, triangleVector);
    } else {
        if (firstEdgePoints.size() > secondEdgePoints.size()) {
            split1Iter(splitEdge(edge(secondEdgePoints[i], firstEdgePoints[i]), l),
                       splitEdge(edge(secondEdgePoints[secondEdgePoints.size() - 1],
                                      firstEdgePoints[firstEdgePoints.size() - 1]), l), l, triangleVector);
        } else {
            split1Iter(splitEdge(edge(firstEdgePoints[firstEdgePoints.size() - 1],
                                      secondEdgePoints[secondEdgePoints.size() - 1]), l),
                       splitEdge(edge(firstEdgePoints[i], secondEdgePoints[i]), l), l, triangleVector);
        }
    }
}

void splitOneLevel(edge edge1, edge edge2, double l, vector<TriangleSharePtr > &result) {
    vector<pointSharePtr> upPoints = splitEdge(edge1, l);
    vector<pointSharePtr> downPoints = splitEdge(edge2, l);

    if (upPoints.size() + 1 == downPoints.size()) {
        for (int i = 0; i < downPoints.size() - 1; ++i) {
            addTriangle(downPoints[i], upPoints[i], downPoints[i + 1], result);
        }

        for (int i = 0; i < upPoints.size() - 1; ++i) {
            addTriangle(upPoints[i], upPoints[i + 1], downPoints[i + 1], result);
        }

    } else if (upPoints.size() == downPoints.size()) {
        for (int i = 0; i < upPoints.size() - 1; ++i) {
            if (edge(upPoints[i], downPoints[i + 1]).getLength()
                > edge(downPoints[i], upPoints[i + 1]).getLength()) {
                addTriangle(downPoints[i], upPoints[i + 1], downPoints[i + 1], result);
                addTriangle(downPoints[i], upPoints[i], upPoints[i + 1], result);
            } else {
                addTriangle(upPoints[i], upPoints[i + 1], downPoints[i + 1], result);
                addTriangle(downPoints[i], upPoints[i], downPoints[i + 1], result);
            }
        }
    } else {
        cout << upPoints.size() << " " << downPoints.size() << endl;
        cout << "error!!!!" << endl;
        outputAsFile(result);
        exit(EXIT_FAILURE);
    }
}

void outputAsFile(std::vector<TriangleSharePtr> &triangles) {
    std::ofstream of("result.poly");
    of << triangles.size() * 3 << " 2 0 0" << endl;
    for (int i = 0; i < triangles.size(); ++i) {
        of << i * 3 + 1 << " " << triangles[i]->getP1()->getX() << " " << triangles[i]->getP1()->getY() << endl;
        of << i * 3 + 2 << " " << triangles[i]->getP2()->getX() << " " << triangles[i]->getP2()->getY() << endl;
        of << i * 3 + 3 << " " << triangles[i]->getP3()->getX() << " " << triangles[i]->getP3()->getY() << endl;
    }


    of << triangles.size() * 3 << " 0" << endl;
    for (int i = 0; i < triangles.size(); ++i) {
        of << i * 3 + 1 << " " << i * 3 + 1 << " " << i * 3 + 2 << endl;
        of << i * 3 + 2 << " " << i * 3 + 2 << " " << i * 3 + 3 << endl;
        of << i * 3 + 3 << " " << i * 3 + 3 << " " << i * 3 + 1 << endl;
    }

    of << "0" << endl;
    of.close();
    system("pwd");
}

std::vector<pointSharePtr> splitEdge(const edge &e, const double &l) {
    int n = (int) ceil(e.getLength() / l);
    vector3d delta = *e.getV12() / n;
    vector<pointSharePtr> pointVector;
    pointVector.reserve((unsigned long) (n + 1));
    for (int i = 0; i < n + 1; ++i) {
        pointSharePtr pointAux = e.getP1();
        const std::shared_ptr<point> pointTmp = *pointAux + delta * i;
        pointVector.push_back(point::getPoint(pointTmp->getX(), pointTmp->getY(), pointTmp->getZ()));
    }
#ifdef TEST
    cout << "check split edge" << endl;
    cout << e << endl;

    for (pointSharePtr p : pointVector) {
        cout << *p;
    }
#endif
    return pointVector;
}


std::vector<triangle> split1(pointSharePtr p1, pointSharePtr p2, pointSharePtr p3, const double &l) {
    split1(triangle(p1, p2, p3), l);
}

std::vector<triangle> split2(const triangle &t, const double &l) {

}

std::vector<triangle> split2(pointSharePtr p1, pointSharePtr p2, pointSharePtr p3, const double &l) {
    split2(triangle(p1, p2, p3), l);
}

void addTriangle(pointSharePtr p1, pointSharePtr p2, pointSharePtr p3, std::vector<TriangleSharePtr> &result) {
    TriangleSharePtr  triangleSharePtr = std::make_shared<triangle>(p1, p2, p3);
    result.push_back(triangleSharePtr);

    pointTriangleMap[p1.get()].push_back(triangleSharePtr);
    pointTriangleMap[p2.get()].push_back(triangleSharePtr);
    pointTriangleMap[p3.get()].push_back(triangleSharePtr);
}
