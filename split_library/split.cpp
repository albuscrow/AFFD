//
// Created by ac on 6/17/15.
//

#include "split.h"
#include <algorithm>
#include <fstream>


void outputAsFile(std::vector<triangle> &vector);

using std::vector;
using std::cout;
using std::cin;
using std::endl;

std::vector<triangle> split1(const triangle &t, const double &l) {

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

    //调整前一条边的走向，另两条边都是从公共定点出发
    firstEdge = -firstEdge;
#ifdef TEST
    if (firstEdge.getP1() == secondEdge.getP1()) {
        cout << "test adjust direct of edge ok" << endl;
    }
#endif

    vector<point> firstEdgePoints = splitEdge(firstEdge, l);
    vector<point> secondEdgePoints = splitEdge(secondEdge, l);
    vector<triangle> result;

    //先把最顶上的三角形分割出来
    result.push_back(triangle(firstEdgePoints[1], firstEdgePoints[0], secondEdgePoints[1]));
    firstEdgePoints.erase(firstEdgePoints.begin());
    secondEdgePoints.erase(secondEdgePoints.begin());

    split1Iter(firstEdgePoints, secondEdgePoints, l, true, result);

    result.push_back(t);
    outputAsFile(result);
    return result;
}

void split1Iter(vector<point> firstEdgePoints, vector<point> secondEdgePoints,
                const double &l, bool isOriginal,
                vector<triangle> &triangleVector) {
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
            split1Iter(
                    //edgePoint1
                    splitEdge(edge(secondEdgePoints[i], firstEdgePoints[i]), l),
                    //edgePoint2
                    splitEdge(edge(secondEdgePoints[secondEdgePoints.size() - 1],
                                   firstEdgePoints[firstEdgePoints.size() - 1]), l),
                    //other params
                    l, false, triangleVector);
        } else {
            split1Iter(
                    //edgePoint2
                    splitEdge(edge(firstEdgePoints[firstEdgePoints.size() - 1],
                                   secondEdgePoints[secondEdgePoints.size() - 1]), l),
                    //edgePoint1
                    splitEdge(edge(firstEdgePoints[i], secondEdgePoints[i]), l),

                    //other params
                    l, false, triangleVector);
        }
    }
}

void splitOneLevel(edge edge1, edge edge2, double l, vector<triangle> &result) {
    vector<point> upPoints = splitEdge(edge1, l);
    vector<point> downPoints = splitEdge(edge2, l);

    if (upPoints.size() + 1 == downPoints.size()) {
        for (int i = 0; i < downPoints.size() - 1; ++i) {
            result.push_back(triangle(downPoints[i], upPoints[i], downPoints[i + 1]));
        }

        for (int i = 0; i < upPoints.size() - 1; ++i) {
            result.push_back(triangle(upPoints[i], upPoints[i + 1], downPoints[i + 1]));
        }

    } else if (upPoints.size() == downPoints.size()) {
        for (int i = 0; i < upPoints.size() - 1; ++i) {
            if (edge(upPoints[i], downPoints[i + 1]).getLength()
                > edge(downPoints[i], upPoints[i + 1]).getLength()) {
                result.push_back(triangle(downPoints[i], upPoints[i + 1], downPoints[i + 1]));
                result.push_back(triangle(downPoints[i], upPoints[i], upPoints[i + 1]));
            } else {
                result.push_back(triangle(upPoints[i], upPoints[i + 1], downPoints[i + 1]));
                result.push_back(triangle(downPoints[i], upPoints[i], downPoints[i + 1]));
            }
        }
    } else {
        cout << upPoints.size() << " " << downPoints.size() << endl;
        cout << "error!!!!" << endl;
        outputAsFile(result);
        exit(EXIT_FAILURE);
    }
}

void outputAsFile(std::vector<triangle> &vector) {
    std::ofstream of("result.poly");
    of << vector.size() * 3 << " 2 0 0" << endl;
    for (int i = 0; i < vector.size(); ++i) {
        of << i * 3 + 1<< " " << vector[i].getP1().getX() << " " << vector[i].getP1().getY() << endl;
        of << i * 3 + 2<< " " << vector[i].getP2().getX() << " " << vector[i].getP2().getY() << endl;
        of << i * 3 + 3<< " " << vector[i].getP3().getX() << " " << vector[i].getP3().getY() << endl;
    }


    of << vector.size() * 3 << " 0" << endl;
    for (int i = 0; i < vector.size(); ++i) {
        of << i * 3 + 1<< " " << i * 3 + 1 << " " << i * 3 + 2 << endl;
        of << i * 3 + 2<< " " << i * 3 + 2 << " " << i * 3 + 3 << endl;
        of << i * 3 + 3<< " " << i * 3 + 3 << " " << i * 3 + 1 << endl;
    }

    of << "0" << endl;
    of.close();
    system("pwd");
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


std::vector<triangle> split1(const point &p1, const point &p2, const point &p3, const double &l) {
    split1(triangle(p1, p2, p3), l);
}

std::vector<triangle> split2(const triangle &t, const double &l) {

}

std::vector<triangle> split2(const point &p1, const point &p2, const point &p3, const double &l) {
    split2(triangle(p1, p2, p3), l);
}

