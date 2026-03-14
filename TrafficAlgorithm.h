#ifndef TRAFFICALGORITHM_H
#define TRAFFICALGORITHM_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QMap>
#include <QFile>
#include <QTextStream>

class TrafficAlgorithm : public QObject
{
    Q_OBJECT

public:
    // 路径结果结构
    struct PathResult {
        QString city;
        double distance;
        double cost;
        double time;
    };

    TrafficAlgorithm(QObject *parent = nullptr);
    ~TrafficAlgorithm();

    // 从文件加载数据
    bool loadFromFile(const QString &filePath);

    // 添加新路径
    bool addPath(const QString &start, const QString &end,
                 double distance, double cost, double time);

    // 查询最短路径
    QVector<PathResult> findShortestPath(const QString &start,
                                         const QString &end,
                                         int option); // 1=距离, 2=费用, 3=时间

    // 获取所有城市
    QVector<QString> getAllCities() const;

    // 检查城市是否存在
    bool cityExists(const QString &city) const;

signals:
    void dataLoaded(bool success);
    void pathAdded(bool success);

private:

    const int inf = 1000000000;

    // side结构体
    struct side {
        int u;//顶点
        int wd;//距离
        int wc;//花费
        int wt;//时间
        int get_val(int opt) const{
            if (opt == 1) return wd;
            else if (opt == 2) return wc;
            else return wt;
        }
    };

    // graph类 - 直接从原始文件复制，但改为Qt风格命名
    class Graph {
    private:
        int n;
        QVector<QVector<side>> s;  // 使用QVector替代std::vector

    public:
        Graph() { n = 0; }
        Graph(int x) {
            n = x;
            s.resize(n + 10);
        }

        ~Graph() {
            for (int i = 0; i <= n; ++i)
                s[i].clear();
            s.clear();
        }

        void add_path(int x, int y, int dis, int cst, int tme) {
            s[x].push_back(side{y, dis, cst, tme});
            s[y].push_back(side{x, dis, cst, tme});
        }

        QVector<side>& get_edges(int x) { return s[x]; }
    };

    // 算法核心数据结构
    Graph *m_graph;
    QMap<QString, int> m_cityMap;      // 城市名到ID的映射
    QVector<QString> m_cities;         // ID到城市名的映射
    int m_cityCount;                   // 城市数量

    // 辅助函数
    int getCityId(const QString &cityName);
    void ensureCityAdded(const QString &cityName);

    // Dijkstra算法实现 - 基于lab.cpp移植
    QVector<side> runDijkstra(int startId, int endId, int option);

    // 工具函数
    QString intToString(int x);
};
#endif // TRAFFICALGORITHM_H
