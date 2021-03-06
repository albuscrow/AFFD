#include "common_data.h"
#include <algorithm>
#include <QTime>
#include <lapackpp/blas3pp.h>
#include <lapackpp/laslv.h>
#include <memory>
#include <map>
#include "triangle.h"
#include "obj_data.h"
#include "split_library/split.h"


#include <sstream>
#include <vector_types.h>

using std::cout;
using std::endl;
using std::vector;
using std::list;

using namespace objdata;

QTime calcTimeLoad, calcTimeChange;

/* GPU 函数 */
void callCudaThreadSynchronize();

void loadTriangleMatrixD();

void loadMatrixBSplineD();

void preCalcD(CommonData *commonData);

void copyCtrlPointD(CommonData *commonData);

void fromParamToCoordD(CommonData *commonData);

void loadTriangleListD(const vector<Triangle> &triangleList, int *triangle_adjacent_table, int deg);

void generateUVW(int samplePointPerEdge);

void calcTriangleCtrlPoint(bool adjust_silhouette, bool use_pn, AlgorithmType algo_type);

void calcSampleValue(AlgorithmType algo_type);

void tessellateD(bool firstLoad, float maxX, float maxY, float maxZ, AlgorithmType algo_type);

#ifdef TRUTH

void generateUVW_truth(int samplePointPerEdge);

void calcSampleValue_truth();

void matrixMul1_truth();

void tessellateD_truth(bool firstLoad);

#endif

void freeTessMemD();

void freeModelMemD();

void freeMemD();

void printCudaError(const char *file, const char *function, int line);

/*----------------------------------------------------------------------------------------*/

DirectPoint::DirectPoint(const VertexCoord &p)
        : CtrlPoint(p.x(), p.y(), p.z(), false) {
    m_fU = p.x();
    m_fV = p.y();
    m_fW = p.z();
}

DirectPoint::DirectPoint(double u, double v, double w, double x, double y, double z)
        : CtrlPoint(x, y, z, false) {
    m_fU = u;
    m_fV = v;
    m_fW = w;
}

std::ostream &operator<<(std::ostream &out, const DirectPoint &p) {
    out << p.u() << " " << p.v() << " " << p.w() << " " << p.x() << " " << p.y() << " " << p.z();
    return out;
}

/*----------------------------------------------------------------------------------------*/

int factorial(int n);

double power(double a, int n);

template<int n>
VertexCoord BezierTriangle<n>::calcValue(const objdata::VertexCoord &baryCoord) {
    VertexCoord result(0.0, 0.0, 0.0);
    int idx = 0;
    for (int i = n; i >= 0; --i)
        for (int j = n - i; j >= 0; --j) {
            int k = n - i - j;
            result += ctrl_points[idx++] * (factorial(n) / factorial(i) / factorial(j) / factorial(k)
                                            * power(baryCoord.x(), i) * power(baryCoord.y(), j) *
                                            power(baryCoord.z(), k));
        }
    return result;
}

/*----------------------------------------------------------------------------------------*/

EdgeWithNormal::EdgeWithNormal() {
    v_max = n_min[1] = n_max[1] = face_id[1] = -1;
#ifdef IS_BORDER_INFO
    border = false;
#endif
}

EdgeWithNormal::EdgeWithNormal(int vmax, int nmin, int nmax, int faceid) {
    v_max = vmax;
    n_min[1] = n_max[1] = face_id[1] = -1;
#ifdef IS_BORDER_INFO
    border = false;
#endif

    setXthNormal(0, nmin, nmax, faceid);
}

void EdgeWithNormal::setXthNormal(int edge_id, int nmin, int nmax, int faceid) {
    n_min[edge_id] = nmin;
    n_max[edge_id] = nmax;
    n_count = edge_id + 1;
    face_id[edge_id] = faceid;
}

std::ostream &operator<<(std::ostream &out, const EdgeWithNormal &edge) {
    out << "末顶点" << edge.v_max << ", n = " << edge.n_count << ", 第一个法向(" << edge.n_min[0] << ", " << edge.n_max[0] <<
    ")";
    if (edge.n_count == 2)
        out << ", 第二个法向(" << edge.n_min[1] << ", " << edge.n_max[1] << ")";
    return out;
}

/*----------------------------------------------------------------------------------------*/

TrimmedPolygon::TrimmedPolygon(bool useTexture, int mtlIdx, const VertexCoord *vo, int origin_facd_idx) {
    vertexList.reserve(6);
    normalList.reserve(6);
    normal_adj_list.reserve(6);
    textureList.reserve(6);
    v_origin[0] = vo[0];
    v_origin[1] = vo[1];
    v_origin[2] = vo[2];
    m_bUseTexture = useTexture;
    m_nMtlIdx = mtlIdx;
    origin_face_idx_ = origin_facd_idx;
}

void TrimmedPolygon::normalizeAllNormals() {
    for (vector<NormalCoord>::size_type i = 0; i < normalList.size(); ++i)
        normalList[i].normalize();
    for (vector<NormalCoord>::size_type i = 0; i < normal_adj_list.size(); ++i)
        normal_adj_list[i].normalize();
}

/*----------------------------------------------------------------------------------------*/

Triangle::Triangle(const VertexCoord &v0, const VertexCoord &v1, const VertexCoord &v2,
                   const NormalCoord &n0, const NormalCoord &n1, const NormalCoord &n2,
                   const NormalCoord &n_adj0, const NormalCoord &n_adj1, const NormalCoord &n_adj2,
#ifdef LINE
        const VertexCoord &bary_ori0, const VertexCoord &bary_ori1, const VertexCoord &bary_ori2,
#endif
                   const VertexCoord *vo, int origin_face_idx,
                   int nc0, int nc1, int nc2,
                   const TextureCoord &vt0, const TextureCoord &vt1, const TextureCoord &vt2) {
    v[0] = v0;
    v[1] = v1;
    v[2] = v2;
    n[0] = n0;
    n[1] = n1;
    n[2] = n2;
    n_adj[0] = n_adj0;
    n_adj[1] = n_adj1;
    n_adj[2] = n_adj2;
    v_origin[0] = vo[0];
    v_origin[1] = vo[1];
    v_origin[2] = vo[2];
#ifdef LINE
    bary_origin[0] = bary_ori0;
    bary_origin[1] = bary_ori1;
    bary_origin[2] = bary_ori2;
#endif
    n_count[0] = nc0;
    n_count[1] = nc1;
    n_count[2] = nc2;
    origin_face_idx_ = origin_face_idx;
    vt[0] = vt0;
    vt[1] = vt1;
    vt[2] = vt2;
}

/*----------------------------------------------------------------------------------------*/

#ifndef ZERO
#define ZERO 0.0000001
#endif
const double CommonData::EXPAND = 0.001;
//const double CommonData::EXPAND = 0.0;
const double CommonData::PI = 3.1415926536;
//const double CommonData::TAN_FACTOR = 0.01;

/*----------------------------------------------------------------------------------------*/

CommonData::CommonData() {
    //fout.open("common_data.txt");
    vertexParamList.reserve(MAXVERTEX);    // 预留一定空间，提高性能
    vertexCoordList.reserve(MAXVERTEX);    // 预留一定空间，提高性能
    faceList.reserve(MAXFACE);            // 预留一定空间，提高性能
    objData = new ObjData();
    m_eAlgorithm = AFFD;                // 默认算法为 AFFD
    m_nFFDThreadCount = 128;    // 使用 GPU 加速 FFD 算法时线程块默认含有128线程
    m_bGPU = true;
    m_bLoaded = false;
    m_bLoadingEdit = false;
    adjust_silhouette_ = true;
    adjust_split_points_ = true;
    use_pn_ = true;
    algorithm_type_ = LZQ;
    //algorithm_type_ = PN_CUTTING;
    //algorithm_type_ = PN_NO_CUTTING;

    //adjust_silhouette_ = false;
    //adjust_split_points_ = false;

//#define ALGO_TEST
#ifndef ALGO_TEST
    m_nSamplePointCount = 2;        // Bézier 曲面片三角化时各方向的采样点数量为11
    //m_nSamplePointCount = 30;		// Bézier 曲面片三角化时各方向的采样点数量为11
#else
    m_nSamplePointCount = 2;		// Bézier 曲面片三角化时各方向的采样点数量为2
#endif

#ifndef ALGO_TEST
    m_nOrder[U] = m_nOrder[V] = m_nOrder[W] = 3;    // 默认的 B 样条体三个方向的阶数
    //m_nCtrlPointCount[U] = m_nCtrlPointCount[V] =  m_nCtrlPointCount[W] = 5;	// 默认的B样条体三个方向的控制顶点数量
    //m_nOrder[U] = m_nOrder[V] = m_nOrder[W] = 4;	// 默认的 B 样条体三个方向的阶数
    //m_nCtrlPointCount[U] = m_nCtrlPointCount[V] =  m_nCtrlPointCount[W] = 7;	// 默认的B样条体三个方向的控制顶点数量
    //m_nCtrlPointCount[U] = m_nCtrlPointCount[V] =  m_nCtrlPointCount[W] = 4;
    //m_nCtrlPointCount[U] = m_nCtrlPointCount[V] =  m_nCtrlPointCount[W] = 3;
    //m_nOrder[U] = m_nOrder[V] = m_nOrder[W] = 2;
    //m_nCtrlPointCount[U] = m_nCtrlPointCount[V] =  m_nCtrlPointCount[W] = 2;
    //m_nOrder[U] = m_nOrder[V] = m_nOrder[W] = 4;
    //m_nCtrlPointCount[U] = m_nCtrlPointCount[V] =  m_nCtrlPointCount[W] = 6;
    //m_nOrder[U] = 2;				m_nOrder[V] = 3;				m_nOrder[W] = 4;
    //m_nCtrlPointCount[U] = 5;		m_nCtrlPointCount[V] = 3;		m_nCtrlPointCount[W] = 6;
    //m_nOrder[U] = m_nOrder[V] = m_nOrder[W] = 3;
    //m_nCtrlPointCount[U] = m_nCtrlPointCount[V] =  m_nCtrlPointCount[W] = 3;

    //m_nCtrlPointCount[U] = 5;	m_nCtrlPointCount[V] = 8;	m_nCtrlPointCount[W] = 5; // 船
    //m_nCtrlPointCount[U] = 5;	m_nCtrlPointCount[V] = 9;	m_nCtrlPointCount[W] = 5; // 棋子
    m_nCtrlPointCount[U] = 5;
    m_nCtrlPointCount[V] = 5;
    m_nCtrlPointCount[W] = 5; // 希腊罐子

    //m_nCtrlPointCount[U] = 5;	m_nCtrlPointCount[V] = 5; m_nCtrlPointCount[W] = 5;		// 罐子
    //m_nCtrlPointCount[U] = 10;	m_nCtrlPointCount[V] = 5; m_nCtrlPointCount[W] = 5;	// 鸟
    //m_nCtrlPointCount[U] = 11;	m_nCtrlPointCount[V] = 8; m_nCtrlPointCount[W] = 5;		// 蜗牛
    //m_nCtrlPointCount[U] = 11;	m_nCtrlPointCount[V] = 5; m_nCtrlPointCount[W] = 5;		// 飞船
    //m_nCtrlPointCount[U] = 5;	m_nCtrlPointCount[V] = 8; m_nCtrlPointCount[W] = 5;		// 树
#else
    //m_nCtrlPointCount[U] = 9;	m_nCtrlPointCount[V] = 4; m_nCtrlPointCount[W] = 4;

    //m_nOrder[U] = m_nOrder[V] = m_nOrder[W] = 3;
    //m_nCtrlPointCount[U] = m_nCtrlPointCount[V] = m_nCtrlPointCount[W] = 3;
    //m_nOrder[U] = 2;				m_nOrder[V] = 2;				m_nOrder[W] = 3;
    //m_nCtrlPointCount[U] = 2;		m_nCtrlPointCount[V] = 2;		m_nCtrlPointCount[W] = 4;
    //m_nOrder[U] = 4;				m_nOrder[V] = 4;				m_nOrder[W] = 4;
    //m_nCtrlPointCount[U] = 15;		m_nCtrlPointCount[V] = 15;		m_nCtrlPointCount[W] = 15;
    //m_nOrder[U] = 2;				m_nOrder[V] = 2;				m_nOrder[W] = 2;
    //m_nCtrlPointCount[U] = 2;		m_nCtrlPointCount[V] = 2;		m_nCtrlPointCount[W] = 2;
#endif

    /* 各种次数 Bézier 曲线的节点向量 */
    /* 前一半 (i + 1) 个节点都是0.0，后一半 (i + 1) 个节点都是1.0 */
    /* i 为 Bézier 曲线的次数 */
    //for (int i = 1; i <= 9; ++i)
    //{
    //for (int j = 0; j <= i; ++j)
    //m_fTrimmedBezierKnotPoint[i][j] = 0.0f;
    //for (int j = i + 1; j <= i * 2 + 1; ++j)
    //m_fTrimmedBezierKnotPoint[i][j] = 1.0f;
    //}
    faceMtlList.clear();
    faceMtlCountList.clear();
    edge_table_ = 0;
    face_adjacent_table_ = 0;
    face_children_table_ = 0;
    triangle_adjacent_table_ = 0;
}

CommonData::~CommonData() {
    //fout.close();
    delete objData;
    delete[]edge_table_;
    delete[]face_adjacent_table_;
    delete[]face_children_table_;
    delete[]triangle_adjacent_table_;
    freeMemD();
}

//void CommonData::moveShip(int target, int id0, int id1, int xyz)
//{
//--id0;
//--id1;
//--target;
//VertexCoord v0 = objData->vertexCoordList[id0];
//VertexCoord v1 = objData->vertexCoordList[id1];
//VertexCoord v_target = objData->vertexCoordList[target];
//double value0, value1, valuec;
//if (xyz == 0)
//{
//value0 = v0.x();
//value1 = v1.x();
//valuec = v_target.x();
//}
//else if (xyz == 1)
//{
//value0 = v0.y();
//value1 = v1.y();
//valuec = v_target.y();
//}
//else
//{
//value0 = v0.z();
//value1 = v1.z();
//valuec = v_target.z();
//}
//double ratio = (valuec - value0) / (value1 - value0);
//VertexCoord v_target_new = (v1 - v0) * ratio + v0;
//cout << target + 1 << ": " << v_target << " | " << v_target_new << endl;
//}

/* 载入 obj 模型 */
void CommonData::loadObj(const char *fileName) {
    calcTimeLoad.start();
    objData->readObj(fileName);                // 将 obj 文件中的内容存放到相应的数据结构中

    vertexParamList.clear();
    vertexCoordList.clear();
    faceList.clear();
    m_bLoaded = true;
    m_bFirstLoad = true;
    delete[]edge_table_;
    delete[]face_adjacent_table_;
    delete[]face_children_table_;
    delete[]triangle_adjacent_table_;

    /* 将面片信息存放到新的地方 */
//#define selectFace
#ifdef selectFace
    vector<Face> temp_face_list;
    for (vector<Face>::size_type i = 0; i < objData->faceList.size(); ++i)
    {
        //if (i <= 41100)
        if (i <= 41098)
        {
            temp_face_list.push_back(objData->faceList[i]);
            //int faceSize = objData->faceList[i].vertexCoordIndex.size();
            //cout << "唯一面片" << endl;
            //for (int j = 0; j < faceSize; ++j)
            //{
                //int idx = objData->faceList[i].vertexCoordIndex[j];
                //VertexCoord vc = objData->vertexCoordList[idx];
                //NormalCoord n = objData->normalCoordList[idx];
                //cout << vc << "->" << n << endl;
            //}

            /*outFile << "i = " << i << std::endl << "\t";
            int faceSize = objData->faceList[i].vertexCoordIndex.size();
            for (int j = 0; j < faceSize; ++j)
            {
                int idx = objData->faceList[i].vertexCoordIndex[j];
                VertexCoord vc = objData->vertexCoordList[idx];
                outFile << "(" << vc.x() << ", " << vc.y() << ", " << vc.z() << ") ";
            }
            outFile << endl;

            faceSize = objData->faceList[i].textureCoordIndex.size();
            for (int j = 0; j < faceSize; ++j)
            {
                int idx = objData->faceList[i].textureCoordIndex[j];
                TextureCoord tc = objData->textureCoordList[idx];
                outFile << "(" << tc.u() << ", " << tc.v() << ", " << tc.w() << ") ";
            }
            outFile << endl << endl;*/
        }
    }
#else
    vector<Face> temp_face_list = objData->faceList;
    //faceList = objData->faceList;
#endif

    /* 记录原始坐标中 x, y, z 的最大、最小值 */
    unsigned long totalVertex = objData->vertexCoordList.size();
    m_fMin[X] = m_fMax[X] = objData->vertexCoordList[0].x();
    m_fMin[Y] = m_fMax[Y] = objData->vertexCoordList[0].y();
    m_fMin[Z] = m_fMax[Z] = objData->vertexCoordList[0].z();

    for (unsigned int i = 1; i < totalVertex; ++i) {
        if (objData->vertexCoordList[i].x() < m_fMin[X])
            m_fMin[X] = objData->vertexCoordList[i].x();
        else if (objData->vertexCoordList[i].x() > m_fMax[X])
            m_fMax[X] = objData->vertexCoordList[i].x();


        if (objData->vertexCoordList[i].y() < m_fMin[Y])
            m_fMin[Y] = objData->vertexCoordList[i].y();
        else if (objData->vertexCoordList[i].y() > m_fMax[Y])
            m_fMax[Y] = objData->vertexCoordList[i].y();


        if (objData->vertexCoordList[i].z() < m_fMin[Z])
            m_fMin[Z] = objData->vertexCoordList[i].z();
        else if (objData->vertexCoordList[i].z() > m_fMax[Z])
            m_fMax[Z] = objData->vertexCoordList[i].z();
    }

//    cout << "x = (" << m_fMin[X] << ", " << m_fMax[X] << ")" << endl;
//    cout << "y = (" << m_fMin[Y] << ", " << m_fMax[Y] << ")" << endl;
//    cout << "z = (" << m_fMin[Z] << ", " << m_fMax[Z] << ")" << endl;

    /* 将三个方向的最小、最大坐标调整到以(0, 0, 0)为中心的新位置 */
    double delta[3];
    for (int i = 0; i < 3; ++i) {
        delta[i] = -0.5 * (m_fMax[i] + m_fMin[i]);
        m_fMin[i] += delta[i];
        m_fMax[i] += delta[i];
    }

    double maxXYZ = m_fMax[X];
    if (m_fMax[Y] > maxXYZ)
        maxXYZ = m_fMax[Y];
    if (m_fMax[Z] > maxXYZ)
        maxXYZ = m_fMax[Z];
    /* 将模型中心点调整到(0, 0, 0)，并存放到新的地方 */
    for (unsigned int i = 0; i < totalVertex; ++i) {
        double x = objData->vertexCoordList[i].x();
        double y = objData->vertexCoordList[i].y();
        double z = objData->vertexCoordList[i].z();

        x += delta[X];
        y += delta[Y];
        z += delta[Z];

#ifdef NORMALIZE_TO_1
        x /= maxXYZ;
        y /= maxXYZ;
        z /= maxXYZ;
#endif

        VertexCoord v(x, y, z);
        vertexCoordList.push_back(v);

        VertexParam p(x, y, z);
        vertexParamList.push_back(p);
    }

#ifdef NORMALIZE_TO_1
    m_fMin[X] /= maxXYZ;
    m_fMax[X] /= maxXYZ;
    m_fMin[Y] /= maxXYZ;
    m_fMax[Y] /= maxXYZ;
    m_fMin[Z] /= maxXYZ;
    m_fMax[Z] /= maxXYZ;
#endif
    cout << "原始模型共" << objData->faceList.size() << "个面片, "
    << totalVertex << "个顶点, "
    << "minx = " << m_fMin[X] << ", maxx = " << m_fMax[X] << endl;;

    /* 计算模型半径 */
    for (int i = 0; i < 3; ++i)
        m_fLength[i] = (m_fMax[i] - m_fMin[i]) * 0.5;
    m_fTotalLength = sqrt(m_fLength[X] * m_fLength[X] + m_fLength[Y] * m_fLength[Y] + m_fLength[Z] * m_fLength[Z]);

    cout << "===================================================" << endl;
    int elapsedTimeLoad = calcTimeLoad.restart();
    cout << "载入\t" << elapsedTimeLoad << "\tCPU : 读入obj" << endl;

    /*---------------------------- 将多边形模型变成三角形模型 ----------------------------*/
    for (vector<Face>::size_type f = 0; f < temp_face_list.size(); ++f) {
        Face &reference = temp_face_list[f];
        for (vector<int>::size_type s = 0; s < reference.vertexCoordIndex.size() - 2; ++s) {
            Face temp_face;
            temp_face.vertexCoordIndex.push_back(reference.vertexCoordIndex[0]);
            temp_face.vertexCoordIndex.push_back(reference.vertexCoordIndex[s + 1]);
            temp_face.vertexCoordIndex.push_back(reference.vertexCoordIndex[s + 2]);
            if (reference.m_eFaceCase == V_T_N) {
                temp_face.textureCoordIndex.push_back(reference.textureCoordIndex[0]);
                temp_face.textureCoordIndex.push_back(reference.textureCoordIndex[s + 1]);
                temp_face.textureCoordIndex.push_back(reference.textureCoordIndex[s + 2]);
            }
            temp_face.normalCoordIndex.push_back(reference.normalCoordIndex[0]);
            temp_face.normalCoordIndex.push_back(reference.normalCoordIndex[s + 1]);
            temp_face.normalCoordIndex.push_back(reference.normalCoordIndex[s + 2]);
            temp_face.m_eFaceCase = reference.m_eFaceCase;
            temp_face.m_nMtlIdx = reference.m_nMtlIdx;
            faceList.push_back(temp_face);
        }
    }

    /*------------------------ 记录每条边的法向数量以及面片相邻信息 ----------------------*/
    edge_table_ = new list<EdgeWithNormal>[vertexCoordList.size()];
    face_adjacent_table_ = new list<int>[faceList.size()];
    face_children_table_ = new list<int>[faceList.size()];

    for (vector<Face>::size_type i = 0; i < faceList.size(); ++i) {
        Face &reference = faceList[i];
        for (vector<int>::size_type j = 0; j < reference.vertexCoordIndex.size(); ++j) {
            int v_id = reference.vertexCoordIndex[j];
            int n_id = reference.normalCoordIndex[j];
            int v_next_id = reference.vertexCoordIndex[0];
            int n_next_id = reference.normalCoordIndex[0];
            if (j != reference.vertexCoordIndex.size() - 1) {
                v_next_id = reference.vertexCoordIndex[j + 1];
                n_next_id = reference.normalCoordIndex[j + 1];
            }
            int v_max, v_min, n_max, n_min;
//            NormalCoord n_max_vertex, n_min_vertex;
            if (v_id > v_next_id) {
                v_max = v_id;
                v_min = v_next_id;
                n_max = n_id;
                n_min = n_next_id;
            } else {
                v_max = v_next_id;
                v_min = v_id;
                n_max = n_next_id;
                n_min = n_id;
            }
            bool exist = false;

            for (list<EdgeWithNormal>::iterator it = edge_table_[v_min].begin();
                 it != edge_table_[v_min].end(); ++it) {
                if (it->v_max == v_max)            // 这个边两端端点都相同
                {
                    face_adjacent_table_[it->face_id[0]].push_front(i);
                    face_adjacent_table_[i].push_front(it->face_id[0]);
                    if (it->n_count == 1)        // 这个边目前只有一个法向
                    {
                        it->face_id[1] = i;
                        if ((it->n_min[0] != n_min) || (it->n_max[0] != n_max))    // 新的法向和原有的不同
                        {
                            it->setXthNormal(1, n_min, n_max, i);
                        }
                    }
                    exist = true;
                    break;
                }
            }
            if (!exist)
                edge_table_[v_min].push_front(EdgeWithNormal(v_max, n_min, n_max, i));
        }
    }

#ifdef IS_BORDER_INFO
    // 找到边界边（只有一侧有面片的边）
    vector<int> border_face;                // 用于生成新的兔子模型
    for (vector<VertexCoord>::size_type i = 0; i < vertexCoordList.size(); ++i) {
        for (list<EdgeWithNormal>::iterator it = edge_table_[i].begin(); it != edge_table_[i].end(); ++it) {
            if (it->n_count == 1 && it->face_id[1] < 0)    // 边界边
            {
                it->border = true;
                border_face.push_back(it->face_id[0]);    // 用于生成新的兔子模型
            }
            else {
                it->border = false;
            }
        }
    }
#endif

}

void CommonData::calcFinalResult() {
#ifndef MORPH
    int elapsedTimeChange;
    cout << "\n===================================================\n" << endl;
    elapsedTimeChange = calcTimeChange.restart();
    cout << "改变\t" << elapsedTimeChange << "\tCPU : 改变控制顶点" << endl;
#endif
    if (m_bGPU) {
#ifndef MORPH
        cout << "改变\t**** GPU ****" << endl;
#endif
        copyCtrlPointD(this);
#ifndef MORPH
        callCudaThreadSynchronize();
        elapsedTimeChange = calcTimeChange.restart();
        cout << "改变\t" << elapsedTimeChange << "\tGPU : 将新的控制顶点传入显存" << endl;
#endif
        if (m_eAlgorithm == AFFD) {
            calcSampleValue(algorithm_type_);
#ifndef MORPH
            callCudaThreadSynchronize();
            elapsedTimeChange = calcTimeChange.restart();
            cout << "改变\t" << elapsedTimeChange << "\tGPU : 计算 Bezier 曲面片采样点的值" << endl;
#endif

            calcTriangleCtrlPoint(adjust_silhouette_, use_pn_, algorithm_type_);
#ifndef MORPH
            callCudaThreadSynchronize();
            elapsedTimeChange = calcTimeChange.restart();
            cout << "改变\t" << elapsedTimeChange << "\tGPU : 计算三角Bezier曲面片控制顶点" << endl;
#endif

            tessellateD(m_bFirstLoad, m_fMax[0], m_fMax[1], m_fMax[2], algorithm_type_);
#ifndef MORPH
            callCudaThreadSynchronize();
            elapsedTimeChange = calcTimeChange.restart();
            cout << "改变\t" << elapsedTimeChange << "\tGPU : 计算三角化点的值" << endl;
#endif

#ifdef TRUTH
            /*-------------------- 计算 ground truth 开始 ----------------------*/
            calcSampleValue_truth();
            matrixMul1_truth();
            tessellateD_truth(m_bFirstLoad);
            /*-------------------- 计算 ground truth 结束 ----------------------*/
#endif

            // 将未选中的直接编辑顶点移动到新位置
            for (vector<DirectPoint>::size_type idx = 0; idx < directPointVector.size(); ++idx) {
                if (directPointVector[idx].selected())
                    continue;
                double u = directPointVector[idx].u();
                double v = directPointVector[idx].v();
                double w = directPointVector[idx].w();
                directPointVector[idx] = fromParamToCoord(u, v, w);
            }
        }
        else {
            fromParamToCoordD(this);
        }
    }
    else {
        cout << "改变\t**** CPU ****" << endl;
        if (m_eAlgorithm == AFFD) {
        }
        else {
            /* 重新计算模型上所有顶点在物体空间中的坐标 */
            for (vector<VertexParam>::size_type idx = 0; idx < vertexParamList.size(); ++idx) {
                double u = vertexParamList[idx].u();
                double v = vertexParamList[idx].v();
                double w = vertexParamList[idx].w();
                vertexCoordList[idx] = fromParamToCoord(u, v, w);
            }
        }
    }
}

/* AFFD 算法的预计算 */
void CommonData::preCalc(bool reset_ctrl_point) {
    if (m_bFirstLoad) {
        loadTriangleMatrixD();
        loadMatrixBSplineD();
    }
    /* 首先作一些必要的内存释放工作 */
    for (int i = 0; i < MAXKNOTINTERVAL; ++i) {
        for (int j = 0; j < MAXKNOTINTERVAL; ++j) {
            for (int k = 0; k < MAXKNOTINTERVAL; ++k) {
                trimmedPolygonList[i][j][k].clear();
            }
        }
    }
    triangleList.clear();
    faceMtlList.clear();
    faceMtlCountList.clear();
    directPointVector.clear();
    cubicBezierTriangleList.clear();
    quadraticBezierTriangleList.clear();
    freeModelMemD();
    callCudaThreadSynchronize();
    int elapsedTimeLoad = calcTimeLoad.restart();
    cout << "载入\t" << elapsedTimeLoad << "\tCPU : 清理内存" << endl;

    if (reset_ctrl_point) {
        calcKnotCtrlPoint();
        elapsedTimeLoad = calcTimeLoad.restart();
        cout << "载入\t" << elapsedTimeLoad << "\tCPU : 计算节点向量和控制顶点" << endl;
    }

    preCalcD(this);
    callCudaThreadSynchronize();
    elapsedTimeLoad = calcTimeLoad.restart();
    cout << "载入\t" << elapsedTimeLoad << "\tGPU : 复制节点向量和控制顶点到显存" << endl;

    if (algorithm_type_ == PN_NO_CUTTING) {
        triangulatePolygon_PN_NO_CUTTING();
        elapsedTimeLoad = calcTimeLoad.restart();
        cout << "载入\t" << elapsedTimeLoad << "\tCPU : 多边形三角化_PN_NO_CUTTING" << endl;
    }
    else if (algorithm_type_ == LZQ) {
        PN();
        callCudaThreadSynchronize();
        elapsedTimeLoad = calcTimeLoad.restart();
        cout << "载入\t" << elapsedTimeLoad << "\tGPU : 按照PN-Triangle方法计算原始面片的控制顶点" << endl;

        acSplit();
        elapsedTimeLoad = calcTimeLoad.restart();
        cout << "载入\t" << elapsedTimeLoad << "\tCPU : 分割多边形并三角化" << endl;

        preComputerSamplerParamter();
        elapsedTimeLoad = calcTimeLoad.restart();
        cout << "载入\t" << elapsedTimeLoad << "\tCPU : 预计算采样点信息" << endl;
    } else {
        PN();
        callCudaThreadSynchronize();
        elapsedTimeLoad = calcTimeLoad.restart();
        cout << "载入\t" << elapsedTimeLoad << "\tGPU : 按照PN-Triangle方法计算原始面片的控制顶点" << endl;

        clipPolygon();
        elapsedTimeLoad = calcTimeLoad.restart();
        cout << "载入\t" << elapsedTimeLoad << "\tCPU : 分割多边形" << endl;

        triangulatePolygon();
        elapsedTimeLoad = calcTimeLoad.restart();
        cout << "载入\t" << elapsedTimeLoad << "\tCPU : 多边形三角化" << endl;

    }

    loadTriangleListD(triangleList, triangle_adjacent_table_, m_nOrder[0] + m_nOrder[1] + m_nOrder[2] - 3);
    callCudaThreadSynchronize();
    elapsedTimeLoad = calcTimeLoad.restart();
    cout << "载入\t" << elapsedTimeLoad << "\tGPU : 将三角形列表信息载入显存" << endl;
    printCudaError(__FILE__, __FUNCTION__, __LINE__);

    generateUVW(m_nSamplePointCount);
#ifdef TRUTH
    generateUVW_truth(m_nSamplePointCount);
#endif
    callCudaThreadSynchronize();
    elapsedTimeLoad = calcTimeLoad.restart();
    cout << "载入\t" << elapsedTimeLoad << "\tGPU : 生成UVW矩阵" << endl;
    printCudaError(__FILE__, __FUNCTION__, __LINE__);
}

void CommonData::execute() {
    if (m_bGPU) {
#ifndef MORPH
        int elapsedTimeLoad = calcTimeChange.restart();
#endif

        calcSampleValue(algorithm_type_);
#ifndef MORPH
        callCudaThreadSynchronize();
        elapsedTimeLoad = calcTimeChange.restart();
        cout << "载入\t" << elapsedTimeLoad << "\tGPU : 计算 Bezier 曲面片采样点的值" << endl;
#endif

        calcTriangleCtrlPoint(adjust_silhouette_, use_pn_, algorithm_type_);
#ifndef MORPH
        callCudaThreadSynchronize();
        elapsedTimeLoad = calcTimeChange.restart();
        cout << "载入\t" << elapsedTimeLoad << "\tGPU : 计算三角Bezier曲面片控制顶点" << endl;
#endif

        tessellateD(m_bFirstLoad, m_fMax[0], m_fMax[1], m_fMax[2], algorithm_type_);
#ifndef MORPH
        callCudaThreadSynchronize();
        elapsedTimeLoad = calcTimeChange.restart();
        cout << "载入\t" << elapsedTimeLoad << "\tGPU : 计算三角化点的值" << endl;
#endif

#ifdef TRUTH
        /*-------------------- 计算 ground truth 开始 ----------------------*/
        calcSampleValue_truth();
        matrixMul1_truth();
        tessellateD_truth(m_bFirstLoad);
        /*-------------------- 计算 ground truth 结束 ----------------------*/
#endif
    } else { }

    if (m_bFirstLoad)
        m_bFirstLoad = false;
}

/* 重新设置三角化精度后需要进行的处理 */
void CommonData::newSamplePointTesslate() {
    if (m_bLoaded) {
        faceMtlList.clear();
        faceMtlCountList.clear();
        freeTessMemD();

        triangulatePolygon();
        generateUVW(m_nSamplePointCount);
#ifdef TRUTH
        generateUVW_truth(m_nSamplePointCount);
#endif
    }
}

void CommonData::callTesslateD() {
    if (m_bGPU) {
        calcSampleValue(algorithm_type_);
        calcTriangleCtrlPoint(adjust_silhouette_, use_pn_, algorithm_type_);
        tessellateD(m_bFirstLoad, m_fMax[0], m_fMax[1], m_fMax[2], algorithm_type_);

#ifdef TRUTH
        /*-------------------- 计算 ground truth 开始 ----------------------*/
        calcSampleValue_truth();
        matrixMul1_truth();
        tessellateD_truth(m_bFirstLoad);
        /*-------------------- 计算 ground truth 结束 ----------------------*/
#endif
    }
}

// 改进的PN三角形算法，每个角点有两个法向，分别用于调整角点两侧的两个边点
void CommonData::PN() {
    int edge_ctrlpoint_idx[6] = {2, 1, 3, 7, 8, 5};    // 依次移动的边控制顶点的编号
    int corner_ctrlpoint_idx[3] = {0, 6, 9};    // 移动上面的边控制顶点需要的角控制顶点的编号

    // 移动edge_ctrlpoint时，需要用到0, 1, 2三个角控制顶点中的两个
    // (此处的0, 1, 2不是0～9那个编号系统中的0, 1, 2)
    // 第一个角控制顶点分别是0, 0, 1, 1, 2, 2（2, 1, 3, 7, 8, 5对应的角控制顶点）
    // 第二个就是下面这行中的2, 1,		0, 2,		1, 0
    int pair_idx[6] = {2, 1, 0, 2, 1, 0};

    for (vector<Face>::size_type i = 0; i < faceList.size(); ++i) {
        BezierTriangle<3> result_triangle;
        VertexCoord v[3], n[6];            // 三个角点的位置和法向
        for (int j = 0; j < 3; ++j)        // 生成三个角点的位置v[j]
        {
            int v_id = faceList[i].vertexCoordIndex[j];
            result_triangle[corner_ctrlpoint_idx[j]] = v[j] = vertexCoordList[v_id];
        }
        for (int j = 0; j < 6; ++j)        // 生成三个角点上的六个法向n[j]
        {
            int v_id = faceList[i].vertexCoordIndex[j / 2];
            int pair_v_id = faceList[i].vertexCoordIndex[pair_idx[j]];
            EdgeWithNormal e = findEdge(v_id, pair_v_id);
            int n_id = faceList[i].normalCoordIndex[j / 2];

            int other_id = 0;
            if (e.face_id[0] == static_cast<int>(i))//边e的0号相邻面片是当前面片，则1号是另一侧面片
                other_id = 1;
            int n_id_other = -1;
            if (e.v_max == v_id)    // 当前顶点是max顶点
                n_id_other = e.n_max[other_id];
            else                    // 当前顶点是min顶点
                n_id_other = e.n_min[other_id];
            VertexCoord n_cur = objData->normalCoordList[n_id];
            if (e.n_count > 1) {
                VertexCoord n_other = objData->normalCoordList[n_id_other];
                VertexCoord v_pair = v[pair_idx[j]];
                VertexCoord v_mid = 0.5 * (v_pair + v[j / 2]);
                VertexCoord v01 = v_pair - v[j / 2];
                float t0;
                VertexCoord center0;
                bool t0_exist = false;
                if (fabs(n_cur * v01) > ZERO) {
                    t0 = (v_mid - v[j / 2]) * v01 / (n_cur * v01);
                    center0 = v[j / 2] + t0 * n_cur;
                    t0_exist = true;
                }
                float t1;
                VertexCoord center1;
                bool t1_exist = false;
                if (fabs(n_other * v01) > ZERO) {
                    t1 = (v_mid - v[j / 2]) * v01 / (n_other * v01);
                    center1 = v[j / 2] + t1 * n_other;
                    t1_exist = true;
                }

                float t;
                VertexCoord center_mid;
                if (t0_exist && t1_exist) {
                    VertexCoord center_delta = center0 - center1;
                    t = (v[j / 2] - center0) * center_delta / (center_delta * center_delta);
                    center_mid = center0 + t * center_delta;
                    n[j] = v[j / 2] - center_mid;
                }
                else if (t0_exist) {
                    t = (v[j / 2] - center0) * n_other / (n_other * n_other);
                    center_mid = center0 + t * n_other;
                    n[j] = v[j / 2] - center_mid;
                }
                else if (t1_exist) {
                    t = (v[j / 2] - center1) * n_cur / (n_cur * n_cur);
                    center_mid = center1 + t * n_cur;
                    n[j] = v[j / 2] - center_mid;
                }
                else {
                    n[j] = n_cur + n_other;
                }
            }
            else
                n[j] = n_cur;
            n[j].normalize();
        }
        for (int j = 0; j < 6; ++j)        // 六个边点的新位置
        {
            VertexCoord p = v[j / 2] * (2.0 / 3) + v[pair_idx[j]] * (1.0 / 3);
            result_triangle[edge_ctrlpoint_idx[j]] = p - n[j] * ((p - v[j / 2]) * n[j]);
        }
        VertexCoord ave = (result_triangle[1] + result_triangle[2] + result_triangle[3] +
                           result_triangle[7] + result_triangle[5] + result_triangle[8]) * (1.0 / 6);
        result_triangle[4] = ave + (ave - (v[0] + v[1] + v[2]) * (1.0 / 3)) * 0.5;
        cubicBezierTriangleList.push_back(result_triangle);
    }

    // 生成法向二次三角Bezier曲面片
    for (vector<Face>::size_type i = 0; i < faceList.size(); ++i) {
        BezierTriangle<2> result_triangle;

        NormalCoord n0, n1, n2;
        // 生成三个角点
        int n_id = faceList[i].normalCoordIndex[0];
        result_triangle[0] = n0 = objData->normalCoordList[n_id];
        result_triangle[0].normalize();
        n0.normalize();

        n_id = faceList[i].normalCoordIndex[1];
        result_triangle[3] = n1 = objData->normalCoordList[n_id];
        result_triangle[3].normalize();
        n1.normalize();

        n_id = faceList[i].normalCoordIndex[2];
        result_triangle[5] = n2 = objData->normalCoordList[n_id];
        result_triangle[5].normalize();
        n2.normalize();

        // 生成三个边点
        VertexCoord v0, v1, v2;
        int v_id = faceList[i].vertexCoordIndex[0];
        v0 = vertexCoordList[v_id];
        v_id = faceList[i].vertexCoordIndex[1];
        v1 = vertexCoordList[v_id];
        v_id = faceList[i].vertexCoordIndex[2];
        v2 = vertexCoordList[v_id];

        ////////////////////////////////////////

        VertexCoord n01 = n0 + n1;
        VertexCoord n12 = n1 + n2;
        VertexCoord n20 = n2 + n0;

        VertexCoord x = n01;
        x.normalize();

        VertexCoord v01 = v1 - v0;
        v01.normalize();
        result_triangle[1] = n01 - 2 * v01 * (n01 * v01);

        VertexCoord v12 = v2 - v1;
        v12.normalize();
        result_triangle[4] = n12 - 2 * v12 * (n12 * v12);

        VertexCoord v20 = v0 - v2;
        v20.normalize();
        result_triangle[2] = n20 - 2 * v20 * (n20 * v20);

        //result_triangle[1] = n01;
        //result_triangle[4] = n12;
        //result_triangle[2] = n20;

        ////////////////////////////////////////

        //double v_temp_01 = 2 * (v1 - v0) * (n0 + n1) / ((v1 - v0) * (v1 - v0));
        //double v_temp_12 = 2 * (v2 - v1) * (n1 + n2) / ((v2 - v1) * (v2 - v1));
        //double v_temp_20 = 2 * (v0 - v2) * (n2 + n0) / ((v0 - v2) * (v0 - v2));

        //result_triangle[1] = n0 + n1 - v_temp_01 * (v1 - v0);
        //result_triangle[4] = n1 + n2 - v_temp_12 * (v2 - v1);
        //result_triangle[2] = n2 + n0 - v_temp_20 * (v0 - v2);

        result_triangle[1].normalize();
        result_triangle[4].normalize();
        result_triangle[2].normalize();

        quadraticBezierTriangleList.push_back(result_triangle);
    }
}

VertexCoord CommonData::calcBaryCoord(const VertexCoord &v0, const VertexCoord &v1, const VertexCoord &v2,
                                      const VertexCoord &v) {
    // 三个方向向量
    VertexCoord v12 = v2 - v1;
    VertexCoord v20 = v0 - v2;
    VertexCoord v01 = v1 - v0;
    v12.normalize();
    v20.normalize();
    v01.normalize();

    // 计算当前点v到三角形三条边的距离，并通过距离的比值得到重心坐标
    double t0 = v12 * (v - v1);
    double t1 = v20 * (v - v2);
    double t2 = v01 * (v - v0);
    double t_origin0 = v12 * (v0 - v1);
    double t_origin1 = v20 * (v1 - v2);
    double t_origin2 = v01 * (v2 - v0);
    double d0 = (v - (v1 + v12 * t0)).norm();
    double d1 = (v - (v2 + v20 * t1)).norm();
    double d2 = (v - (v0 + v01 * t2)).norm();
    double d_origin0 = (v0 - (v1 + v12 * t_origin0)).norm();
    double d_origin1 = (v1 - (v2 + v20 * t_origin1)).norm();
    double d_origin2 = (v2 - (v0 + v01 * t_origin2)).norm();

    return VertexCoord(d0 / d_origin0, d1 / d_origin1, d2 / d_origin2);
}


/* 计算所有面片被节点盒裁剪的结果 */
void CommonData::clipPolygon() {
    for (int i = m_nOrder[U] - 1; i <= m_nOrder[U] - 1 + m_nKnotIntervalCount[U] - 1; ++i) {
        for (int j = m_nOrder[V] - 1; j <= m_nOrder[V] - 1 + m_nKnotIntervalCount[V] - 1; ++j) {
            for (int k = m_nOrder[W] - 1; k <= m_nOrder[W] - 1 + m_nKnotIntervalCount[W] - 1; ++k) {
                double smallX = knotList[U][i];
                double bigX = knotList[U][i + 1];
                double smallY = knotList[V][j];
                double bigY = knotList[V][j + 1];
                double smallZ = knotList[W][k];
                double bigZ = knotList[W][k + 1];
                //cout << "i = " << i << ", j = " << j << ", k = " << k << ", f = " << faceList.size() << endl;
                for (unsigned int f = 0; f < faceList.size(); ++f) {
                    bool useTexture = false;
                    if (faceList[f].m_eFaceCase == V_T_N)
                        useTexture = true;
                    long mtlIdx = faceList[f].m_nMtlIdx;

                    vector<int> &vertexCoordIndex = faceList[f].vertexCoordIndex;

                    //获得三个顶点
                    VertexCoord v_origin[3]; //表示物体原来的三个顶点
                    int idx = vertexCoordIndex[0];
                    v_origin[0] = vertexCoordList[idx];

                    idx = vertexCoordIndex[1];
                    v_origin[1] = vertexCoordList[idx];

                    idx = vertexCoordIndex[2];
                    v_origin[2] = vertexCoordList[idx];

                    TrimmedPolygon tempPolygon(useTexture, mtlIdx, v_origin, f);
                    tempPolygon.push_back_adjust(false);
                    tempPolygon.push_back_adjust(false);
                    tempPolygon.push_back_adjust(false);
                    for (unsigned int s = 0; s < vertexCoordIndex.size(); ++s) {
                        int idx = vertexCoordIndex[s];
                        tempPolygon.push_back(vertexCoordList[idx]);

                        //找出前一个定点index，如果当前定点index是0,则前一个就是最后一个
                        int prev_id = vertexCoordIndex[vertexCoordIndex.size() - 1];
                        if (s != 0)
                            prev_id = vertexCoordIndex[s - 1];

                        tempPolygon.push_back_n_count(findNormalCount(idx, prev_id));

                        //这边的nornalidx又是f面s顶点的法向index了
                        int normalidx = faceList[f].normalCoordIndex[s];

                        const NormalCoord &temp_n(objData->normalCoordList[normalidx]);
                        tempPolygon.push_back_n(NormalCoord(temp_n.i(), temp_n.j(), temp_n.k()));
                        tempPolygon.push_back_n_adj(NormalCoord(temp_n.i(), temp_n.j(), temp_n.k()));
#ifdef IS_BORDER_INFO
                        tempPolygon.push_back_border(findIsBorder(normalidx, prev_id));
#endif
                    }

                    for (unsigned int s = 0; s < faceList[f].textureCoordIndex.size(); ++s) {
                        int idx = faceList[f].textureCoordIndex[s];
                        tempPolygon.push_back_texture(objData->textureCoordList[idx]);
                    }
                    /* 和平面 x - smallX = 0 求交 */
                    int result = clipPolygonAgainstPlane(tempPolygon, 1, 0, 0, -smallX, true);
                    if (result == 0)
                        continue;

                    /* 和平面 x - bigX = 0 求交 */
                    result = clipPolygonAgainstPlane(tempPolygon, 1, 0, 0, -bigX, false);
                    if (result == 0)
                        continue;

                    /* 和平面 y - smallY = 0 求交 */
                    result = clipPolygonAgainstPlane(tempPolygon, 0, 1, 0, -smallY, true);
                    if (result == 0)
                        continue;

                    /* 和平面 y - bigY = 0 求交 */
                    result = clipPolygonAgainstPlane(tempPolygon, 0, 1, 0, -bigY, false);
                    if (result == 0)
                        continue;

                    /* 和平面 z - smallZ = 0 求交 */
                    result = clipPolygonAgainstPlane(tempPolygon, 0, 0, 1, -smallZ, true);
                    if (result == 0)
                        continue;

                    /* 和平面 z - bigZ = 0 求交 */
                    result = clipPolygonAgainstPlane(tempPolygon, 0, 0, 1, -bigZ, false);
                    if (result == 0)
                        continue;

                    /* 为多边形上的每一点都找到其在原始模型的PN-Triangle中的对应点 */
                    for (int idx = 0; idx < tempPolygon.size(); ++idx) {
                        VertexCoord v = tempPolygon[idx];
                        VertexCoord bary = calcBaryCoord(v_origin[0], v_origin[1], v_origin[2], v);

// adjust_split_points_是总开关，在图形界面上打开和关闭，决定是否调整切割点
// tempPolygon.adjust(idx)决定tempPolygon的idx号面片是否进行调整，切割生成的
// 		顶点都需要调整，继承自初始模型的顶点无需调整，可以节省效率和提高精度
// tempPolygon.border(idx)是判断idx号顶点的前一条边是不是边界边，不过这里的判断
// 		似乎不完整，当前用不到这个功能，因此先搁置此问题
                        //if (adjust_split_points_)
                        if (adjust_split_points_ && tempPolygon.adjust(idx))
                            //if (adjust_split_points_ && tempPolygon.adjust(idx) && (!tempPolygon.border(idx)))
                        {
                            tempPolygon[idx] = cubicBezierTriangleList[f].calcValue(bary);
#define ADJ_NORMAL
#ifdef ADJ_NORMAL
                            tempPolygon.setNormalAdj(idx, quadraticBezierTriangleList[f].calcValue(bary));
#endif
                        }
#ifdef LINE
                        tempPolygon.push_back_bary(bary);
#endif
                        //cout << idx << ", n_count = " << tempPolygon.normalCount(idx) << endl;
                    }

                    //for (int i = 0; i < tempPolygon.size(); ++i)
                    //{
                    //cout << "V:" << tempPolygon[i] << "\t";
                    //cout << "N:" << tempPolygon.getNormal(i) << endl;
                    //}
                    //cout << "=====================" << endl;
                    /* 保存结果 */
                    tempPolygon.normalizeAllNormals();
                    trimmedPolygonList[i - (m_nOrder[U] - 1)][j - (m_nOrder[V] - 1)][k - (m_nOrder[W] - 1)].push_back(
                            tempPolygon);
                }
            }
        }
    }
}

/* 计算一个凸多边形被一个平面裁剪的结果，isPositive表示保留正侧还是负侧 */
int CommonData::clipPolygonAgainstPlane(TrimmedPolygon &polygon, double a, double b, double c, double d,
                                        bool isPositive) {
    int positive = 0;
    int negative = 0;
    int vertexCount = polygon.size();
    int location[50];

    /* 计算每个顶点分布在截平面的哪一侧 */
    for (int i = 0; i < vertexCount; ++i) {
        double x = polygon[i].x();
        double y = polygon[i].y();
        double z = polygon[i].z();
        double distance = a * x + b * y + c * z + d;
        if (distance > ZERO) {
            if (isPositive) {
                location[i] = 1;
                positive++;
            }
            else {
                location[i] = -1;
                negative++;
            }
        }
        else {
            if (distance < -ZERO) {
                if (isPositive) {
                    location[i] = -1;
                    negative++;
                }
                else {
                    location[i] = 1;
                    positive++;
                }
            }
            else {
                location[i] = 0;
            }
        }
    }
    if (negative == 0)        // 全部顶点都需要保留
        return vertexCount;
    if (positive == 0)        // 全部顶点都不需要保留
        return 0;

    //cout << "a = " << a << ", b = " << b << ", c = " << c << ", ZERO = " << ZERO << endl;
    //cout << "nega = " << negative << ", positive = " << positive << endl;

    bool useTexture = polygon.useTexture();
    int mtlIdx = polygon.mtlIdx();
    VertexCoord *v_origin = polygon.v_origin;
    TrimmedPolygon result(useTexture, mtlIdx, v_origin, polygon.origin_face_idx_);
    int previous = vertexCount - 1;
    for (int i = 0; i < vertexCount; ++i) {
        if (location[i] == -1) {
            if (location[previous] == 1)    // 前一点保留，当前点舍弃
            {
                VertexCoord delta = polygon[i] - polygon[previous];
                double t = (a * polygon[i].x() + b * polygon[i].y() + c * polygon[i].z() + d) /
                           (a * delta.x() + b * delta.y() + c * delta.z());
                result.push_back(polygon[i] - delta * t);

                //cout << "cur = " << polygon[i] << "\tprev = " << polygon[previous] << endl;
                //cout << "VV:" << polygon[i] - delta * t << "\t";

                //// （旧算法）新切割出来的点，根据所在边的法向数量决定是否进行第一阶段调整
                //if (polygon.normalCount(i) > 1)
                //result.push_back_adjust(false);
                //else
                //result.push_back_adjust(true);

                // （新算法）新切割出来的点，一定会进行第一阶段调整
                result.push_back_adjust(true);

                // 切割点之前的边位于原三角形一条边上，所以继承它的边界属性
#ifdef IS_BORDER_INFO
                result.push_back_border(polygon.border(i));
#endif

                delta = polygon.getNormal(i) - polygon.getNormal(previous);
                NormalCoord n_result = polygon.getNormal(i) - delta * t;
                //n_result.normalize();
                result.push_back_n(n_result);
                result.push_back_n_adj(n_result);
                //result.push_back_n(polygon.getNormal(i) - delta * t);

                //cout << "NN:" << n_result << endl;

                result.push_back_n_count(polygon.normalCount(i));

                if (useTexture) {
                    TextureCoord currentTexture = polygon.getTexture(i);
                    TextureCoord previousTexture = polygon.getTexture(previous);
                    TextureCoord deltaTexture = currentTexture - previousTexture;
                    result.push_back_texture(currentTexture - deltaTexture * t);
                }
            }
        }
        else        // location[i]不是0就是1
        {
            if ((location[i] == 1) && (location[previous] == -1))    // 前一点舍弃，当前点保留
            {
                VertexCoord delta = polygon[previous] - polygon[i];
                double t = (a * polygon[previous].x() + b * polygon[previous].y() + c * polygon[previous].z() + d) /
                           (a * delta.x() + b * delta.y() + c * delta.z());
                result.push_back(polygon[previous] - delta * t);
                //cout << "cur = " << polygon[i] << ", pre = " << polygon[previous] << "\t";
                //cout << "VV2:" << polygon[previous] - delta * t << "\t";

                // （旧算法）新切割出来的点，根据所在边的法向数量决定是否进行第一阶段调整
                //if (polygon.normalCount(i) > 1)
                //result.push_back_adjust(false);
                //else
                //result.push_back_adjust(true);

                // （新算法）新切割出来的点，一定会进行第一阶段调整
                result.push_back_adjust(true);

                // 切割点之前的边位于原多边形内部，所以一定不是边界
#ifdef IS_BORDER_INFO
                result.push_back_border(false);
#endif

                delta = polygon.getNormal(previous) - polygon.getNormal(i);
                NormalCoord n_result = polygon.getNormal(previous) - delta * t;
                //n_result.normalize();
                result.push_back_n(n_result);
                //cout << "NN2:" << n_result << endl;
                result.push_back_n_adj(n_result);
                result.push_back_n_count(1);

                if (useTexture) {
                    TextureCoord currentTexture = polygon.getTexture(i);
                    TextureCoord previousTexture = polygon.getTexture(previous);
                    TextureCoord deltaTexture = previousTexture - currentTexture;
                    result.push_back_texture(previousTexture - deltaTexture * t);
                }
            }
            result.push_back(polygon[i]);
            result.push_back_adjust(polygon.adjust(i));    // 从上一个切割结果继承的点，继续保留是否调整的判断
#ifdef IS_BORDER_INFO
            result.push_back_border(polygon.border(i)); // 切割点之前的边位于原三角形一条边上，所以继承它的边界属性
#endif
            result.push_back_n(polygon.getNormal(i));
            result.push_back_n_adj(polygon.getNormal(i));
            if ((location[i] == 0) && (location[previous] == -1))    // 当前点恰好在切割平面上
                result.push_back_n_count(1);
            else
                result.push_back_n_count(polygon.normalCount(i));
            if (result.useTexture())
                result.push_back_texture(polygon.getTexture(i));

        }
        previous = i;
    }
    polygon = result;
    return result.size();
}

void CommonData::triangulatePolygon_PN_NO_CUTTING() {
    vector<vector<int> > mtlFaceList(objData->mtlTexList.size() + 1);

    for (vector<Triangle>::size_type f = 0; f < faceList.size(); ++f) {
        int id0 = faceList[f].normalCoordIndex[0];
        int id1 = faceList[f].normalCoordIndex[1];
        int id2 = faceList[f].normalCoordIndex[2];
        const NormalCoord &n0(objData->normalCoordList[id0]);
        const NormalCoord &n1(objData->normalCoordList[id1]);
        const NormalCoord &n2(objData->normalCoordList[id2]);

        id0 = faceList[f].vertexCoordIndex[0];
        id1 = faceList[f].vertexCoordIndex[1];
        id2 = faceList[f].vertexCoordIndex[2];
        VertexCoord v[3];
        v[0] = vertexCoordList[id0];
        v[1] = vertexCoordList[id1];
        v[2] = vertexCoordList[id2];

        int mtlIdx = faceList[f].m_nMtlIdx;
        if (mtlIdx == -1)
            mtlIdx = mtlFaceList.size() - 1;
        int nc0 = findNormalCount(id2, id0);
        int nc1 = findNormalCount(id0, id1);
        int nc2 = findNormalCount(id1, id2);
        if (faceList[f].m_eFaceCase == V_T_N) {
            id0 = faceList[f].textureCoordIndex[0];
            id1 = faceList[f].textureCoordIndex[1];
            id2 = faceList[f].textureCoordIndex[2];
            const TextureCoord &t0 = objData->textureCoordList[id0];
            const TextureCoord &t1 = objData->textureCoordList[id1];
            const TextureCoord &t2 = objData->textureCoordList[id2];
            Triangle t(v[0], v[1], v[2], n0, n1, n2, n0, n1, n2,
#ifdef LINE
                    VertexCoord(1, 0, 0), VertexCoord(0, 1, 0), VertexCoord(0, 0, 1),
#endif
                       v, f,
                       nc0, nc1, nc2,
                       t0, t1, t2);
            triangleList.push_back(t);
        }
        else {
            Triangle t(v[0], v[1], v[2], n0, n1, n2, n0, n1, n2,
#ifdef LINE
                    VertexCoord(1, 0, 0), VertexCoord(0, 1, 0), VertexCoord(0, 0, 1),
#endif
                       v, f,
                       nc0, nc1, nc2);
            triangleList.push_back(t);
        }
        face_children_table_[f].push_front(triangleList.size() - 1);
        mtlFaceList[mtlIdx].push_back(f);
    }

    /* 将此模型中所有的三角形顶点统一编号，所有面片根据材质的不同，存入faceMtlList中 */
    int samplePointPerTriangle = m_nSamplePointCount * (m_nSamplePointCount + 1) / 2;
    m_nTessPointCount = samplePointPerTriangle * triangleList.size();
    int segment = m_nSamplePointCount - 1;
    int subTrianglePerTriangle = segment * segment;

    int totalFaceNum = 0;
    for (vector<vector<int> >::size_type ii = 0; ii < mtlFaceList.size(); ++ii)
        totalFaceNum += mtlFaceList[ii].size();
    int faceCounter = 0;
    for (vector<vector<int> >::size_type ii = 0; ii < mtlFaceList.size(); ++ii) {
        int *tempFaceList = new int[mtlFaceList[ii].size() * subTrianglePerTriangle * 3];
        int pos = 0;
        for (vector<int>::size_type jj = 0; jj < mtlFaceList[ii].size(); ++jj) {
            int idx = mtlFaceList[ii][jj];
            int pointBase = samplePointPerTriangle * idx;
            for (int i = 0; i < segment; ++i) {
                for (int j = 0; j <= i; ++j) {
                    tempFaceList[pos++] = pointBase + triangleCoord(i, j);
                    tempFaceList[pos++] = pointBase + triangleCoord(i + 1, j);
                    tempFaceList[pos++] = pointBase + triangleCoord(i + 1, j + 1);
                    if (i < segment - 1) {
                        tempFaceList[pos++] = pointBase + triangleCoord(i + 1, j);
                        tempFaceList[pos++] = pointBase + triangleCoord(i + 2, j + 1);
                        tempFaceList[pos++] = pointBase + triangleCoord(i + 1, j + 1);
                    }
                }
            }
        }
        faceCounter += mtlFaceList[ii].size();
        faceMtlList.push_back(tempFaceList);
        faceMtlCountList.push_back(mtlFaceList[ii].size() * subTrianglePerTriangle);
    }

    // 对三角化之后的模型重新寻找相邻关系
    // triangle_adjacent_table_中的元素三个一组，[3i+0], [3i+1], [3i+2]
    // 分别代表与i号面片的0号、1号、2号边相邻的三角形信息。0号边是v2v0，1号边是v0v1，2号边是v1v2
    // 每个元素的高30位存储了对方三角形的编号，低两位存储了对方三角形的相邻边编号(可能是0, 1, 2)
    // 初始情况将所有元素都设置成-1234
    triangle_adjacent_table_ = new int[triangleList.size() * 3];
    std::fill(triangle_adjacent_table_, triangle_adjacent_table_ + triangleList.size() * 3, -1234);
    for (vector<Triangle>::size_type i = 0; i < triangleList.size(); ++i) {
        int ori_idx = triangleList[i].origin_face_idx_;
        //cout << "triangle = " << i << ", ori_idx = " << ori_idx << endl;
        // 遍历i号三角形的亲兄弟三角形
        for (list<int>::iterator it = face_children_table_[ori_idx].begin();
             it != face_children_table_[ori_idx].end(); ++it) {
            int result;
            //cout << "\tchild = " << *it << endl;
            if ((result = triangularAdjacent(i, *it)) >= 0) {
                //cout << "\t相邻" << endl;
                int idx0 = result >> 4;
                int idx1 = result & 0xF;
                triangle_adjacent_table_[i * 3 + idx0] = *it << 2 | idx1;
                //cout << "face_idx = " << *it << " " << idx0 << " " << idx1
                //<< " " << i * 3 + idx0 << "=" << triangle_adjacent_table_[i * 3 + idx0] << endl;
            }
        }
        // 遍历i号三角形的叔伯三角形（即和i号三角形的父亲相邻的三角形）
        for (list<int>::iterator it = face_adjacent_table_[ori_idx].begin();
             it != face_adjacent_table_[ori_idx].end(); ++it) {
            //cout << "\t叔伯 = " << *it << endl;
            // 遍历该叔叔所有的儿子三角形（称为i号三角形的堂兄弟三角形）
            for (list<int>::iterator it2 = face_children_table_[*it].begin();
                 it2 != face_children_table_[*it].end(); ++it2) {
                int result;
                //cout << "\t\t叔伯的儿子 = " << *it2 << endl;
                if ((result = triangularAdjacent(i, *it2)) >= 0) {
                    //cout << "\t\t相邻" << endl;
                    int idx0 = result >> 4;
                    int idx1 = result & 0xF;
                    triangle_adjacent_table_[i * 3 + idx0] = *it2 << 2 | idx1;
                    //cout << "face_idx = " << *it2 << " " << idx0 << " " << idx1
                    //<< " " << i * 3 + idx0 << "=" << triangle_adjacent_table_[i * 3 + idx0] << endl;
                }
            }
        }
    }

}

void CommonData::triangulatePolygon() {
    vector<vector<int> > mtlFaceList(objData->mtlTexList.size() + 1);
    int triangleIdx = 0;

    for (int i = 0; i < m_nKnotIntervalCount[U]; ++i) {
        for (int j = 0; j < m_nKnotIntervalCount[V]; ++j) {
            for (int k = 0; k < m_nKnotIntervalCount[W]; ++k) {
                vector<TrimmedPolygon> &TrimmedPolygonList = trimmedPolygonList[i][j][k];
                for (unsigned int p = 0; p < TrimmedPolygonList.size(); ++p) {
                    TrimmedPolygon &trimmedPolygon = TrimmedPolygonList[p];
                    const VertexCoord &v0 = trimmedPolygon[0];
                    const VertexCoord n0 = trimmedPolygon.getNormal(0);
                    const VertexCoord n_adj0 = trimmedPolygon.getNormalAdj(0);
                    //cout << "n0 = " << n0.x() << ", " << n0.y() << ", " << n0.z() << endl;
                    int origin_face_idx = trimmedPolygon.origin_face_idx_;
                    bool useTex = trimmedPolygon.useTexture();
                    TextureCoord vt0;
                    if (useTex)
                        vt0 = trimmedPolygon.getTexture(0);
                    int mtlIdx = trimmedPolygon.mtlIdx();
                    if (mtlIdx == -1) {
                        mtlIdx = (int) (mtlFaceList.size() - 1);
                    }
                    const VertexCoord *v_origin = trimmedPolygon.v_origin;
#ifdef LINE
                    const VertexCoord &bary_origin0 = trimmedPolygonList[i][j][k][p].getBary(0);
#endif
                    int size = trimmedPolygon.size();
                    for (int q = 1; q < size - 1; ++q) {
                        int nc0, nc1, nc2 = trimmedPolygon.normalCount(q + 1);
                        if (size == 3)        // 三角形扇仅有一个三角形，需要单独处理
                        {
                            nc0 = trimmedPolygon.normalCount(0);
                            nc1 = trimmedPolygon.normalCount(1);
                        }
                        else if (q == 1)    // 三角形扇的第一个三角形
                        {
                            nc0 = 1;
                            nc1 = trimmedPolygon.normalCount(1);
                            //cout << "q = 1" << endl;
                        }
                        else if (q == size - 2)        // 三角形扇的最后一个三角形
                        {
                            nc0 = trimmedPolygon.normalCount(0);
                            nc1 = 1;
                            //cout << "q = size - 2" << endl;
                        }
                        else                // 三角形扇的中间三角形
                        {
                            nc0 = 1;
                            nc1 = 1;
                            //cout << "else" << endl;
                        }
                        if (useTex) {
                            Triangle t(v0, trimmedPolygon[q], trimmedPolygon[q + 1],
                                       n0, trimmedPolygon.getNormal(q),
                                       trimmedPolygon.getNormal(q + 1),
                                       n_adj0, trimmedPolygon.getNormalAdj(q),
                                       trimmedPolygon.getNormalAdj(q + 1),
#ifdef LINE
                            bary_origin0, trimmedPolygonList[i][j][k][p].getBary(q),
                            trimmedPolygonList[i][j][k][p].getBary(q + 1),
#endif
                                       v_origin, origin_face_idx,
                                       nc0, nc1, nc2,
                                       vt0, trimmedPolygon.getTexture(q),
                                       trimmedPolygon.getTexture(q + 1));
                            triangleList.push_back(t);
                            //cout << "n1 = " << trimmedPolygonList[i][j][k][p].getNormal(q).x()
                            //<< ", " << trimmedPolygonList[i][j][k][p].getNormal(q).y()
                            //<< ", " << trimmedPolygonList[i][j][k][p].getNormal(q).z() << endl;
                        }
                        else {
                            Triangle t(v0, trimmedPolygon[q], trimmedPolygon[q + 1],

                                       n0, trimmedPolygon.getNormal(q), trimmedPolygon.getNormal(q + 1),

                                       n_adj0, trimmedPolygon.getNormalAdj(q), trimmedPolygon.getNormalAdj(q + 1),
#ifdef LINE
                            bary_origin0, trimmedPolygonList[i][j][k][p].getBary(q),
                            trimmedPolygonList[i][j][k][p].getBary(q + 1),
#endif
                                       v_origin, origin_face_idx,
                                       nc0, nc1, nc2
                            );
                            triangleList.push_back(t);
                            //cout << "v0 = " << v0
                            //<< ", v1 = " << trimmedPolygonList[i][j][k][p][q]
                            //<< ", v2 = " << trimmedPolygonList[i][j][k][p][q + 1] << endl;
                            //cout << "n0 = " << n0
                            //<< ", n1 = " << trimmedPolygonList[i][j][k][p].getNormal(q)
                            //<< ", n2 = " << trimmedPolygonList[i][j][k][p].getNormal(q + 1) << endl;
                            //cout << "n2 = " << trimmedPolygonList[i][j][k][p].getNormal(q).x()
                            //<< ", " << trimmedPolygonList[i][j][k][p].getNormal(q).y()
                            //<< ", " << trimmedPolygonList[i][j][k][p].getNormal(q).z() << endl;
                            //cout << t.v[0] << ", " << t.v[1] << ", " << t.v[2] << endl;
                            //cout << nc0 << ", " << nc1 << ", " << nc2 << endl;
                        }
                        face_children_table_[origin_face_idx].push_front(triangleList.size() - 1);
                        mtlFaceList[mtlIdx].push_back(triangleIdx++);
                    }
                }
            }
        }
    }
    // 打印face_children_table_
    //for (vector<Face>::size_type i = 0; i < faceList.size(); ++i)
    //{
    //cout << "Face " << i << endl;
    //for (list<int>::iterator it = face_children_table_[i].begin();
    //it != face_children_table_[i].end(); ++it)
    //{
    //cout << "\t" << *it << endl;
    //}
    //}

    /* 将此模型中所有的三角形顶点统一编号，所有面片根据材质的不同，存入faceMtlList中 */
    int samplePointPerTriangle = m_nSamplePointCount * (m_nSamplePointCount + 1) / 2;
    m_nTessPointCount = (int) (samplePointPerTriangle * triangleList.size());
    int segment = m_nSamplePointCount - 1;
    int subTrianglePerTriangle = segment * segment;

    int totalFaceNum = 0;
    for (auto ii = 0; ii < mtlFaceList.size(); ++ii)
        totalFaceNum += mtlFaceList[ii].size();

    int faceCounter = 0;
    for (vector<vector<int> >::size_type ii = 0; ii < mtlFaceList.size(); ++ii) {
        int *tempFaceList = new int[mtlFaceList[ii].size() * subTrianglePerTriangle * 3];
        int pos = 0;
        for (vector<int>::size_type jj = 0; jj < mtlFaceList[ii].size(); ++jj) {
            int idx = mtlFaceList[ii][jj];
            int pointBase = samplePointPerTriangle * idx;
            for (int i = 0; i < segment; ++i) {
                for (int j = 0; j <= i; ++j) {
                    tempFaceList[pos++] = pointBase + triangleCoord(i, j);
                    tempFaceList[pos++] = pointBase + triangleCoord(i + 1, j);
                    tempFaceList[pos++] = pointBase + triangleCoord(i + 1, j + 1);
                    if (i < segment - 1) {
                        tempFaceList[pos++] = pointBase + triangleCoord(i + 1, j);
                        tempFaceList[pos++] = pointBase + triangleCoord(i + 2, j + 1);
                        tempFaceList[pos++] = pointBase + triangleCoord(i + 1, j + 1);
                    }
                }
            }
        }
        faceCounter += mtlFaceList[ii].size();
        faceMtlList.push_back(tempFaceList);
        faceMtlCountList.push_back(mtlFaceList[ii].size() * subTrianglePerTriangle);
    }


    // 对三角化之后的模型重新寻找相邻关系
    // triangle_adjacent_table_中的元素三个一组，[3i+0], [3i+1], [3i+2]
    // 分别代表与i号面片的0号、1号、2号边相邻的三角形信息。0号边是v2v0，1号边是v0v1，2号边是v1v2
    // 每个元素的高30位存储了对方三角形的编号，低两位存储了对方三角形的相邻边编号(可能是0, 1, 2)
    // 初始情况将所有元素都设置成-1234
    triangle_adjacent_table_ = new int[triangleList.size() * 3];
    std::fill(triangle_adjacent_table_, triangle_adjacent_table_ + triangleList.size() * 3, -1234);
    for (vector<Triangle>::size_type i = 0; i < triangleList.size(); ++i) {
        //cout << "triangle = " << i << endl;
        int ori_idx = triangleList[i].origin_face_idx_;
        // 遍历i号三角形的亲兄弟三角形
        for (list<int>::iterator it = face_children_table_[ori_idx].begin();
             it != face_children_table_[ori_idx].end(); ++it) {
            int result;
            //cout << "\tchild = " << *it << endl;
            if ((result = triangularAdjacent(i, *it)) >= 0) {
                //cout << "\t相邻" << endl;
                int idx0 = result >> 4;
                int idx1 = result & 0xF;
                triangle_adjacent_table_[i * 3 + idx0] = *it << 2 | idx1;
                //cout << "face_idx = " << *it << " " << idx0 << " " << idx1
                //<< " " << i * 3 + idx0 << "=" << triangle_adjacent_table_[i * 3 + idx0] << endl;
            }
        }
        // 遍历i号三角形的叔伯三角形（即和i号三角形的父亲相邻的三角形）
        for (list<int>::iterator it = face_adjacent_table_[ori_idx].begin();
             it != face_adjacent_table_[ori_idx].end(); ++it) {
            //cout << "\t叔伯 = " << *it << endl;
            // 遍历该叔叔所有的儿子三角形（称为i号三角形的堂兄弟三角形）
            for (list<int>::iterator it2 = face_children_table_[*it].begin();
                 it2 != face_children_table_[*it].end(); ++it2) {
                int result;
                //cout << "\t\t叔伯的儿子 = " << *it2 << endl;
                if ((result = triangularAdjacent(i, *it2)) >= 0) {
                    //cout << "\t\t相邻" << endl;
                    int idx0 = result >> 4;
                    int idx1 = result & 0xF;
                    triangle_adjacent_table_[i * 3 + idx0] = *it2 << 2 | idx1;
                    //cout << "face_idx = " << *it2 << " " << idx0 << " " << idx1
                    //<< " " << i * 3 + idx0 << "=" << triangle_adjacent_table_[i * 3 + idx0] << endl;
                }
            }
        }
    }

}

int CommonData::triangularAdjacent(int idx0, int idx1) {
    const double limit = 0.000001;
    if (idx0 == idx1)        // 自己和自己不算相邻
        return -1;
    Triangle &t0 = triangleList[idx0];
    Triangle &t1 = triangleList[idx1];
    for (int i = 0; i < 3; ++i) {
        int i_pre = (i == 0 ? 2 : i - 1);
        VertexCoord &t0v0 = t0.v[i];
        VertexCoord &t0v1 = t0.v[i_pre];
        //cout << "t0v0 = " << t0v0 << ", t0v1 = " << t0v1 << endl;
        for (int j = 0; j < 3; ++j) {
            int j_pre = (j == 0 ? 2 : j - 1);
            VertexCoord &t1v0 = t1.v[j];
            VertexCoord &t1v1 = t1.v[j_pre];
            //cout << "\tt1v0 = " << t1v0 << ", t1v1 = " << t1v1 << endl;
            if (((t0v0 - t1v0).norm() < limit && (t0v1 - t1v1).norm() < limit) ||
                ((t0v0 - t1v1).norm() < limit && (t0v1 - t1v0).norm() < limit)) {
                //cout << "i << 4 | j = " << (i << 4 | j) << endl;
                return i << 4 | j;
            }
        }
    }
    return -1;
}

const EdgeWithNormal &CommonData::findEdge(int v0, int v1) {
    int v_min, v_max;
    if (v0 < v1) {
        v_min = v0;
        v_max = v1;
    }
    else {
        v_min = v1;
        v_max = v0;
    }
    for (list<EdgeWithNormal>::iterator it = edge_table_[v_min].begin(); it != edge_table_[v_min].end(); ++it)
        if (it->v_max == v_max)
            return *it;

    return edge_illegal_;
}

int CommonData::findNormalCount(int v0, int v1) {
    EdgeWithNormal e = findEdge(v0, v1);
    if (e.v_max < 0)
        return -1234;
    return e.n_count;
}

#ifdef IS_BORDER_INFO

int CommonData::findIsBorder(int v0, int v1) {
    EdgeWithNormal e = findEdge(v0, v1);
    return e.border;
}

#endif

// stable functions
/*----------------------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------------------*/
/*----------------------------   下列函数几乎不需要发生变动   ----------------------------*/
/*----------------------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------------------*/

void CommonData::increaseCenterCtrlPointFactor() {
    extern float center_factor;
    center_factor += 0.1;
    calcFinalResult();
}

void CommonData::decreaseCenterCtrlPointFactor() {
    extern float center_factor;
    center_factor -= 0.1;
    calcFinalResult();
}

void CommonData::setUsePN() {
    if (use_pn_)
        use_pn_ = false;
    else
        use_pn_ = true;
    calcFinalResult();
}

void CommonData::testRotate(bool clockwise) {
    int max_x_id = m_nCtrlPointCount[U];
    int max_y_id = m_nCtrlPointCount[V];
    int max_z_id = m_nCtrlPointCount[W];
    int y_id = max_y_id / 2;

    double degree = -90.0;
    if (!clockwise)
        degree = -degree;
    double theta = degree * PI / 180;

    for (int i = 0; i < max_x_id; ++i) {
        for (int k = 0; k < max_z_id; ++k) {
            double x = ctrlPoint[i][y_id][k].x();
            double z = ctrlPoint[i][y_id][k].z();
            double x_ = x * cos(theta) + z * sin(theta);
            double z_ = -x * sin(theta) + z * cos(theta);
            ctrlPoint[i][y_id][k].x(x_);
            ctrlPoint[i][y_id][k].z(z_);
        }
    }
    calcFinalResult();
}

void CommonData::testMove(bool out) {
    int max_x_id = m_nCtrlPointCount[U] - 1;
    int max_y_id = m_nCtrlPointCount[V] - 1;
    int max_z_id = m_nCtrlPointCount[W] - 1;
    float dist = 0.5;
    if (!out)
        dist = -0.5f;
    ctrlPoint[0][0][0] += VertexCoord(-dist, -dist, -dist);                        // 000
    ctrlPoint[0][0][max_z_id] += VertexCoord(-dist, -dist, dist);                // 001
    ctrlPoint[max_x_id][0][max_z_id] += VertexCoord(dist, -dist, dist);            // 101
    ctrlPoint[max_x_id][0][0] += VertexCoord(dist, -dist, -dist);                // 100
    ctrlPoint[0][max_y_id][0] += VertexCoord(-dist, dist, -dist);                // 010
    ctrlPoint[0][max_y_id][max_z_id] += VertexCoord(-dist, dist, dist);            // 011
    ctrlPoint[max_x_id][max_y_id][0] += VertexCoord(dist, dist, -dist);            // 110
    ctrlPoint[max_x_id][max_y_id][max_z_id] += VertexCoord(dist, dist, dist);    // 111
    calcFinalResult();
}

void CommonData::testMoveTorus(bool out) {
    int x_id = m_nCtrlPointCount[U] / 2;
    int y_id = m_nCtrlPointCount[V] / 2;
    int z_id = m_nCtrlPointCount[W] / 2;
    int max_x_id = m_nCtrlPointCount[U] - 1;
    int max_y_id = m_nCtrlPointCount[V] - 1;
    int max_z_id = m_nCtrlPointCount[W] - 1;
    float dist = 0.5;
    if (!out)
        dist = -0.5f;
    // 前方
    ctrlPoint[x_id][max_y_id][max_z_id] += VertexCoord(dist, dist, dist);    // 上
    ctrlPoint[x_id][0][max_z_id] += VertexCoord(-dist, -dist, dist);        // 下
    ctrlPoint[0][y_id][max_z_id] += VertexCoord(-dist, dist, dist);            // 左
    ctrlPoint[max_x_id][y_id][max_z_id] += VertexCoord(dist, -dist, dist);    // 右
    // 中间
    ctrlPoint[0][max_y_id][max_z_id / 2] += VertexCoord(-dist, dist, 0);        // 左上
    ctrlPoint[max_x_id][max_y_id][max_z_id / 2] += VertexCoord(dist, dist, 0);    // 右上
    ctrlPoint[0][0][max_z_id / 2] += VertexCoord(-dist, -dist, 0);                // 左下
    ctrlPoint[max_x_id][0][max_z_id / 2] += VertexCoord(dist, -dist, 0);        // 右下
    // 后方
    ctrlPoint[x_id][max_y_id][0] += VertexCoord(-dist, dist, -dist);        // 上
    ctrlPoint[x_id][0][0] += VertexCoord(dist, -dist, -dist);                // 下
    ctrlPoint[0][y_id][0] += VertexCoord(-dist, -dist, -dist);                // 左
    ctrlPoint[max_x_id][y_id][0] += VertexCoord(dist, dist, -dist);            // 右
    // 六个中心点
    ctrlPoint[0][y_id][z_id] += VertexCoord(-dist * 1.5, 0, 0);
    ctrlPoint[max_x_id][y_id][z_id] += VertexCoord(dist * 1.5, 0, 0);
    ctrlPoint[x_id][0][z_id] += VertexCoord(0, -dist * 1.5, 0);
    ctrlPoint[x_id][max_y_id][z_id] += VertexCoord(0, dist * 1.5, 0);
    ctrlPoint[x_id][y_id][0] += VertexCoord(0, 0, -dist * 1.5);
    ctrlPoint[x_id][y_id][max_z_id] += VertexCoord(0, 0, dist * 1.5);

    calcFinalResult();
}

/* 将相应控制顶点在选中和未选中状态之间切换 */
void CommonData::setCtrlPointSelected(int i, int j, int k) {
    ctrlPoint[i][j][k].setSelect(!ctrlPoint[i][j][k].selected());
}

void CommonData::cancelAllSelection() {
    int max_x_id = m_nCtrlPointCount[U];
    int max_y_id = m_nCtrlPointCount[V];
    int max_z_id = m_nCtrlPointCount[W];
    for (int i = 0; i < max_x_id; ++i) {
        for (int j = 0; j < max_y_id; ++j) {
            for (int k = 0; k < max_z_id; ++k) {
                ctrlPoint[i][j][k].setSelect(false);
            }
        }
    }
}

void CommonData::directPointVectorPush(DirectPoint &p) {
    int knotCount[3];
    for (int i = 0; i < 3; ++i)
        knotCount[i] = m_nCtrlPointCount[i] + m_nOrder[i];

    /* 由于计算存在数值误差，所以结果点的参数有可能超出参数节点区间，需要调整 */
    if (p.u() > knotList[U][knotCount[U] - 1])
        p.setU(knotList[U][knotCount[U] - 1]);
    else if (p.u() < knotList[U][0])
        p.setU(knotList[U][0]);

    if (p.v() > knotList[V][knotCount[V] - 1])
        p.setV(knotList[V][knotCount[V] - 1]);
    else if (p.v() < knotList[V][0])
        p.setV(knotList[V][0]);

    if (p.w() > knotList[W][knotCount[W] - 1])
        p.setW(knotList[W][knotCount[W] - 1]);
    else if (p.w() < knotList[W][0])
        p.setW(knotList[W][0]);

    directPointVector.push_back(p);
}

void CommonData::setDirectPointSelected(int idx) {
    directPointVector[idx].setSelect(!directPointVector[idx].selected());
}

int CommonData::directPointSelectedCount() {
    int count = 0;
    for (vector<DirectPoint>::size_type i = 0; i != directPointVector.size(); ++i) {
        if (directPointVector[i].selected())
            count++;
    }
    return count;
}

double *matrixCaseHost(double *matrix_b_spline, int order, int ctrlPointNum, int leftIdx);

extern double matrix_b_spline[185];

/*
 * 使用矩阵乘法计算 B 样条体值
 * 仅用于 FFD 算法和计算未选中的直接编辑点的新位置
 */
VertexCoord CommonData::BSplineVolumeValueMatrix(double u, double v, double w,
                                                 int leftUIdx, int leftVIdx, int leftWIdx) {
    VertexCoord result;
    VertexCoord tempCtrlPoint1[4];
    VertexCoord tempCtrlPoint2[4][4];

    double *M = 0, temp[4], mul1[4];

    u = (u - knotList[U][leftUIdx]) / (knotList[U][leftUIdx + 1] - knotList[U][leftUIdx]);
    v = (v - knotList[V][leftVIdx]) / (knotList[V][leftVIdx + 1] - knotList[V][leftVIdx]);
    w = (w - knotList[W][leftWIdx]) / (knotList[W][leftWIdx + 1] - knotList[W][leftWIdx]);

    // 由三维控制顶点算出二维临时控制顶点
    temp[0] = 1.0;
    temp[1] = w;
    temp[2] = w * w;
    temp[3] = temp[2] * w;

    M = matrixCaseHost(matrix_b_spline, m_nOrder[W], m_nCtrlPointCount[W], leftWIdx);

    for (int i = 0; i < m_nOrder[W]; ++i) {
        mul1[i] = 0.0;
        for (int j = 0; j < m_nOrder[W]; ++j) {
            mul1[i] += temp[j] * M[j * m_nOrder[W] + i];
        }
    }
    for (int i = 0; i < m_nOrder[U]; ++i) {
        for (int j = 0; j < m_nOrder[V]; ++j) {
            tempCtrlPoint2[i][j] = VertexCoord(0.0, 0.0, 0.0);
            for (int k = 0; k < m_nOrder[W]; ++k) {
                tempCtrlPoint2[i][j] += ctrlPoint[leftUIdx - i][leftVIdx - j][leftWIdx - k] * mul1[m_nOrder[W] - 1 - k];
            }
        }
    }

    // 由二维临时控制顶点算出一维临时控制顶点
    temp[1] = v;
    temp[2] = v * v;
    temp[3] = temp[2] * v;

    M = matrixCaseHost(matrix_b_spline, m_nOrder[V], m_nCtrlPointCount[V], leftVIdx);

    for (int i = 0; i < m_nOrder[V]; ++i) {
        mul1[i] = 0.0;
        for (int j = 0; j < m_nOrder[V]; ++j) {
            mul1[i] += temp[j] * M[j * m_nOrder[V] + i];
        }
    }
    for (int i = 0; i < m_nOrder[U]; ++i) {
        tempCtrlPoint1[i] = VertexCoord(0.0, 0.0, 0.0);
        for (int j = 0; j < m_nOrder[V]; ++j) {
            tempCtrlPoint1[i] += tempCtrlPoint2[i][j] * mul1[m_nOrder[V] - 1 - j];
        }
    }

    // 由一维临时控制顶点算出结果
    temp[1] = u;
    temp[2] = u * u;
    temp[3] = temp[2] * u;

    M = matrixCaseHost(matrix_b_spline, m_nOrder[U], m_nCtrlPointCount[U], leftUIdx);

    for (int i = 0; i < m_nOrder[U]; ++i) {
        mul1[i] = 0.0;
        for (int j = 0; j < m_nOrder[U]; ++j) {
            mul1[i] += temp[j] * M[j * m_nOrder[U] + i];
        }
    }
    result = VertexCoord(0.0, 0.0, 0.0);
    for (int i = 0; i < m_nOrder[U]; ++i) {
        result += tempCtrlPoint1[i] * mul1[m_nOrder[U] - 1 - i];
    }

    return result;
}

/*
 * 由参数坐标(u, v, w)计算出对应的的几何坐标
 * 仅用于 FFD 算法和计算未选中的直接编辑点的新位置
 */
VertexCoord CommonData::fromParamToCoord(double u, double v, double w) {
    /* 预先将其值设为最大，将末端点归入最后一段 */
    int leftUIdx, leftVIdx, leftWIdx;
    leftUIdx = m_nOrder[U] - 1 + m_nKnotIntervalCount[U] - 1;
    leftVIdx = m_nOrder[V] - 1 + m_nKnotIntervalCount[V] - 1;
    leftWIdx = m_nOrder[W] - 1 + m_nKnotIntervalCount[W] - 1;

    /* 沿 U 方向查找当前点所在的节点区间 */
    for (int i = m_nOrder[U] - 1; i <= m_nOrder[U] - 1 + m_nKnotIntervalCount[U] - 1; ++i) {
        if (u >= knotList[U][i] && u < knotList[U][i + 1]) {
            leftUIdx = i;
            break;
        }
    }
    /* 沿 V 方向查找当前点所在的节点区间 */
    for (int j = m_nOrder[V] - 1; j <= m_nOrder[V] - 1 + m_nKnotIntervalCount[V] - 1; ++j) {
        if (v >= knotList[V][j] && v < knotList[V][j + 1]) {
            leftVIdx = j;
            break;
        }
    }
    /* 沿 W 方向查找当前点所在的节点区间 */
    for (int k = m_nOrder[W] - 1; k <= m_nOrder[W] - 1 + m_nKnotIntervalCount[W] - 1; ++k) {
        if (w >= knotList[W][k] && w < knotList[W][k + 1]) {
            leftWIdx = k;
            break;
        }
    }
    return BSplineVolumeValueMatrix(u, v, w, leftUIdx, leftVIdx, leftWIdx);
}

void CommonData::setAllCtrlPoint(vector<CtrlPoint> &result) {
    int idx = 0;
    for (int i = 0; i < m_nCtrlPointCount[U]; ++i)
        for (int j = 0; j < m_nCtrlPointCount[V]; ++j)
            for (int k = 0; k < m_nCtrlPointCount[W]; ++k)
                ctrlPoint[i][j][k] = result[idx++];
    calcFinalResult();
}

void CommonData::setTargetCtrlPoint(vector<vector<CtrlPoint> > &result, vector<int> &timeList,
                                    vector<matrix_stack::Matrix4x4> &cubemap_matrix_list) {
    animationStep = result.size();
    for (int step = 0; step < animationStep; ++step) {
        int idx = 0;
        for (int i = 0; i < m_nCtrlPointCount[U]; ++i)
            for (int j = 0; j < m_nCtrlPointCount[V]; ++j)
                for (int k = 0; k < m_nCtrlPointCount[W]; ++k)
                    animationList[step][i][j][k] = result[step][idx++];
    }
    morphStep = 0;
    sourceIdx = 0;
    targetIdx = 1;
    for (int step = 0; step < animationStep; ++step)
        animationTimeList[step] = timeList[step];
    cubemap_matrix_list_ = cubemap_matrix_list;
}

void CommonData::morph() {
    //static int ccc = 0;
    calcTimeChange.start();
    //int max = 2;
    int max = animationTimeList[sourceIdx];
    //cout << "max = " << max << endl;
    //cout << "sourceIdx = " << sourceIdx << endl;
    //cout << ccc++ << ": max = " << max << endl;

#define METHOD1
#ifdef METHOD1
    // 变形方法1（根据一个预定义的变形序列进行变形）
    double rate1 = (double) morphStep / max;
    double rate0 = 1.0 - rate1;

    for (int i = 0; i < m_nCtrlPointCount[U]; ++i)
        for (int j = 0; j < m_nCtrlPointCount[V]; ++j)
            for (int k = 0; k < m_nCtrlPointCount[W]; ++k) {
                double x =
                        animationList[sourceIdx][i][j][k].x() * rate0 + animationList[targetIdx][i][j][k].x() * rate1;
                double y =
                        animationList[sourceIdx][i][j][k].y() * rate0 + animationList[targetIdx][i][j][k].y() * rate1;
                double z =
                        animationList[sourceIdx][i][j][k].z() * rate0 + animationList[targetIdx][i][j][k].z() * rate1;
                bool selected = ctrlPoint[i][j][k].selected();
                ctrlPoint[i][j][k] = CtrlPoint(x, y, z, selected);
            }
    if (morphStep++ == max) {
        morphStep = 0;
        // 一般情况下使用
        if (++sourceIdx == animationStep)
            sourceIdx = 0;
        if (++targetIdx == animationStep)
            targetIdx = 0;

        // 专用于bishop模型
        //if (++sourceIdx == animationStep)
        //sourceIdx = 10;
        //if (++targetIdx == animationStep)
        //targetIdx = 10;
    }
#else		// METHOD1
    // 变形方法2（鱼游动）
    double r = 20.0;

    for (int i = 0; i < m_nCtrlPointCount[U]; ++i)
        for (int j = 0; j < m_nCtrlPointCount[V]; ++j)
            for (int k = 0; k < m_nCtrlPointCount[W]; ++k)
            {
                double theta = (2 * PI * morphStep) / max - PI / 2 * i / 8;
                double rou = r + sin(theta * 10) + j * 2;
                bool selected = ctrlPoint[i][j][k].selected();
                double x = rou * cos(theta);
                double y = rou * sin(theta);
                double z = sourceCtrlPoint[i][j][k].z();
                ctrlPoint[i][j][k] = CtrlPoint(x, y, z, selected);
            }
    if (morphStep++ == max)
        morphStep = 0;
#endif		// METHOD1
    calcFinalResult();
}

/* 移动控制顶点 */
void CommonData::ctrlPointTranslate(double delta, int editDirection) {
    calcTimeChange.start();
    for (int i = 0; i < m_nCtrlPointCount[U]; ++i)
        for (int j = 0; j < m_nCtrlPointCount[V]; ++j)
            for (int k = 0; k < m_nCtrlPointCount[W]; ++k) {
                bool selected = ctrlPoint[i][j][k].selected();
                if (selected)            // 只移动选中的控制顶点
                {
                    double x = ctrlPoint[i][j][k].x();
                    double y = ctrlPoint[i][j][k].y();
                    double z = ctrlPoint[i][j][k].z();
                    if ((editDirection & X_AXIS) != 0)
                        x += delta;
                    if ((editDirection & Y_AXIS) != 0)
                        y += delta;
                    if ((editDirection & Z_AXIS) != 0)
                        z += delta;
                    ctrlPoint[i][j][k] = CtrlPoint(x, y, z, selected);
                }
            }
    calcFinalResult();
}

/* 旋转控制顶点 */
void CommonData::ctrlPointRotate(double theta, int editDirection) {
    calcTimeChange.start();
    for (int i = 0; i < m_nCtrlPointCount[U]; ++i)
        for (int j = 0; j < m_nCtrlPointCount[V]; ++j)
            for (int k = 0; k < m_nCtrlPointCount[W]; ++k) {
                bool selected = ctrlPoint[i][j][k].selected();
                if (selected) {
                    double x = ctrlPoint[i][j][k].x();
                    double y = ctrlPoint[i][j][k].y();
                    double z = ctrlPoint[i][j][k].z();
                    double resultX, resultY, resultZ;
                    if (editDirection == X_AXIS) {
                        resultX = x;
                        resultY = y * cos(theta) - z * sin(theta);
                        resultZ = y * sin(theta) + z * cos(theta);
                    }
                    else if (editDirection == Y_AXIS) {
                        resultX = z * sin(theta) + x * cos(theta);
                        resultY = y;
                        resultZ = z * cos(theta) - x * sin(theta);
                    }
                    else {
                        resultX = x * cos(theta) - y * sin(theta);
                        resultY = x * sin(theta) + y * cos(theta);
                        resultZ = z;
                    }
                    ctrlPoint[i][j][k] = CtrlPoint(resultX, resultY, resultZ, selected);
                }
            }
    calcFinalResult();
}

/* 放缩控制顶点 */
void CommonData::ctrlPointScale(double factor, int editDirection) {
    calcTimeChange.start();
    for (int i = 0; i < m_nCtrlPointCount[U]; ++i)
        for (int j = 0; j < m_nCtrlPointCount[V]; ++j)
            for (int k = 0; k < m_nCtrlPointCount[W]; ++k) {
                bool selected = ctrlPoint[i][j][k].selected();
                if (selected) {
                    double x = ctrlPoint[i][j][k].x();
                    double y = ctrlPoint[i][j][k].y();
                    double z = ctrlPoint[i][j][k].z();
                    if ((editDirection & X_AXIS) != 0)
                        x *= exp(factor);
                    if ((editDirection & Y_AXIS) != 0)
                        y *= exp(factor);
                    if ((editDirection & Z_AXIS) != 0)
                        z *= exp(factor);
                    ctrlPoint[i][j][k] = CtrlPoint(x, y, z, selected);
                }
            }
    calcFinalResult();
}

void CommonData::directLinearSolve(const LaGenMatDouble &D) {
    double arrayR[MAXCTRLPOINT * MAXCTRLPOINT * MAXCTRLPOINT * MAXDIRECTPOINT];
    int rowCountR = m_nCtrlPointCount[U] * m_nCtrlPointCount[V] * m_nCtrlPointCount[W];
    vector<DirectPoint>::size_type selectedIdx = 0;
    for (vector<DirectPoint>::size_type idx = 0; idx < directPointVector.size(); ++idx) {
        if (!directPointVector[idx].selected())
            continue;
        double u = directPointVector[idx].u();
        double v = directPointVector[idx].v();
        double w = directPointVector[idx].w();
        int row = 0;
        double Nu[MAXCTRLPOINT], Nv[MAXCTRLPOINT], Nw[MAXCTRLPOINT];
        for (int i = 0; i < m_nCtrlPointCount[U]; ++i)
            Nu[i] = BSplineBase(i, m_nOrder[U], u, U);
        for (int j = 0; j < m_nCtrlPointCount[V]; ++j)
            Nv[j] = BSplineBase(j, m_nOrder[V], v, V);
        for (int k = 0; k < m_nCtrlPointCount[W]; ++k)
            Nw[k] = BSplineBase(k, m_nOrder[W], w, W);

        for (int i = 0; i < m_nCtrlPointCount[U]; ++i)
            for (int j = 0; j < m_nCtrlPointCount[V]; ++j)
                for (int k = 0; k < m_nCtrlPointCount[W]; ++k)
                    arrayR[selectedIdx * rowCountR + row++] = Nu[i] * Nv[j] * Nw[k];
        selectedIdx++;
    }
    LaGenMatDouble R(arrayR, rowCountR, directPointSelectedCount());

    double arrayRTR[MAXDIRECTPOINT * MAXDIRECTPOINT] = {0.0};
    LaGenMatDouble RTR(arrayRTR, directPointSelectedCount(), directPointSelectedCount());
    Blas_Mat_Trans_Mat_Mult(R, R, RTR);

    LaGenMatDouble M(directPointSelectedCount(), 3);
    LaLinearSolve(RTR, M, D);

    //cout << "M:\n" << M << endl;

    double arrayd[MAXCTRLPOINT * MAXCTRLPOINT * MAXCTRLPOINT * 3] = {0.0f};
    LaGenMatDouble d(arrayd, rowCountR, 3);
    Blas_Mat_Mat_Mult(R, M, d);
    //cout << "ZERO:\n" << ZERO << endl;

    int idx = 0;
    for (int i = 0; i < m_nCtrlPointCount[U]; ++i)
        for (int j = 0; j < m_nCtrlPointCount[V]; ++j)
            for (int k = 0; k < m_nCtrlPointCount[W]; ++k) {
                ctrlPoint[i][j][k] += VertexCoord(arrayd[idx],
                                                  arrayd[idx + rowCountR],
                                                  arrayd[idx + rowCountR * 2]);
                idx++;
            }
}

void CommonData::directPointTranslate(double delta, int editDirection) {
    calcTimeChange.start();

    VertexCoord T_S;
    if ((editDirection & X_AXIS) != 0)
        T_S.x(delta);
    if ((editDirection & Y_AXIS) != 0)
        T_S.y(delta);
    if ((editDirection & Z_AXIS) != 0)
        T_S.z(delta);
    double arrayD[MAXDIRECTPOINT * 3];
    vector<DirectPoint>::size_type selectedIdx = 0;
    for (vector<DirectPoint>::size_type idx = 0; idx != directPointVector.size(); ++idx) {
        if (!directPointVector[idx].selected())
            continue;
        directPointVector[idx] += T_S;
        arrayD[selectedIdx] = T_S.x();
        arrayD[selectedIdx + directPointSelectedCount()] = T_S.y();
        arrayD[selectedIdx + directPointSelectedCount() * 2] = T_S.z();
        selectedIdx++;
    }
    LaGenMatDouble D(arrayD, directPointSelectedCount(), 3);

    directLinearSolve(D);
    calcFinalResult();
}

void CommonData::directPointRotate(double theta, int editDirection) {
    calcTimeChange.start();

    double arrayD[MAXDIRECTPOINT * 3];
    vector<DirectPoint>::size_type selectedIdx = 0;
    for (vector<DirectPoint>::size_type idx = 0; idx != directPointVector.size(); ++idx) {
        if (!directPointVector[idx].selected())
            continue;
        double x = directPointVector[idx].x();
        double y = directPointVector[idx].y();
        double z = directPointVector[idx].z();
        double resultX, resultY, resultZ;
        if (editDirection == X_AXIS) {
            resultX = x;
            resultY = y * cos(theta) - z * sin(theta);
            resultZ = y * sin(theta) + z * cos(theta);
        }
        else if (editDirection == Y_AXIS) {
            resultX = z * sin(theta) + x * cos(theta);
            resultY = y;
            resultZ = z * cos(theta) - x * sin(theta);
        }
        else {
            resultX = x * cos(theta) - y * sin(theta);
            resultY = x * sin(theta) + y * cos(theta);
            resultZ = z;
        }
        VertexCoord del(resultX - x, resultY - y, resultZ - z);
        directPointVector[idx] += del;
        arrayD[selectedIdx] = del.x();
        arrayD[selectedIdx + directPointSelectedCount()] = del.y();
        arrayD[selectedIdx + directPointSelectedCount() * 2] = del.z();
        selectedIdx++;
    }
    LaGenMatDouble D(arrayD, directPointSelectedCount(), 3);

    directLinearSolve(D);
    calcFinalResult();
}

void CommonData::directPointScale(double factor, int editDirection) {
    calcTimeChange.start();

    double arrayD[MAXDIRECTPOINT * 3];
    vector<DirectPoint>::size_type selectedIdx = 0;
    for (vector<DirectPoint>::size_type idx = 0; idx != directPointVector.size(); ++idx) {
        if (!directPointVector[idx].selected())
            continue;
        double x = directPointVector[idx].x();
        double y = directPointVector[idx].y();
        double z = directPointVector[idx].z();
        double resultX = x, resultY = y, resultZ = z;
        if ((editDirection & X_AXIS) != 0)
            resultX = x * exp(factor);
        if ((editDirection & Y_AXIS) != 0)
            resultY = y * exp(factor);
        if ((editDirection & Z_AXIS) != 0)
            resultZ = z * exp(factor);
        VertexCoord del(resultX - x, resultY - y, resultZ - z);
        directPointVector[idx] += del;
        arrayD[selectedIdx] = del.x();
        arrayD[selectedIdx + directPointSelectedCount()] = del.y();
        arrayD[selectedIdx + directPointSelectedCount() * 2] = del.z();
        selectedIdx++;
    }
    LaGenMatDouble D(arrayD, directPointSelectedCount(), 3);

    directLinearSolve(D);
    calcFinalResult();
}

/* 将三角化时每个参数方向上的采样点数量设置为 count */
void CommonData::setSamplePointCount(int count) {
    m_nSamplePointCount = count;
    m_bFirstLoad = true;
}

/* 设置变形算法（CYM，PN_CUTTING 或 PN_NO_CUTTING） */

void CommonData::setAlgorithmType() {
    if (algorithm_type_ == CYM) {
        algorithm_type_ = LZQ;
        cout << "pn_cutting" << endl;
    } else if (algorithm_type_ == PN_CUTTING) {
        algorithm_type_ = PN_NO_CUTTING;
        cout << "pn_no_cutting" << endl;
    } else if (algorithm_type_ == PN_NO_CUTTING) {
        algorithm_type_ = LZQ;
        cout << "lzq" << endl;
    } else {
        algorithm_type_ = CYM;
        cout << "cym" << endl;
    }
}

/* 设置当前使用的变形算法（FFD 或 AFFD） */
void CommonData::setAlgorithm(const Algorithm algo) {
    m_eAlgorithm = algo;
    if (m_bLoaded)
        calcFinalResult();
}

MtlTex CommonData::getMtl(int i) const {
    if (i >= (int) objData->mtlTexList.size())
        return MtlTex();
    return objData->mtlTexList[i];
}

/* 计算节点向量和控制顶点 */
void CommonData::calcKnotCtrlPoint() {
    /* 计算节点向量 */
    for (int i = 0; i < 3; ++i) {
        int knotCount = m_nCtrlPointCount[i] + m_nOrder[i];        // 节点个数＝控制顶点个数＋阶数
        m_nKnotIntervalCount[i] = knotCount - (m_nOrder[i] - 1) * 2 - 1;    // 节点区间个数（重节点之间的不算）
        /* 前 m_nOrder[i] 个节点都是该方向最小值 */
        for (int j = 0; j < m_nOrder[i]; ++j)
            knotList[i][j] = m_fMin[i] * (1 + EXPAND);
        //knotList[i][j] = m_fMin[i];
        /* 中间一部分节点均匀分布 */
        for (int j = m_nOrder[i]; j < m_nCtrlPointCount[i]; ++j)
            knotList[i][j] =
                    m_fMin[i] + (double) (j - m_nOrder[i] + 1) * (m_fMax[i] - m_fMin[i]) / m_nKnotIntervalCount[i];
        /* 最后 m_nOrder[i] 个节点都是该方向最大值 */
        for (int j = m_nCtrlPointCount[i]; j < knotCount; ++j)
            knotList[i][j] = m_fMax[i] * (1 + EXPAND);
    }

    /* 计算 B 样条体控制顶点辅助向量 */
    int ctrlPointAuxList[3][MAXCTRLPOINT];
    for (int i = 0; i < 3; ++i) {
        if (m_nOrder[i] == m_nCtrlPointCount[i])    // 控制顶点均匀分布
        {
            for (int j = 0; j < m_nCtrlPointCount[i]; ++j) {
                ctrlPointAuxList[i][j] = j;
            }
        }
            /* 控制顶点呈1k, 2k, 3k, ... , degree * k, ... , degree * k, ... , 3k, 2k, 1k分布 */
        else {
            vector<int> lengthVector;
            int totalInterval = 0, currentInterval = 0, j;
            for (j = 0; j <= m_nCtrlPointCount[i] / 2; ++j) {
                totalInterval += currentInterval;
                lengthVector.push_back(currentInterval);
                ctrlPointAuxList[i][j] = totalInterval;
                if (currentInterval < m_nOrder[i] - 1)
                    currentInterval++;
            }
            int startIdx = 0;
            if (m_nCtrlPointCount[i] % 2 == 0) {
                startIdx = lengthVector.size() - 2;
            }
            else {
                startIdx = lengthVector.size() - 1;
            }
            for (; j < m_nCtrlPointCount[i]; ++j) {
                int temp = ctrlPointAuxList[i][j - 1] + lengthVector[startIdx--];
                ctrlPointAuxList[i][j] = temp;
            }
        }
    }
    /* 计算控制顶点坐标 */
    for (int i = 0; i < m_nCtrlPointCount[U]; ++i) {
        double coordX =
                m_fLength[X] * 2 * ctrlPointAuxList[U][i] / ctrlPointAuxList[U][m_nCtrlPointCount[U] - 1] + m_fMin[X];
        for (int j = 0; j < m_nCtrlPointCount[V]; ++j) {
            double coordY = m_fLength[Y] * 2 * ctrlPointAuxList[V][j] / ctrlPointAuxList[V][m_nCtrlPointCount[V] - 1] +
                            m_fMin[Y];
            for (int k = 0; k < m_nCtrlPointCount[W]; ++k) {
                double coordZ =
                        m_fLength[Z] * 2 * ctrlPointAuxList[W][k] / ctrlPointAuxList[W][m_nCtrlPointCount[W] - 1] +
                        m_fMin[Z];
                //sourceCtrlPoint[i][j][k] = ctrlPoint[i][j][k] = CtrlPoint(coordX, coordY, coordZ);
                ctrlPoint[i][j][k] = CtrlPoint(coordX, coordY, coordZ);
            }
        }
    }
}

#ifdef a1231412341234
void BSplineBase(double t, int leftIdx, const Direction direction)
{
    double temp[4] = {0.0};
    for (int r = 1; r <= m_nOrder[direction] - 1; ++r)
    {
        for (int iter = 0; iter <= m_nOrder[direction] - r - 1; ++iter)
        {
            int i = leftIndex - iter;
            double coefficient0, coefficient1;
            coefficient0 = (t - knotList[direction][i]) / (knotList[direction][i + m_nOrder[direction] - r] - knotList[direction][i]);
            coefficient1 = (knotList[direction][i + m_nOrder[direction] - r] - t) / (knotList[direction][i + m_nOrder[direction] - r] - knotList[direction][i]);
            temp[iter] = cp[iter] * coefficient0 + cp[iter + 1] * coefficient1;
        }
        for (int j = 0; j < 5; ++j)
        {
            cp[j] = temp[j];
        }
    }
    return cp[0];
}
#endif

double CommonData::BSplineBase(int i, int k, double t, const Direction direction) {
    if (k == 1) {
        int lastKnotIdx = m_nOrder[direction] + m_nCtrlPointCount[direction] - 1;
        if (fabs(t - knotList[direction][lastKnotIdx]) < ZERO && i + 1 == m_nCtrlPointCount[direction]) {
            //outFile << "ZERO return 1" << endl << endl;
            return 1;
        }
        else {
            if (t >= knotList[direction][i] && t < knotList[direction][i + 1]) {
                if (direction == U) {
                    //outFile << "middle return 1" << endl << endl;
                }
                return 1;
            }
            else {
                if (direction == U) {
                    //outFile << "other return 0" << endl << endl;
                }
                return 0;
            }
        }
    }
    else {
        double coe0, coe1;
        if (fabs(knotList[direction][i + k - 1] - knotList[direction][i]) < ZERO)
            coe0 = 0.0;
        else
            coe0 = (t - knotList[direction][i]) / (knotList[direction][i + k - 1] - knotList[direction][i]);

        if (fabs(knotList[direction][i + k] - knotList[direction][i + 1]) < ZERO)
            coe1 = 0.0;
        else
            coe1 = (knotList[direction][i + k] - t) / (knotList[direction][i + k] - knotList[direction][i + 1]);
        return coe0 * BSplineBase(i, k - 1, t, direction) + coe1 * BSplineBase(i + 1, k - 1, t, direction);
    }
}

void CommonData::saveEdit(std::ofstream &fout) {
    fout << "B样条体阶数\t\t\t" << m_nOrder[U] << " " << m_nOrder[V] << " " << m_nOrder[W] << endl;
    fout << "B样条体顶点数\t\t" << m_nCtrlPointCount[U] << " "
    << m_nCtrlPointCount[V] << " " << m_nCtrlPointCount[W] << endl;

    fout << "控制顶点\t\t\t" << m_nCtrlPointCount[U] * m_nCtrlPointCount[V] * m_nCtrlPointCount[W] << endl;
    for (int i = 0; i < m_nCtrlPointCount[U]; ++i) {
        for (int j = 0; j < m_nCtrlPointCount[V]; ++j) {
            for (int k = 0; k < m_nCtrlPointCount[W]; ++k) {
                fout << "\t\t\t\t\t" << ctrlPoint[i][j][k] << endl;
            }
        }
    }

    fout << "直接编辑点\t\t\t" << directPointVector.size() << endl;
    for (vector<DirectPoint>::size_type i = 0; i != directPointVector.size(); ++i) {
        fout << "\t\t\t\t\t" << directPointVector[i] << endl;
    }
}

#define XMIN 1
#define YMIN 2
#define ZMIN 3


void report(struct triangulateio *io) {
    int i, j;

    for (i = 0; i < io->numberofpoints; i++) {
        printf("Point %4d:", i);
        for (j = 0; j < 2; j++) {
            printf("  %.6g", io->pointlist[i * 2 + j]);
        }
    }
}

#define SPLIT_DEGREE 0.2

void CommonData::split(VertexCoord pCoord[], int normalCount[], vector<SplitResultPoint> &points,
                       vector<SplitResultTriangle> &triangles) {
    //根据边的长度，算出各个边应该分成几段
    int spitNum[3];
    spitNum[0] = (int) (round((pCoord[0] - pCoord[1]).norm()) / SPLIT_DEGREE);
    spitNum[1] = (int) (round((pCoord[1] - pCoord[2]).norm()) / SPLIT_DEGREE);
    spitNum[2] = (int) (round((pCoord[2] - pCoord[0]).norm()) / SPLIT_DEGREE);

    for (int i1 = 0; i1 < 3; ++i1) {
        if (spitNum[i1] < 1) {
            spitNum[i1] = 1;
        }
//        cout << spitNum[i1] << " ";
    }
//    cout << endl;



    //因为是所用的细分库是二维的，所以传入的三角形要舍弃一个维度，为了使细分效果跟好，
    //舍弃一个维度后要保证三角形面积尽量大
    //所用的算法是计算三角形的法向，然后根据法向的最大分量确定舍弃的维度。

    VertexCoord u = pCoord[0] - pCoord[1];
    VertexCoord v = pCoord[0] - pCoord[2];

    VertexCoord normal;
    normal.x(u.y() * v.z() - u.z() * v.y());
    normal.y(u.z() * v.x() - u.x() * v.z());
    normal.z(u.x() * v.y() - u.y() * v.x());

    normal.x(fabs(normal.x()));
    normal.y(fabs(normal.y()));
    normal.z(fabs(normal.z()));


    int deletedAxis;
    if (normal.x() > normal.y() && normal.x() > normal.z()) {
        deletedAxis = XMIN;
    } else if (normal.y() > normal.z()) {
        deletedAxis = YMIN;
    } else {
        deletedAxis = ZMIN;
    }

    struct triangulateio in, out;
    in.acinit();
    out.acinit();

    /* Define input points. */

    in.numberofpoints = spitNum[0] + spitNum[1] + spitNum[2];
    in.numberofpointattributes = 0;
    in.pointlist = (double *) malloc(in.numberofpoints * 2 * sizeof(double));

    TextureCoord tempPoint[3];
    switch (deletedAxis) {
        case XMIN:
            for (int j = 0; j < 3; ++j) {
                tempPoint[j].u(pCoord[j].y());
                tempPoint[j].v(pCoord[j].z());
            }
            break;

        case YMIN:
            for (int j = 0; j < 3; ++j) {
                tempPoint[j].u(pCoord[j].x());
                tempPoint[j].v(pCoord[j].z());
            }
            break;

        case ZMIN:

            for (int j = 0; j < 3; ++j) {
                tempPoint[j].u(pCoord[j].x());
                tempPoint[j].v(pCoord[j].y());
            }
            break;
        default:
            break;
    }

    int index = 0;
    for (int n = 0; n < 3; ++n) {
        TextureCoord p0 = tempPoint[n];
        TextureCoord p1 = tempPoint[(n + 1) % 3];
        if (spitNum[n] == 1) {
//            cout <<index / 2 + 1 << " " << p0 << endl;
            if (fabs(p0.v()) < ZERO) {
                p0.v(0);
            }

            if (fabs(p0.u()) < ZERO) {
                p0.u(0);
            }
            in.pointlist[index++] = p0.u();
            in.pointlist[index++] = p0.v();
        } else {
            TextureCoord delta = p1 - p0;
            delta.u(delta.u() / spitNum[n]);
            delta.v(delta.v() / spitNum[n]);
            for (int i = 0; i < spitNum[n]; ++i) {
                TextureCoord middleP = p0 + i * delta;
//                cout <<index / 2 + 1 << " " << middleP << endl;
//                cout <<index / 2 + 1 << " " << delta << endl;
                if (fabs(middleP.v()) < ZERO) {
                    middleP.v(0);
                }

                if (fabs(middleP.u()) < ZERO) {
                    middleP.u(0);
                }
                in.pointlist[index++] = middleP.u();
                in.pointlist[index++] = middleP.v();
            }
        }
    }
    in.pointmarkerlist = (int *) NULL;
    in.numberofsegments = in.numberofpoints;
    in.segmentlist = (int *) malloc(in.numberofsegments * 2 * sizeof(int));

    for (int l1 = 0; l1 < in.numberofsegments - 1; ++l1) {
        in.segmentlist[l1 * 2] = l1;
        in.segmentlist[l1 * 2 + 1] = l1 + 1;
    }

    in.segmentlist[in.numberofsegments * 2 - 2] = in.numberofsegments - 1;
    in.segmentlist[in.numberofsegments * 2 - 1] = 0;

//    for (int k1 = 0; k1 < in.numberofsegments; ++k1) {
//        cout << k1 + 1 << " " << in.segmentlist[k1 * 2] << " " << in.segmentlist[k1 * 2 + 1] << endl;
//    }

    in.numberofholes = 0;
    in.numberofregions = 0;

    /* Make necessary initializations so that Triangle can return a */
    /*   triangulation in `mid' and a voronoi diagram in `vorout'.  */

    out.pointlist = (REAL *) NULL;            /* Not needed if -N switch used. */
    /* Not needed if -N switch used or number of point attributes is zero: */
    out.pointattributelist = (REAL *) NULL;
    out.pointmarkerlist = (int *) NULL; /* Not needed if -N or -B switch used. */
    out.trianglelist = (int *) NULL;          /* Not needed if -E switch used. */
    /* Not needed if -E switch used or number of triangle attributes is zero: */
    out.triangleattributelist = (REAL *) NULL;
    out.neighborlist = (int *) NULL;         /* Needed only if -n switch used. */
    /* Needed only if segments are output (-p or -c) and -P not used: */
    out.segmentlist = (int *) NULL;
    /* Needed only if segments are output (-p or -c) and -P and -B not used: */
    out.segmentmarkerlist = (int *) NULL;
    out.edgelist = (int *) NULL;             /* Needed only if -e switch used. */
    out.edgemarkerlist = (int *) NULL;   /* Needed if -e used and -B not used. */


    /* Triangulate the points.  Switches are chosen to read and write a  */
    /*   PSLG (p), preserve the convex hull (c), number everything from  */
    /*   zero (z), assign a regional attribute to each element (A), and  */
    /*   produce an edge list (e), a Voronoi diagram (v), and a triangle */
    /*   neighbor list (n).                                              */

    char *c = "zQpq20a0.0005";


    triangulate(c, &in, &out, NULL);

    //count inter point number
    int counter = 0;
    for (int l = 0; l < out.numberofpoints; ++l) {
        if (out.pointmarkerlist[l] == 0) {
            ++counter;
        }
    }

    int newPointNumber = counter + in.numberofpoints;
    double *newPointList = (double *) malloc(newPointNumber * 2 * sizeof(double));
    memcpy(newPointList, in.pointlist, in.numberofpoints * 2 * sizeof(double));
    int currentIndex = in.numberofpoints * 2;
    for (int l = 0; l < out.numberofpoints; ++l) {
        if (out.pointmarkerlist[l] == 0) {
            newPointList[currentIndex++] = out.pointlist[l * 2];
            newPointList[currentIndex++] = out.pointlist[l * 2 + 1];
        }
    }
//    cout << newPointNumber << endl;
//    if (newPointNumber * 2 == currentIndex){ cout << "ok ";
//    }
    free(in.pointlist);
    in.pointlist = newPointList;
    in.numberofpoints = newPointNumber;


    out.acfree();
    out.acinit();

    out.pointlist = (REAL *) NULL;            /* Not needed if -N switch used. */
    /* Not needed if -N switch used or number of point attributes is zero: */
    out.pointattributelist = (REAL *) NULL;
    out.pointmarkerlist = (int *) NULL; /* Not needed if -N or -B switch used. */
    out.trianglelist = (int *) NULL;          /* Not needed if -E switch used. */
    /* Not needed if -E switch used or number of triangle attributes is zero: */
    out.triangleattributelist = (REAL *) NULL;
    out.neighborlist = (int *) NULL;         /* Needed only if -n switch used. */
    /* Needed only if segments are output (-p or -c) and -P not used: */
    out.segmentlist = (int *) NULL;
    /* Needed only if segments are output (-p or -c) and -P and -B not used: */
    out.segmentmarkerlist = (int *) NULL;
    out.edgelist = (int *) NULL;             /* Needed only if -e switch used. */
    out.edgemarkerlist = (int *) NULL;   /* Needed if -e used and -B not used. */

    c = (char *) "zQpc";
    triangulate(c, &in, &out, NULL);

    //构造矩阵用于求逆
    LaGenMatDouble tempMat(3, 3);
    LaVectorLongInt piv(3);
    for (int i = 0; i < 3; ++i) {
        tempMat(0, i) = tempPoint[i].u();
        tempMat(1, i) = tempPoint[i].v();
    }
    tempMat(2, 0) = 1;
    tempMat(2, 1) = 1;
    tempMat(2, 2) = 1;

    LaGenMatDouble originalVertex(3, 3);
    for (int i = 0; i < 3; ++i) {
        originalVertex(0, i) = pCoord[i].x();
        originalVertex(1, i) = pCoord[i].y();
        originalVertex(2, i) = pCoord[i].z();
    }
    LUFactorizeIP(tempMat, piv);
    LaLUInverseIP(tempMat, piv);

    LaGenMatDouble result(3, 1);

//    cout << endl << "in points num" << in.numberofpoints << " ";
//    cout << "out points num" << out.numberofpoints << endl;
    for (int k = 0; k < out.numberofpoints; ++k) {
//        cout << out.pointmarkerlist[k] << " ";
        result(0, 0) = out.pointlist[k * 2];
        result(1, 0) = out.pointlist[k * 2 + 1];
        result(2, 0) = 1;

        LaGenMatDouble abc(3, 1);
        Blas_Mat_Mat_Mult(tempMat, result, abc, false, false);
        SplitResultPoint splitResultPoint;
        splitResultPoint.bary.x(abc(0, 0));
        splitResultPoint.bary.y(abc(1, 0));
        splitResultPoint.bary.z(abc(2, 0));
        VertexCoord bary = splitResultPoint.bary;
        if (fabs(1 - bary.x()) < ZERO
            || fabs(1 - bary.y()) < ZERO
            || fabs(1 - bary.z()) < ZERO) {
            splitResultPoint.isOriginal = true;
            splitResultPoint.isBorder = false;
            splitResultPoint.isInter = false;
            if (fabs(1 - bary.x()) < ZERO) {
                splitResultPoint.normalCount = normalCount[0];
            } else if (fabs(1 - bary.y()) < ZERO) {
                splitResultPoint.normalCount = normalCount[1];
            } else {
                splitResultPoint.normalCount = normalCount[2];
            }
        } else if (fabs(bary.z()) < ZERO
                   || fabs(bary.y()) < ZERO
                   || fabs(bary.z()) < ZERO) {
            splitResultPoint.isOriginal = false;
            splitResultPoint.isBorder = true;
            splitResultPoint.isInter = false;
            if (fabs(bary.x()) < ZERO) {
                splitResultPoint.normalCount = normalCount[1] > normalCount[2] ? normalCount[1] : normalCount[2];
            } else if (fabs(bary.y()) < ZERO) {
                splitResultPoint.normalCount = normalCount[0] > normalCount[2] ? normalCount[0] : normalCount[2];
            } else {
                splitResultPoint.normalCount = normalCount[0] > normalCount[1] ? normalCount[0] : normalCount[1];
            }
        } else {
            splitResultPoint.isOriginal = false;
            splitResultPoint.isBorder = false;
            splitResultPoint.isInter = true;
            splitResultPoint.normalCount = 1;
        }


        LaGenMatDouble newPosition(3, 1);
        Blas_Mat_Mat_Mult(originalVertex, abc, newPosition, false, false);
        splitResultPoint.vertex.x(newPosition(0, 0));
        splitResultPoint.vertex.y(newPosition(1, 0));
        splitResultPoint.vertex.z(newPosition(2, 0));
        points.push_back(splitResultPoint);
    }

    for (int m = 0; m < out.numberoftriangles; ++m) {
        SplitResultTriangle triangle;
        triangle.vertexId[0] = out.trianglelist[m * 3];
        triangle.vertexId[1] = out.trianglelist[m * 3 + 1];
        triangle.vertexId[2] = out.trianglelist[m * 3 + 2];
        triangles.push_back(triangle);
    }

    in.acfree();
    out.acfree();
}

int getID(std::map<point *, int> ids, vector<pointSharePtr> &points, pointSharePtr key) {
    if (ids.find(key.get()) == ids.end()) {
        unsigned long i = ids.size();
        ids[key.get()] = (int) i;
        points.push_back(key);
    }

    return ids[key.get()];
}

void CommonData::splitUseNewMethod(VertexCoord pCoord[], int normalCount[],
                                   std::vector<objdata::SplitResultPoint> &outPoints,
                                   std::vector<objdata::SplitResultTriangle> &outTriangles) {

    triangle t(std::make_shared<point>(pCoord[0].x(), pCoord[0].y(), pCoord[0].z()),
               std::make_shared<point>(pCoord[1].x(), pCoord[1].y(), pCoord[1].z()),
               std::make_shared<point>(pCoord[2].x(), pCoord[2].y(), pCoord[2].z()));
    t.rotate();
    vector<TriangleSharePtr> triangles = split1(t, SPLIT_DEGREE);

    std::vector<pointSharePtr> points = point::getPointPool();
    std::map<point *, int> ids;
    for (int i = 0; i < points.size(); ++i) {
        ids[points[i].get()] = i;
    }

    LaGenMatDouble result(3, 1);
    for (TriangleSharePtr trianglePtr : triangles) {
        SplitResultTriangle srt;
        srt.vertexId[0] = getID(ids, points, trianglePtr->getP1());
        srt.vertexId[1] = getID(ids, points, trianglePtr->getP2());
        srt.vertexId[2] = getID(ids, points, trianglePtr->getP3());
        outTriangles.push_back(srt);
    }

    for (int k = 0; k < points.size(); ++k) {
//        cout << out.pointmarkerlist[k] << " ";
        pointSharePtr p = points[k];
        result(0, 0) = p->getX();
        result(1, 0) = p->getY();
        result(2, 0) = 1;

        LaGenMatDouble abc(3, 1);
        Blas_Mat_Mat_Mult(t.getAuxMatrixForContain(), result, abc, false, false);
        SplitResultPoint splitResultPoint;
        splitResultPoint.bary.x(abc(0, 0));
        splitResultPoint.bary.y(abc(1, 0));
        splitResultPoint.bary.z(abc(2, 0));
        VertexCoord bary = splitResultPoint.bary;
        if (fabs(1 - bary.x()) < 0.0000001f
            || fabs(1 - bary.y()) < 0.0000001f
            || fabs(1 - bary.z()) < 0.0000001f) {
            splitResultPoint.isOriginal = true;
            splitResultPoint.isBorder = false;
            splitResultPoint.isInter = false;
            if (fabs(1 - bary.x()) < 0.0000001f) {
                splitResultPoint.normalCount = normalCount[0];
            } else if (fabs(1 - bary.y()) < 0.0000001f) {
                splitResultPoint.normalCount = normalCount[1];
            } else {
                splitResultPoint.normalCount = normalCount[2];
            }
        } else if (fabs(bary.z()) < 0.0000001f
                   || fabs(bary.y()) < 0.0000001f
                   || fabs(bary.z()) < 0.0000001f) {
            splitResultPoint.isOriginal = false;
            splitResultPoint.isBorder = true;
            splitResultPoint.isInter = false;
            if (fabs(bary.x()) < 0.0000001f) {
                splitResultPoint.normalCount = normalCount[1] > normalCount[2] ? normalCount[1] : normalCount[2];
            } else if (fabs(bary.y()) < 0.0000001f) {
                splitResultPoint.normalCount = normalCount[0] > normalCount[2] ? normalCount[0] : normalCount[2];
            } else {
                splitResultPoint.normalCount = normalCount[0] > normalCount[1] ? normalCount[0] : normalCount[1];
            }
        } else {
            splitResultPoint.isOriginal = false;
            splitResultPoint.isBorder = false;
            splitResultPoint.isInter = true;
            splitResultPoint.normalCount = 1;
        }


        LaGenMatDouble newPosition(3, 1);

        LaGenMatDouble originalVertex(3, 3);
        for (int i = 0; i < 3; ++i) {
            originalVertex(0, i) = pCoord[i].x();
            originalVertex(1, i) = pCoord[i].y();
            originalVertex(2, i) = pCoord[i].z();
        }
        Blas_Mat_Mat_Mult(originalVertex, abc, newPosition, false, false);
        splitResultPoint.vertex.x(newPosition(0, 0));
        splitResultPoint.vertex.y(newPosition(1, 0));
        splitResultPoint.vertex.z(newPosition(2, 0));
        outPoints.push_back(splitResultPoint);
    }

}

static const int DEGREE = 3;

void CommonData::preComputerSamplerParamter() {
    for (Triangle &triangle : triangleList) {
        //表示是第几个控制顶点，degree是3,一共有十个控制顶点
        for (int samplePointIndex = 0; samplePointIndex < 28; ++samplePointIndex) {
            float tempFloorFloat = (sqrtf((float) samplePointIndex * 8 + 9) - 3) * 0.5;
            int floor = rintf(tempFloorFloat);
            if ((floor * 2 + 3) * (floor * 2 + 3) != samplePointIndex * 8 + 9)
                floor = ceilf(tempFloorFloat);
            int room = samplePointIndex - ((floor + 1) * floor >> 1);
            VertexCoord barycentric_coord;
            barycentric_coord.x((float) (DEGREE - floor) / DEGREE);
            barycentric_coord.y((float) (floor - room) / DEGREE);
            barycentric_coord.z(1.0f - barycentric_coord.x() - barycentric_coord.y());

            VertexCoord v0 = triangle.v[0];
            VertexCoord v1 = triangle.v[1];
            VertexCoord v2 = triangle.v[2];

            // u, v, w 表示经过重心坐标插值之后的采样点的x, y, z分量
            float u = (float) (
                    v0.x() * barycentric_coord.x() + v1.x() * barycentric_coord.y() + v2.x() * barycentric_coord.z());
            float v = (float) (
                    v0.y() * barycentric_coord.x() + v1.y() * barycentric_coord.y() + v2.y() * barycentric_coord.z());
            float w = (float) (
                    v0.z() * barycentric_coord.x() + v1.z() * barycentric_coord.y() + v2.z() * barycentric_coord.z());
            triangle.barr[samplePointIndex].x(u);
            triangle.barr[samplePointIndex].y(v);
            triangle.barr[samplePointIndex].z(w);

            int orderU = 3, orderV = 3, orderW = 3;
            int ctrlPointNumU = 5;
            int ctrlPointNumV = 5;
            int ctrlPointNumW = 5;

            // u, v, w方向节点区间数量
            int knot_interval_count_u = orderU + ctrlPointNumU - (orderU - 1) * 2 - 1;
            int knot_interval_count_v = orderV + ctrlPointNumV - (orderV - 1) * 2 - 1;
            int knot_interval_count_w = orderW + ctrlPointNumW - (orderW - 1) * 2 - 1;

            // 预先将其值设为最大，将末端点归入最后一段
            int left_idx_u = orderU - 1 + knot_interval_count_u - 1;
            int left_idx_v = orderV - 1 + knot_interval_count_v - 1;
            int left_idx_w = orderW - 1 + knot_interval_count_w - 1;


            // 沿 U 方向查找当前点所在的节点区间
            for (int ii = orderU - 1; ii <= orderU - 1 + knot_interval_count_u - 1; ++ii) {
                if (u >= knotList[0][ii] && u < knotList[0][ii + 1]) {
                    left_idx_u = ii;
                    break;
                }
            }
            // 沿 V 方向查找当前点所在的节点区间
            for (int jj = orderV - 1; jj <= orderV - 1 + knot_interval_count_v - 1; ++jj) {
                if (v >= knotList[1][jj] && v < knotList[1][jj + 1]) {
                    left_idx_v = jj;
                    break;
                }
            }
            // 沿 W 方向查找当前点所在的节点区间
            for (int kk = orderW - 1; kk <= orderW - 1 + knot_interval_count_w - 1; ++kk) {
                if (w >= knotList[2][kk] && w < knotList[2][kk + 1]) {
                    left_idx_w = kk;
                    break;
                }
            }

            triangle.boxInfo[samplePointIndex][0] = left_idx_u;
            triangle.boxInfo[samplePointIndex][1] = left_idx_v;
            triangle.boxInfo[samplePointIndex][2] = left_idx_w;

            float tmpKnot = knotList[0][left_idx_u];
            float tmpKnot1 = knotList[0][left_idx_u + 1];
            float x_stride = tmpKnot1 - tmpKnot;

            tmpKnot = knotList[1][left_idx_v];
            tmpKnot1 = knotList[1][left_idx_v + 1];
            float y_stride = tmpKnot1 - tmpKnot;

            tmpKnot = knotList[2][left_idx_w];
            tmpKnot1 = knotList[2][left_idx_w + 1];
            float z_stride = tmpKnot1 - tmpKnot;

            triangle.samplePoint[samplePointIndex].x((u - tmpKnot) / x_stride);
            triangle.samplePoint[samplePointIndex].y((v - tmpKnot) / y_stride);
            triangle.samplePoint[samplePointIndex].z((w - tmpKnot) / z_stride);

            triangle.strip[samplePointIndex].x(x_stride);
            triangle.strip[samplePointIndex].y(y_stride);
            triangle.strip[samplePointIndex].z(z_stride);

        }
    }
}

void CommonData::acSplit() {
    vector<vector<int> > mtlFaceList(objData->mtlTexList.size() + 1);
    int triangleIdx = 0;


    for (int i = 0; i < faceList.size(); ++i) {
        vector<int> &vertexCoordIndex = faceList[i].vertexCoordIndex;

        //获得三个顶点
        VertexCoord v_origin[3]; //表示物体原来的三个顶点
        int normalCount[3]; //表示物体原来的三个顶点
        int idx1 = vertexCoordIndex[0];
        int idx2 = vertexCoordIndex[1];
        int idx3 = vertexCoordIndex[2];

        v_origin[0] = vertexCoordList[idx1];
        normalCount[0] = findNormalCount(idx1, idx3);

        v_origin[1] = vertexCoordList[idx2];
        normalCount[1] = findNormalCount(idx2, idx1);

        v_origin[2] = vertexCoordList[idx3];
        normalCount[2] = findNormalCount(idx3, idx2);

        //分割三角形
        vector<SplitResultTriangle> splitResultTriangle;
        vector<SplitResultPoint> splitResultPoint;
//        split(v_origin, normalCount, splitResultPoint, splitResultTriangle);
#ifdef TEST
        std::cout << "acsplit face index:" << i << endl;
#endif
        splitUseNewMethod(v_origin, normalCount, splitResultPoint, splitResultTriangle);

        //取出原始三角片的三个顶点法向
        VertexCoord normal1 = objData->normalCoordList[faceList[i].normalCoordIndex[0]];
        VertexCoord normal2 = objData->normalCoordList[faceList[i].normalCoordIndex[1]];
        VertexCoord normal3 = objData->normalCoordList[faceList[i].normalCoordIndex[2]];
        normal1.normalize();
        normal2.normalize();
        normal3.normalize();

#ifdef TEST
        cout << "acSplit result triangle size :"
        << splitResultTriangle.size() << endl;
#endif

//        if (splitResultTriangle.size() > 20) {
//            cout << normal1 << endl;
//            cout << normal2 << endl;
//            cout << normal3 << endl;
//        }

//        TextureCoord textureCoord1;
//        TextureCoord textureCoord2;
//        TextureCoord textureCoord3;
//        if (faceList[i].m_eFaceCase == V_T_N) {
//            textureCoord1 = objData->textureCoordList[faceList[i].textureCoordIndex[0]];
//            textureCoord2 = objData->textureCoordList[faceList[i].textureCoordIndex[1]];
//            textureCoord3 = objData->textureCoordList[faceList[i].textureCoordIndex[2]];
//        }


        long nMtlIdx = faceList[i].m_nMtlIdx;
        if (nMtlIdx == -1) {
            nMtlIdx = mtlFaceList.size() - 1;
        }

        for (int j = 0; j < splitResultTriangle.size(); ++j) {

            //调整切割点和法向量
            for (int k = 0; k < 3; ++k) {
                SplitResultPoint &point = splitResultPoint[splitResultTriangle[j].vertexId[k]];
                point.normal = normal1 * point.bary.x()
                               + normal2 * point.bary.y()
                               + normal3 * point.bary.z();

//                cout << point.bary << endl;

                point.normal.normalize();

//                point.textureCoord = textureCoord1 * point.bary.x()
//                                     + textureCoord2 * point.bary.y()
//                                     + textureCoord3 * point.bary.z();

                if (adjust_split_points_ && !point.isOriginal) {
                    point.vertex = cubicBezierTriangleList[i].calcValue(point.bary);
                    point.normalAdjusted = quadraticBezierTriangleList[i].calcValue(point.bary);
                    point.normalAdjusted.normalize();
                } else {
                    point.normalAdjusted = point.normal;
                }

//                point.normal = point.normalAdjusted;
            }

            SplitResultPoint point1 = splitResultPoint[splitResultTriangle[j].vertexId[0]];
            SplitResultPoint point2 = splitResultPoint[splitResultTriangle[j].vertexId[1]];
            SplitResultPoint point3 = splitResultPoint[splitResultTriangle[j].vertexId[2]];
//
//            VertexCoord u = point1.vertex - point2.vertex;
//            VertexCoord v = point2.vertex - point3.vertex;
//
//            VertexCoord normal;
//            normal.x(u.y() * v.z() - u.z() * v.y());
//            normal.y(u.z() * v.x() - u.x() * v.z());
//            normal.z(u.x() * v.y() - u.y() * v.x());
//
//            if (normal * point1.normal < 0) {
//                point2 = splitResultPoint[splitResultTriangle[j].vertexId[2]];
//                point3 = splitResultPoint[splitResultTriangle[j].vertexId[1]];
//            }
#ifdef LINE

            Triangle t(point1.vertex, point2.vertex, point3.vertex,
                       point1.normal, point2.normal, point3.normal,
                       point1.normalAdjusted, point2.normalAdjusted, point3.normalAdjusted,
                       point1.bary, point2.bary, point3.bary,
                       v_origin, i,
                       point1.normalCount, point2.normalCount, point3.normalCount);
//                       1, 1, 1);
//                       point1.textureCoord, point2.textureCoord, point3.textureCoord);
#else

            Triangle t(point1.vertex, point2.vertex, point3.vertex,
                       point1.normal, point2.normal, point3.normal,
                       point1.normalAdjusted, point2.normalAdjusted, point3.normalAdjusted,
//                       point1.bary, point2.bary, point3.bary,
                       v_origin, i,
                       point1.normalCount, point2.normalCount, point3.normalCount);
//                       2,2,2);
//                       point1.textureCoord, point2.textureCoord, point3.textureCoord);

#endif
            triangleList.push_back(t);

            face_children_table_[i].push_front(triangleList.size() - 1);
            mtlFaceList[nMtlIdx].push_back(triangleIdx++);
        }
    }

    /* 将此模型中所有的三角形顶点统一编号，所有面片根据材质的不同，存入faceMtlList中 */
    int samplePointPerTriangle = m_nSamplePointCount * (m_nSamplePointCount + 1) / 2;
    m_nTessPointCount = (int) (samplePointPerTriangle * triangleList.size());
    int segment = m_nSamplePointCount - 1;
    int subTrianglePerTriangle = segment * segment;

    int totalFaceNum = 0;
    for (vector<vector<int> >::size_type ii = 0; ii < mtlFaceList.size(); ++ii)
        totalFaceNum += mtlFaceList[ii].size();
    int faceCounter = 0;
    for (vector<vector<int> >::size_type ii = 0; ii < mtlFaceList.size(); ++ii) {
        int *tempFaceList = new int[mtlFaceList[ii].size() * subTrianglePerTriangle * 3];
        int pos = 0;
        for (vector<int>::size_type jj = 0; jj < mtlFaceList[ii].size(); ++jj) {
            int idx = mtlFaceList[ii][jj];
            int pointBase = samplePointPerTriangle * idx;
            for (int i = 0; i < segment; ++i) {
                for (int j = 0; j <= i; ++j) {
                    tempFaceList[pos++] = pointBase + triangleCoord(i, j);
                    tempFaceList[pos++] = pointBase + triangleCoord(i + 1, j);
                    tempFaceList[pos++] = pointBase + triangleCoord(i + 1, j + 1);
                    if (i < segment - 1) {
                        tempFaceList[pos++] = pointBase + triangleCoord(i + 1, j);
                        tempFaceList[pos++] = pointBase + triangleCoord(i + 2, j + 1);
                        tempFaceList[pos++] = pointBase + triangleCoord(i + 1, j + 1);
                    }
                }
            }
        }
        faceCounter += mtlFaceList[ii].size();
        faceMtlList.push_back(tempFaceList);
        faceMtlCountList.push_back((const int &) (mtlFaceList[ii].size() * subTrianglePerTriangle));
    }


    // 对三角化之后的模型重新寻找相邻关系
    // triangle_adjacent_table_中的元素三个一组，[3i+0], [3i+1], [3i+2]
    // 分别代表与i号面片的0号、1号、2号边相邻的三角形信息。0号边是v2v0，1号边是v0v1，2号边是v1v2
    // 每个元素的高30位存储了对方三角形的编号，低两位存储了对方三角形的相邻边编号(可能是0, 1, 2)
    // 初始情况将所有元素都设置成-1234
    triangle_adjacent_table_ = new int[triangleList.size() * 3];
    std::fill(triangle_adjacent_table_, triangle_adjacent_table_ + triangleList.size() * 3, -1234);
    for (vector<Triangle>::size_type i = 0; i < triangleList.size(); ++i) {
        //cout << "triangle = " << i << endl;
        int ori_idx = triangleList[i].origin_face_idx_;
        // 遍历i号三角形的亲兄弟三角形
        for (list<int>::iterator it = face_children_table_[ori_idx].begin();
             it != face_children_table_[ori_idx].end(); ++it) {
            int result;
            //cout << "\tchild = " << *it << endl;
            if ((result = triangularAdjacent(i, *it)) >= 0) {
                //cout << "\t相邻" << endl;
                int idx0 = result >> 4;
                int idx1 = result & 0xF;
                triangle_adjacent_table_[i * 3 + idx0] = *it << 2 | idx1;
                //cout << "face_idx = " << *it << " " << idx0 << " " << idx1
                //<< " " << i * 3 + idx0 << "=" << triangle_adjacent_table_[i * 3 + idx0] << endl;
            }
        }
        // 遍历i号三角形的叔伯三角形（即和i号三角形的父亲相邻的三角形）
        for (list<int>::iterator it = face_adjacent_table_[ori_idx].begin();
             it != face_adjacent_table_[ori_idx].end(); ++it) {
            //cout << "\t叔伯 = " << *it << endl;
            // 遍历该叔叔所有的儿子三角形（称为i号三角形的堂兄弟三角形）
            for (list<int>::iterator it2 = face_children_table_[*it].begin();
                 it2 != face_children_table_[*it].end(); ++it2) {
                int result;
                //cout << "\t\t叔伯的儿子 = " << *it2 << endl;
                if ((result = triangularAdjacent(i, *it2)) >= 0) {
                    //cout << "\t\t相邻" << endl;
                    int idx0 = result >> 4;
                    int idx1 = result & 0xF;
                    triangle_adjacent_table_[i * 3 + idx0] = *it2 << 2 | idx1;
                    //cout << "face_idx = " << *it2 << " " << idx0 << " " << idx1
                    //<< " " << i * 3 + idx0 << "=" << triangle_adjacent_table_[i * 3 + idx0] << endl;
                }
            }
        }
    }
}

void CommonData::checkStrangeTriangle() {
    triangleList.erase(
            std::remove_if(
                    triangleList.begin(),
                    triangleList.end(),
                    [](Triangle t) {
                        // Do "some stuff", then return true if element should be removed.
                        return random() % 2 == 0;
                    }
            ),
            triangleList.end()
    );
}
