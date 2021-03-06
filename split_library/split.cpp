//
// Created by ac on 6/17/15.
//

#include "split.h"
#include <algorithm>
#include <fstream>
#include <map>
#include <sstream>

using std::vector;
using std::cout;
using std::cin;
using std::endl;

void outputAsFile(std::vector<TriangleSharePtr> &triangles);

void cvt(const triangle &t);

void handle22(const vector<pointSharePtr> &firstEdgePoints, const vector<pointSharePtr> &secondEdgePoints,
              const double &l, vector<TriangleSharePtr> &triangleVector, const triangle &t);

void handle23(const vector<pointSharePtr> &firstEdgePoints, const vector<pointSharePtr> &secondEdgePoints,
              const double &l, vector<TriangleSharePtr> &triangleVector, const triangle &t);

void handle24(const vector<pointSharePtr> &firstEdgePoints, const vector<pointSharePtr> &secondEdgePoints,
              const double &l, vector<TriangleSharePtr> &triangleVector, const triangle &t);

void handle25(const vector<pointSharePtr> &firstEdgePoints, const vector<pointSharePtr> &secondEdgePoints,
              const double &l, vector<TriangleSharePtr> &triangleVector, const triangle &t);

std::map<point *, std::vector<TriangleSharePtr>> pointTriangleMap =
        std::map<point *, std::vector<TriangleSharePtr>>();

std::vector<TriangleSharePtr> split1(const triangle &t, const double &l, bool needClear) {
    if (needClear) {
        pointTriangleMap.clear();
        point::clearPointPool();
    }


    //取得三角形中两个较长的边
    vector<edge> edgeVector{t.getE12(), t.getE23(), t.getE31()};

    std::sort(edgeVector.begin(), edgeVector.end(), [](const edge edge1, const edge edge2) {
        return edge1.getLength() > edge2.getLength();
    });

    edge &e1 = edgeVector[0];
    edge &e2 = edgeVector[1];
    edge &e3 = edgeVector[2];

    vector<TriangleSharePtr> result;
    vector<pointSharePtr> e1Points = splitEdge(-e1, l);
    vector<pointSharePtr> e2Points = splitEdge(e2, l);
    vector<pointSharePtr> e3Points = splitEdge(e3, l);
    if (e1Points.size() == e2Points.size() + e3Points.size()) {
        e2Points.insert(std::end(e2Points), std::begin(e3Points), std::end(e3Points));
        size_t length = e1Points.size();
        addTriangle(e1Points[0], e1Points[1], e2Points[1], result, t);
        addTriangle(e1Points[length - 1], e1Points[length - 2], e2Points[length - 1], result, t);
        for (int i = 1; i < length - 2; ++i) {
            addTriangle(e1Points[i], e1Points[i + 1], e2Points[i], result, t);
            addTriangle(e1Points[i + 1], e2Points[i], e2Points[i + 1], result, t);
        }
        return result;
    }


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
//    if (firstEdge.getP1() == secondEdge.getP1()) {
//        cout << "test adjust direct of edge ok" << endl;
//    }
#endif

    vector<pointSharePtr> firstEdgePoints = splitEdge(firstEdge, l);
    vector<pointSharePtr> secondEdgePoints = splitEdge(secondEdge, l);


    //先把最顶上的三角形分割出来
    addTriangle(firstEdgePoints[1], firstEdgePoints[0], secondEdgePoints[1], result, t);
    if (firstEdgePoints.size() == 2 && secondEdgePoints.size() == 2) {
        outputAsFile(result);
        return result;
    } else if (firstEdgePoints.size() == 2 && secondEdgePoints.size() == 3) {
        addTriangle(firstEdgePoints[1], secondEdgePoints[1], secondEdgePoints[2], result, t);
        outputAsFile(result);
        return result;
    } else if (firstEdgePoints.size() == 3 && secondEdgePoints.size() == 2) {
        addTriangle(firstEdgePoints[2], firstEdgePoints[1], secondEdgePoints[1], result, t);
        outputAsFile(result);
        return result;
    } else if (firstEdgePoints.size() == 2 && secondEdgePoints.size() == 4) {
        addTriangle(firstEdgePoints[1], secondEdgePoints[1], secondEdgePoints[2], result, t);
        addTriangle(firstEdgePoints[1], secondEdgePoints[2], secondEdgePoints[3], result, t);
        outputAsFile(result);
        return result;
    } else if (firstEdgePoints.size() == 4 && secondEdgePoints.size() == 2) {
        addTriangle(firstEdgePoints[2], firstEdgePoints[1], secondEdgePoints[1], result, t);
        addTriangle(firstEdgePoints[3], firstEdgePoints[2], secondEdgePoints[1], result, t);
        outputAsFile(result);
        return result;
    }
    firstEdgePoints.erase(firstEdgePoints.begin());
    secondEdgePoints.erase(secondEdgePoints.begin());

    split1Iter(firstEdgePoints, secondEdgePoints, l, result, t);

//    result.push_back(std::shared_ptr<triangle>(new triangle(t)));
    for (int i = 0; i < CVT_TIMES; ++i) {
        cvt(t);
    }

//    addTriangle(t.getP1(), t.getP2(), t.getP3(), result, t);
//    outputAsFile(result);
//    result.pop_back();
#ifdef TEST
    //    cout << point::getPointPool().size() << endl;
    //    cout << pointTriangleMap.size() << endl;
    //    for (auto iter = pointTriangleMap.begin(); iter != pointTriangleMap.end(); ++iter) {
    //        cout << iter->second.size() << endl;
    //    }
#endif

    return result;
}

void cvt(const triangle &t) {
#ifdef TEST
//    cout << pointTriangleMap.size() << endl;
#endif
    std::map<point *, point> temp;
    for (auto iter = pointTriangleMap.begin(); iter != pointTriangleMap.end(); ++iter) {
        point *keyPoint = iter->first;
        double x = 0;
        double y = 0;
        int i = 0;
        for (TriangleSharePtr triangle : iter->second) {
            point p = triangle->getExcentre();
            x += p.getX();
            y += p.getY();
            ++i;
        }
        temp[keyPoint] = point(x / i, y / i, 0);
    }

    for (auto iter = pointTriangleMap.begin(); iter != pointTriangleMap.end(); ++iter) {
        point *keyPoint = iter->first;
        keyPoint->setX(temp[keyPoint].getX());
        keyPoint->setY(temp[keyPoint].getY());
    }
}

void split1Iter(const triangle &new_t, const double &l, vector<TriangleSharePtr> &result, const triangle &t) {
    //取得三角形中两个较长的边
    vector<edge> edgeVector{new_t.getE12(), new_t.getE23(), new_t.getE31()};

    std::sort(edgeVector.begin(), edgeVector.end(), [](const edge &edge1, const edge &edge2) {
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

    vector<pointSharePtr> firstEdgePoints = splitEdge(firstEdge, l);
    vector<pointSharePtr> secondEdgePoints = splitEdge(secondEdge, l);
    size_t a = firstEdgePoints.size();
    size_t b = secondEdgePoints.size();

    //先把最顶上的三角形分割出来
    addTriangle(firstEdgePoints[1], secondEdgePoints[0], secondEdgePoints[1], result, t);
//    addTriangle(new_t.getP1(), new_t.getP2(), firstEdgePoints[1], result, t);
//    addTriangle(firstEdgePoints[0], firstEdgePoints[1], firstEdgePoints[2], result, t);
//    addTriangle(secondEdgePoints[0], secondEdgePoints[1], secondEdgePoints[2], result, t);
    if (firstEdgePoints.size() == 2 && secondEdgePoints.size() == 2) {
        return;
    } else if (firstEdgePoints.size() == 2 && secondEdgePoints.size() == 3) {
        addTriangle(firstEdgePoints[1], secondEdgePoints[1], secondEdgePoints[2], result, t);
        return;
    } else if (firstEdgePoints.size() == 3 && secondEdgePoints.size() == 2) {
        addTriangle(firstEdgePoints[2], firstEdgePoints[1], secondEdgePoints[1], result, t);
        return;
    } else if (firstEdgePoints.size() == 2 && secondEdgePoints.size() == 4) {
        addTriangle(firstEdgePoints[1], secondEdgePoints[1], secondEdgePoints[2], result, t);
        addTriangle(firstEdgePoints[1], secondEdgePoints[2], secondEdgePoints[3], result, t);
        return;
    } else if (firstEdgePoints.size() == 4 && secondEdgePoints.size() == 2) {
        addTriangle(firstEdgePoints[2], firstEdgePoints[1], secondEdgePoints[1], result, t);
        addTriangle(firstEdgePoints[3], firstEdgePoints[2], secondEdgePoints[1], result, t);
        return;
    }
    firstEdgePoints.erase(firstEdgePoints.begin());
    secondEdgePoints.erase(secondEdgePoints.begin());

    split1Iter(firstEdgePoints, secondEdgePoints, l, result, t);
}


void split1Iter(vector<pointSharePtr> firstEdgePoints, vector<pointSharePtr> secondEdgePoints, const double &l,
                vector<TriangleSharePtr> &triangleVector, const triangle &t) {
//    size_t a = firstEdgePoints.size();
//    size_t b = secondEdgePoints.size();
    if (firstEdgePoints.size() == 2 || secondEdgePoints.size() == 2) {
        if (firstEdgePoints.size() + secondEdgePoints.size() == 4) {
            handle22(firstEdgePoints, secondEdgePoints, l, triangleVector, t);
        } else if (firstEdgePoints.size() + secondEdgePoints.size() == 5) {
            handle23(firstEdgePoints, secondEdgePoints, l, triangleVector, t);
        } else if (firstEdgePoints.size() + secondEdgePoints.size() == 6) {
            handle24(firstEdgePoints, secondEdgePoints, l, triangleVector, t);
        } else {
            handle25(firstEdgePoints, secondEdgePoints, l, triangleVector, t);
//            cout << "unimplement for 2 5" << endl;
//            exit(1);
        }
        return;
//        if (firstEdgePoints.size() == 2 && secondEdgePoints.size() == 3) {
//            splitOneLevel(edge(firstEdgePoints[0], secondEdgePoints[0]),
//                          edge(firstEdgePoints[1], secondEdgePoints[1]),
//                          l, triangleVector, t);
//
//        } else if (firstEdgePoints.size() == 3 && secondEdgePoints.size() == 2) {
//            splitOneLevel(edge(firstEdgePoints[0], secondEdgePoints[0]),
//                          edge(firstEdgePoints[1], secondEdgePoints[1]),
//                          l, triangleVector, t);
//
//        }

    }

    int i;
    for (i = 0; i < firstEdgePoints.size() - 2 && i < secondEdgePoints.size() - 2; ++i) {
        splitOneLevel(edge(firstEdgePoints[i], secondEdgePoints[i]),
                      edge(firstEdgePoints[i + 1], secondEdgePoints[i + 1]),
                      l, triangleVector, t);
    }

    if (firstEdgePoints.size() == secondEdgePoints.size()) {
        splitOneLevel(edge(firstEdgePoints[i], secondEdgePoints[i]),
                      edge(firstEdgePoints[i + 1], secondEdgePoints[i + 1]),
                      l, triangleVector, t);
    } else {
        if (firstEdgePoints.size() > secondEdgePoints.size()) {
            vector<pointSharePtr> lastMiddle = splitEdge(edge(firstEdgePoints[i], secondEdgePoints[i]), l);
            if (lastMiddle.size() == 2) {

                vector<pointSharePtr> temp = splitEdge(edge(firstEdgePoints[i], secondEdgePoints[i + 1]), l);
                if (temp.size() == 2) {
                    addTriangle(firstEdgePoints[i], secondEdgePoints[i + 1], secondEdgePoints[i], triangleVector, t);
                } else if (temp.size() == 3) {
                    addTriangle(firstEdgePoints[i], temp[1], secondEdgePoints[i], triangleVector, t);
                    addTriangle(temp[1], secondEdgePoints[i + 1], secondEdgePoints[i], triangleVector, t);
                } else {
                    cout << "error" << endl;
                    exit(1);
                }


                split1Iter(triangle(firstEdgePoints[firstEdgePoints.size() - 1], firstEdgePoints[i],
                                    secondEdgePoints[i + 1]), l, triangleVector, t);

//                for (int j = i; j < firstEdgePoints.size() - 1; ++j) {
//                    addTriangle(firstEdgePoints[j], secondEdgePoints[i + 1], firstEdgePoints[j + 1], triangleVector, t);
//                }
            } else {
                split1Iter(splitEdge(edge(secondEdgePoints[i], firstEdgePoints[i]), l),
                           splitEdge(edge(secondEdgePoints[secondEdgePoints.size() - 1],
                                          firstEdgePoints[firstEdgePoints.size() - 1]), l), l, triangleVector, t);
            }
        } else {
            vector<pointSharePtr> lastMiddle = splitEdge(edge(firstEdgePoints[i], secondEdgePoints[i]), l);
            if (lastMiddle.size() == 2) {
                vector<pointSharePtr> temp = splitEdge(edge(firstEdgePoints[i + 1], secondEdgePoints[i]), l);
                if (temp.size() == 2) {
                    addTriangle(firstEdgePoints[i], firstEdgePoints[i + 1], secondEdgePoints[i], triangleVector, t);
                } else if (temp.size() == 3) {
                    addTriangle(firstEdgePoints[i], firstEdgePoints[i + 1], temp[1], triangleVector, t);
                    addTriangle(firstEdgePoints[i], temp[1], secondEdgePoints[i], triangleVector, t);
                } else {
                    cout << "error" << endl;
                    exit(1);
                }
                split1Iter(triangle(secondEdgePoints[secondEdgePoints.size() - 1], firstEdgePoints[i + 1],
                                    secondEdgePoints[i]), l, triangleVector, t);
//                for (int j = i; j < secondEdgePoints.size() - 1; ++j) {
//                    addTriangle(secondEdgePoints[j], firstEdgePoints[i + 1], secondEdgePoints[j + 1], triangleVector, t);
//                }
            } else {
                split1Iter(splitEdge(edge(firstEdgePoints[firstEdgePoints.size() - 1],
                                          secondEdgePoints[secondEdgePoints.size() - 1]), l),
                           splitEdge(edge(firstEdgePoints[i], secondEdgePoints[i]), l), l, triangleVector, t);
            }
        }
    }
}

void handle25(const vector<pointSharePtr> &firstEdgePoints, const vector<pointSharePtr> &secondEdgePoints,
              const double &l, vector<TriangleSharePtr> &triangleVector, const triangle &t) {
    splitOneLevel(edge(firstEdgePoints[0], secondEdgePoints[0]),
                  edge(firstEdgePoints[1], secondEdgePoints[1]),
                  l, triangleVector, t);
    if (firstEdgePoints.size() == 5) {
        vector<pointSharePtr> edgeVector = splitEdge(edge(firstEdgePoints[2], secondEdgePoints[1]), l);
        addTriangle(firstEdgePoints[4], firstEdgePoints[3], edgeVector[1], triangleVector, t);
        addTriangle(edgeVector[1], firstEdgePoints[3], firstEdgePoints[2], triangleVector, t);
        addTriangle(edgeVector[1], firstEdgePoints[2], secondEdgePoints[1], triangleVector, t);
        addTriangle(secondEdgePoints[1], firstEdgePoints[2], firstEdgePoints[2], triangleVector, t);
    } else {
        vector<pointSharePtr> edgeVector = splitEdge(edge(firstEdgePoints[1], secondEdgePoints[2]), l);
        addTriangle(secondEdgePoints[4], edgeVector[1], secondEdgePoints[3], triangleVector, t);
        addTriangle(secondEdgePoints[3], edgeVector[1], secondEdgePoints[2], triangleVector, t);
        addTriangle(secondEdgePoints[2], edgeVector[1], firstEdgePoints[1], triangleVector, t);
        addTriangle(secondEdgePoints[2], firstEdgePoints[1], secondEdgePoints[1], triangleVector, t);
    }
}

void handle24(const vector<pointSharePtr> &firstEdgePoints, const vector<pointSharePtr> &secondEdgePoints,
              const double &l, vector<TriangleSharePtr> &triangleVector, const triangle &t) {
    if (firstEdgePoints.size() == 2) { // 2, 4
        vector<pointSharePtr> temp = splitEdge(edge(firstEdgePoints[1], secondEdgePoints[0]), l);
        if (temp.size() == 2) {
            addTriangle(firstEdgePoints[1], firstEdgePoints[0], secondEdgePoints[0], triangleVector, t);
        } else if (temp.size() == 3) {
            addTriangle(firstEdgePoints[1], firstEdgePoints[0], temp[1], triangleVector, t);
            addTriangle(firstEdgePoints[0], secondEdgePoints[0], temp[1], triangleVector, t);
        } else {
            cout << "unhandle case !!!!!!!!\n temp" << endl;
            cout << temp.size() << endl;
            exit(1);
        }

        vector<pointSharePtr> edgeVector = splitEdge(edge(secondEdgePoints[secondEdgePoints.size() - 1],
                                                          firstEdgePoints[firstEdgePoints.size() - 1]), l);
        addTriangle(secondEdgePoints[secondEdgePoints.size() - 2],
                    secondEdgePoints[secondEdgePoints.size() - 1], edgeVector[1], triangleVector, t);

        if (edgeVector.size() == 2) {

            addTriangle(secondEdgePoints[secondEdgePoints.size() - 3],
                        secondEdgePoints[secondEdgePoints.size() - 2], edgeVector[1], triangleVector, t);

            addTriangle(secondEdgePoints[secondEdgePoints.size() - 4],
                        secondEdgePoints[secondEdgePoints.size() - 3], edgeVector[1], triangleVector, t);
        } else {
            vector<pointSharePtr> newFirstPointVector;
            for (int i = (int) (secondEdgePoints.size() - 2); i >= 0; --i) {
                newFirstPointVector.push_back(secondEdgePoints[i]);
            }
            edgeVector.erase(edgeVector.begin());
            split1Iter(newFirstPointVector, edgeVector, l, triangleVector, t);
        }
    } else { // 4, 2

        vector<pointSharePtr> temp = splitEdge(edge(firstEdgePoints[0], secondEdgePoints[1]), l);
        if (temp.size() == 2) {
            addTriangle(firstEdgePoints[0], secondEdgePoints[0], secondEdgePoints[1], triangleVector, t);
        } else if (temp.size() == 3) {
            addTriangle(firstEdgePoints[0], secondEdgePoints[0], temp[1], triangleVector, t);
            addTriangle(secondEdgePoints[0], secondEdgePoints[1], temp[1], triangleVector, t);
        } else {
            cout << "unhandle case !!!!!!!!\n temp" << endl;
            cout << temp.size() << endl;
            exit(1);
        }

        vector<pointSharePtr> edgeVector = splitEdge(edge(firstEdgePoints[firstEdgePoints.size() - 1],
                                                          secondEdgePoints[secondEdgePoints.size() - 1]), l);
        addTriangle(firstEdgePoints[firstEdgePoints.size() - 1],
                    firstEdgePoints[firstEdgePoints.size() - 2], edgeVector[1], triangleVector, t);

        if (edgeVector.size() == 2) {
            addTriangle(secondEdgePoints[firstEdgePoints.size() - 2],
                        secondEdgePoints[firstEdgePoints.size() - 3], edgeVector[1], triangleVector, t);

            addTriangle(secondEdgePoints[firstEdgePoints.size() - 3],
                        secondEdgePoints[firstEdgePoints.size() - 4], edgeVector[1], triangleVector, t);
        } else {
            vector<pointSharePtr> newSecondPointVector;
            for (int i = (int) (firstEdgePoints.size() - 2); i >= 0; --i) {
                newSecondPointVector.push_back(firstEdgePoints[i]);
            }
            edgeVector.erase(edgeVector.begin());
            split1Iter(edgeVector, newSecondPointVector, l, triangleVector, t);
        }
    }
}

void handle23(const vector<pointSharePtr> &firstEdgePoints, const vector<pointSharePtr> &secondEdgePoints,
              const double &l, vector<TriangleSharePtr> &triangleVector, const triangle &t) {
    vector<pointSharePtr> edgeVector;
    if (firstEdgePoints.size() == 3) {
        edgeVector = splitEdge(edge(firstEdgePoints[2], secondEdgePoints[1]), l);
    } else {
        edgeVector = splitEdge(edge(firstEdgePoints[1], secondEdgePoints[2]), l);
    }
    if (edgeVector.size() == 5) {
        if (firstEdgePoints.size() == 3) {
            addTriangle(firstEdgePoints[2], firstEdgePoints[1], edgeVector[1], triangleVector, t);
            addTriangle(firstEdgePoints[1], firstEdgePoints[0], edgeVector[1], triangleVector, t);
            addTriangle(edgeVector[1], firstEdgePoints[0], edgeVector[2], triangleVector, t);
            addTriangle(edgeVector[2], firstEdgePoints[0], edgeVector[3], triangleVector, t);
            addTriangle(firstEdgePoints[0], secondEdgePoints[0], edgeVector[3], triangleVector, t);
            addTriangle(edgeVector[3], secondEdgePoints[0], edgeVector[4], triangleVector, t);
        } else {
            addTriangle(edgeVector[0], firstEdgePoints[1], edgeVector[1], triangleVector, t);
            addTriangle(firstEdgePoints[0], secondEdgePoints[0], edgeVector[1], triangleVector, t);
            addTriangle(edgeVector[1], secondEdgePoints[0], edgeVector[2], triangleVector, t);
            addTriangle(edgeVector[2], secondEdgePoints[0], edgeVector[3], triangleVector, t);
            addTriangle(edgeVector[3], secondEdgePoints[0], secondEdgePoints[1], triangleVector, t);
            addTriangle(edgeVector[3], secondEdgePoints[1], edgeVector[4], triangleVector, t);
        }
    } else {
        splitOneLevel(edge(firstEdgePoints[0], secondEdgePoints[0]),
                      edge(firstEdgePoints[1], secondEdgePoints[1]),
                      l, triangleVector, t);
        if (firstEdgePoints.size() == 3) {
            if (edgeVector.size() == 3) {
                addTriangle(firstEdgePoints[2], firstEdgePoints[1], edgeVector[1], triangleVector, t);

                vector<pointSharePtr> temp = splitEdge(edge(firstEdgePoints[1], secondEdgePoints[1]), l);
                if (temp.size() == 2) {
                    addTriangle(firstEdgePoints[1], secondEdgePoints[1], edgeVector[1], triangleVector, t);
                } else {
                    addTriangle(firstEdgePoints[1], temp[1], edgeVector[1], triangleVector, t);
                    addTriangle(temp[1], secondEdgePoints[1], edgeVector[1], triangleVector, t);
                }
            } else if (edgeVector.size() == 2) {
                addTriangle(firstEdgePoints[2], firstEdgePoints[1], secondEdgePoints[1], triangleVector, t);
            } else if (edgeVector.size() == 4) {
                vector<pointSharePtr> temp = splitEdge(edge(firstEdgePoints[1], secondEdgePoints[1]), l);
                if (temp.size() == 3) {
                    addTriangle(firstEdgePoints[2], firstEdgePoints[1], edgeVector[1], triangleVector, t);
                    addTriangle(edgeVector[1], firstEdgePoints[1], temp[1], triangleVector, t);
                    addTriangle(edgeVector[1], temp[1], edgeVector[2], triangleVector, t);
                    addTriangle(edgeVector[2], temp[1], edgeVector[3], triangleVector, t);
                } else {
                    cout << "unhandle case !!!!!!!!" << endl;
                    cout << edgeVector.size() << endl;
                    exit(1);
                }
            } else if (edgeVector.size() == 5) {
                //todo
                vector<pointSharePtr> temp = splitEdge(edge(firstEdgePoints[1], secondEdgePoints[1]), l);
                cout << "unhandle case !!!!!!!! implement" << endl;
                cout << "edge" << edgeVector.size() << "temp" << temp.size() << endl;
                exit(1);
            } else {
                cout << "unhandle case !!!!!!!!" << endl;
                cout << edgeVector.size() << endl;
                exit(1);
            }
        } else {
//            vector<pointSharePtr> edgeVector = splitEdge(edge(firstEdgePoints[1], secondEdgePoints[2]), l);
            if (edgeVector.size() == 3) {
                addTriangle(edgeVector[1], secondEdgePoints[1], secondEdgePoints[2], triangleVector, t);

                vector<pointSharePtr> temp = splitEdge(edge(firstEdgePoints[1], secondEdgePoints[1]), l);
                if (temp.size() == 2) {
                    addTriangle(firstEdgePoints[1], secondEdgePoints[1], edgeVector[1], triangleVector, t);
                } else {
                    addTriangle(firstEdgePoints[1], temp[1], edgeVector[1], triangleVector, t);
                    addTriangle(temp[1], secondEdgePoints[1], edgeVector[1], triangleVector, t);
                }

            } else if (edgeVector.size() == 2) {
                addTriangle(firstEdgePoints[1], secondEdgePoints[1], secondEdgePoints[2], triangleVector, t);
            } else if (edgeVector.size() == 4) {
                vector<pointSharePtr> temp = splitEdge(edge(firstEdgePoints[1], secondEdgePoints[1]), l);
                if (temp.size() == 3) {
                    addTriangle(edgeVector[0], temp[1], edgeVector[1], triangleVector, t);
                    addTriangle(edgeVector[1], temp[1], temp[2], triangleVector, t);
                    addTriangle(edgeVector[1], temp[2], edgeVector[2], triangleVector, t);
                    addTriangle(temp[2], edgeVector[3], edgeVector[2], triangleVector, t);
                } else {
                    cout << "unhandle case !!!!!!!!" << endl;
                    cout << edgeVector.size() << endl;
                    exit(1);
                }
            } else if (edgeVector.size() == 5) {
                //todo
                vector<pointSharePtr> temp = splitEdge(edge(firstEdgePoints[1], secondEdgePoints[1]), l);
                cout << "unhandle case !!!!!!!! implement" << endl;
                cout << "edge" << edgeVector.size() << "temp" << temp.size() << endl;
                exit(1);
            } else {
                cout << "unhandle case !!!!!!!!" << endl;
                cout << edgeVector.size() << endl;
                exit(1);
            }
        }
    }
}

void handle22(const vector<pointSharePtr> &firstEdgePoints, const vector<pointSharePtr> &secondEdgePoints,
              const double &l, vector<TriangleSharePtr> &triangleVector, const triangle &t) {
    splitOneLevel(edge(firstEdgePoints[0], secondEdgePoints[0]),
                  edge(firstEdgePoints[1], secondEdgePoints[1]),
                  l, triangleVector, t);
}

void splitOneLevel(edge edge1, edge edge2, double l, vector<TriangleSharePtr> &result, const triangle &t) {
    vector<pointSharePtr> upPoints = splitEdge(edge1, l);
    vector<pointSharePtr> downPoints = splitEdge(edge2, l);

    if (upPoints.size() + 1 == downPoints.size()) {
        for (int i = 0; i < downPoints.size() - 1; ++i) {
            addTriangle(downPoints[i], upPoints[i], downPoints[i + 1], result, t);
        }

        for (int i = 0; i < upPoints.size() - 1; ++i) {
            addTriangle(upPoints[i], upPoints[i + 1], downPoints[i + 1], result, t);
        }

    } else if (upPoints.size() == downPoints.size()) {
        for (int i = 0; i < upPoints.size() - 1; ++i) {
            if (edge(upPoints[i], downPoints[i + 1]).getLength()
                > edge(downPoints[i], upPoints[i + 1]).getLength()) {
                addTriangle(downPoints[i], upPoints[i + 1], downPoints[i + 1], result, t);
                addTriangle(downPoints[i], upPoints[i], upPoints[i + 1], result, t);
            } else {
                addTriangle(upPoints[i], upPoints[i + 1], downPoints[i + 1], result, t);
                addTriangle(downPoints[i], upPoints[i], downPoints[i + 1], result, t);
            }
        }
    } else if (upPoints.size() + 2 == downPoints.size()) {
        addTriangle(downPoints[0], upPoints[0], downPoints[1], result, t);
        addTriangle(downPoints[downPoints.size() - 2],
                    upPoints[upPoints.size() - 1],
                    downPoints[downPoints.size() - 1], result, t);
        downPoints.erase(downPoints.begin());
        for (int i = 0; i < upPoints.size() - 1; ++i) {
            if (edge(upPoints[i], downPoints[i + 1]).getLength()
                > edge(downPoints[i], upPoints[i + 1]).getLength()) {
                addTriangle(downPoints[i], upPoints[i + 1], downPoints[i + 1], result, t);
                addTriangle(downPoints[i], upPoints[i], upPoints[i + 1], result, t);
            } else {
                addTriangle(upPoints[i], upPoints[i + 1], downPoints[i + 1], result, t);
                addTriangle(downPoints[i], upPoints[i], downPoints[i + 1], result, t);
            }
        }
    } else {
        cout << upPoints.size() << " " << downPoints.size() << endl;
        cout << "error!!!!" << endl;
        outputAsFile(result);
        exit(EXIT_FAILURE);
    }
}

int index = 0;

void outputAsFile(std::vector<TriangleSharePtr> &triangles) {
#ifdef TEST
    std::string s1 = "result";
    std::string s2 = ".poly";
    std::ofstream of(s1 + std::to_string(index) + s2);
    ++index;
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
#endif
}

std::vector<pointSharePtr> splitEdge(const edge &e, const double &l) {
    int n = (int) round(e.getLength() / l);
    if (n == 0) {
        n = 1;
    }
    vector3d delta = e.getV12() / n;
    vector<pointSharePtr> pointVector;
    pointVector.reserve((unsigned long) (n + 1));
    for (int i = 0; i < n + 1; ++i) {
        pointSharePtr pointAux = e.getP1();
        point pointTmp = *pointAux + delta * i;
        pointVector.push_back(point::getPoint(pointTmp.getX(), pointTmp.getY(), pointTmp.getZ()));
    }
#ifdef TEST
//    cout << "check split edge" << endl;
//    cout << e << endl;
//
//    for (pointSharePtr p : pointVector) {
//        cout << *p;
//    }
#endif
    return pointVector;
}


std::vector<TriangleSharePtr> split1(pointSharePtr p1, pointSharePtr p2, pointSharePtr p3, const double &l,
                                     bool needClear) {
    split1(triangle(p1, p2, p3), l, needClear);
}

std::vector<triangle> split2(const triangle &t, const double &l) {

}

std::vector<triangle> split2(pointSharePtr p1, pointSharePtr p2, pointSharePtr p3, const double &l) {
    split2(triangle(p1, p2, p3), l);
}

void addTriangle(pointSharePtr p1, pointSharePtr p2, pointSharePtr p3, std::vector<TriangleSharePtr> &result,
                 const triangle &t) {
    TriangleSharePtr triangleSharePtr = std::make_shared<triangle>(p1, p2, p3);
    result.push_back(triangleSharePtr);

    if (t.containPoint(p1.get())) {
        pointTriangleMap[p1.get()].push_back(triangleSharePtr);
    }

    if (t.containPoint(p2.get())) {
        pointTriangleMap[p2.get()].push_back(triangleSharePtr);
    }

    if (t.containPoint(p3.get())) {
        pointTriangleMap[p3.get()].push_back(triangleSharePtr);
    }
}
