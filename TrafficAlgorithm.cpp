#include "TrafficAlgorithm.h"
#include <QDebug>
#include <stack>
#include <algorithm>
// 全局无穷大常量（解决类内inf私有/非静态的访问冲突）
const int INF = 0x3f3f3f3f;
// 线段树节点结构
struct Node {
    int distance;  // 距离值
    int position;  // 节点位置
    Node() : distance(INF), position(-1) {}
    Node(int d, int p) : distance(d), position(p) {}
};
// 线段树类实现
class SegmentTree {
private:
    QVector<Node> tree;
    int n;

    // 合并两个节点（取距离较小的）
    Node merge(const Node& a, const Node& b) {
        if (a.distance < b.distance) return a;
        return b;
    }

    // 构建线段树
    void build(int node, int start, int end, const QVector<int>& dis) {
        if (start == end) {
            tree[node] = Node(dis[start], start);
            return;
        }
        int mid = (start + end) / 2;
        build(2*node, start, mid, dis);
        build(2*node+1, mid+1, end, dis);
        tree[node] = merge(tree[2*node], tree[2*node+1]);
    }

    // 更新线段树
    void update(int node, int start, int end, int pos, const Node& val) {
        if (start == end) {
            tree[node] = val;
            return;
        }
        int mid = (start + end) / 2;
        if (pos <= mid) {
            update(2*node, start, mid, pos, val);
        } else {
            update(2*node+1, mid+1, end, pos, val);
        }
        tree[node] = merge(tree[2*node], tree[2*node+1]);
    }
    // 查询最小值节点
    Node query(int node, int start, int end) {
        if (start == end) {
            return tree[node];
        }
        int mid = (start + end) / 2;
        Node left = query(2*node, start, mid);
        Node right = query(2*node+1, mid+1, end);
        return merge(left, right);
    }
public:
    SegmentTree(int size, const QVector<int>& dis) {
        n = size;
        tree.resize(4 * (n + 1));  // 分配足够空间
        build(1, 1, n, dis);
    }
    void update(int pos, const Node& val) {
        update(1, 1, n, pos, val);
    }
    Node query() {
        return query(1, 1, n);
    }
};
TrafficAlgorithm::TrafficAlgorithm(QObject *parent)
    : QObject(parent)
    , m_graph(nullptr)
    , m_cityCount(0)
{
}
TrafficAlgorithm::~TrafficAlgorithm()
{
    if (m_graph) {
        delete m_graph;
    }
}
// 辅助函数1：统一城市名格式
static QString formatCityName(const QString &city) {
    if (city.isEmpty()) return "";
    QString lowerCity = city.toLower();  // 全转为小写
    lowerCity[0] = lowerCity[0].toUpper();// 首字母大写
    return lowerCity;
}
// 辅助函数2：判断城市名是否仅包含字母（替代 QRegularExpression）
static bool isCityNameValid(const QString &city) {
    if (city.isEmpty()) return false;
    // 遍历每个字符，判断是否为字母（A-Z/a-z）
    for (int i = 0; i < city.size(); ++i) {
        QChar c = city.at(i);
        if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))) {
            return false; // 包含非字母字符，无效
        }
    }
    return true;
}
// 获取城市ID
int TrafficAlgorithm::getCityId(const QString &cityName) {
    QString formattedCity = formatCityName(cityName);
    if (m_cityMap.contains(formattedCity)) {
        return m_cityMap[formattedCity];
    }
    return -1; // 城市不存在
}
// 确保城市被添加到系统
void TrafficAlgorithm::ensureCityAdded(const QString &cityName) {
    QString formattedCity = formatCityName(cityName);
    // 仅添加“有效且未存在”的城市
    if (isCityNameValid(formattedCity) && !m_cityMap.contains(formattedCity)) {
        int newId = ++m_cityCount;
        m_cityMap[formattedCity] = newId;
        // 扩展数组容量
        if (newId >= m_cities.size()) {
            m_cities.resize(newId + 10);
        }
        m_cities[newId] = formattedCity; // 存储格式化后的城市名
    }
}
// 从文件加载数据
bool TrafficAlgorithm::loadFromFile(const QString &filePath)
{
    // 清空现有数据，避免残留
    if (m_graph) {
        delete m_graph;
        m_graph = nullptr;
    }
    m_cityMap.clear();
    m_cities.clear();
    m_cityCount = 0;
    QFile file(filePath);
    // 检查文件是否能正常打开
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "[错误] 无法打开文件:" << filePath;
        emit dataLoaded(false);
        return false;
    }
    QTextStream in(&file);
    // 临时存储边数据
    struct TempEdge {
        QString start;
        QString end;
        int distance;
        int cost;
        int time;
    };
    QVector<TempEdge> edges;
    // 逐行读取文件
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty()) continue; // 跳过空行
        // 按空格分割数据（跳过连续空格）
        QStringList parts = line.split(" ", Qt::SkipEmptyParts);
        // 至少需要 5 个字段（起点、终点、距离、费用、时间）
        if (parts.size() < 5) {
            qDebug() << "[跳过] 数据字段不足：" << line;
            continue;
        }
        // 1. 格式化城市名（统一首字母大写）
        QString start = formatCityName(parts[0]);
        QString end = formatCityName(parts[1]);
        // 2. 验证城市名有效性（仅纯字母，替代正则）
        if (!isCityNameValid(start) || !isCityNameValid(end)) {
            qDebug() << "[跳过] 无效城市名（非纯字母）：" << parts[0] << "|" << parts[1];
            continue;
        }
        // 3. 解析数值（支持小数转int，如 54.8 → 54）
        bool ok1, ok2, ok3;
        double distanceDouble = parts[2].toDouble(&ok1);
        double costDouble = parts[3].toDouble(&ok2);
        double timeDouble = parts[4].toDouble(&ok3);
        // 转换为int（若需保留小数，可将 side 结构体中的 wd/wc/wt 改为 double）
        int distance = static_cast<int>(distanceDouble);
        int cost = static_cast<int>(costDouble);
        int time = static_cast<int>(timeDouble);
        // 4. 验证数值有效性（必须为正数）
        if (!ok1 || !ok2 || !ok3) {
            qDebug() << "[跳过] 数值无效：" << line;
            continue;
        }
        if (distance <= 0 || cost <= 0 || time <= 0) {
            qDebug() << "[跳过] 数值需为正数：" << line;
            continue;
        }
        // 5. 添加城市（自动去重）和边
        ensureCityAdded(start);
        ensureCityAdded(end);
        edges.append({start, end, distance, cost, time});
    }
    file.close(); // 关闭文件，释放资源
    // 6. 创建图并添加所有边（仅当有城市时创建）
    if (m_cityCount > 0) {
        m_graph = new Graph(m_cityCount);
        for (const auto &edge : edges) {
            int startId = m_cityMap[edge.start];
            int endId = m_cityMap[edge.end];
            m_graph->add_path(startId, endId, edge.distance, edge.cost, edge.time);
        }
    }
    // 输出加载结果（调试用）
    qDebug() << "[加载完成] 城市数量:" << m_cityCount << "，边数量:" << edges.size();
    emit dataLoaded(true);
    return true;
}
// 添加新路径（逻辑不变，确保城市格式统一）
bool TrafficAlgorithm::addPath(const QString &start, const QString &end,
                               double distance, double cost, double time)
{
    // 转换为int（若需小数，需修改 side 结构体）
    int intDistance = static_cast<int>(distance);
    int intCost = static_cast<int>(cost);
    int intTime = static_cast<int>(time);
    // 若图未初始化，先创建
    if (!m_graph) {
        ensureCityAdded(start);
        ensureCityAdded(end);
        m_graph = new Graph(m_cityCount);
    }
    // 确保起点、终点已添加
    ensureCityAdded(start);
    ensureCityAdded(end);
    // 获取格式化后的城市ID
    QString formattedStart = formatCityName(start);
    QString formattedEnd = formatCityName(end);
    int startId = m_cityMap[formattedStart];
    int endId = m_cityMap[formattedEnd];

    // 添加双向路径（道路是双向的）
    m_graph->add_path(startId, endId, intDistance, intCost, intTime);
    emit pathAdded(true);
    return true;
}

// 线段树优化的Dijkstra算法实现
QVector<TrafficAlgorithm::side> TrafficAlgorithm::runDijkstra(int startId, int endId, int option)
{
    QVector<side> resultPath;
    // 边界判断（图未初始化或ID无效）
    if (!m_graph || startId == -1 || endId == -1 || startId > m_cityCount || endId > m_cityCount) {
        return resultPath;
    }
    int n = m_cityCount;
    QVector<int> dis(n + 10, INF);    // 全局INF
    QVector<side> pre(n + 10, {-1, 0, 0, 0}); // 前驱节点（记录路径）
    QVector<bool> visited(n + 10, false); // 标记是否已访问
    dis[startId] = 0; // 起点距离为0
    // 初始化线段树
    SegmentTree st(n, dis);
    // 线段树优化的Dijkstra算法
    for (int i = 1; i < n; ++i) {
        // 1. 从线段树查询未访问的最短距离节点
        Node temp = st.query();
        if (temp.position == -1 || temp.distance == INF) break; // 改用全局INF
        int minPos = temp.position;
        if (visited[minPos]) continue;
        visited[minPos] = true;  // 标记为已访问
        // 2. 提前结束（已找到终点）
        if (minPos == endId) break;
        // 3. 更新该节点在segTree中的值为INF（已访问）
        st.update(minPos, Node(INF, minPos)); // 改用全局INF
        // 4. 松弛操作（更新邻接节点距离）
        const QVector<side>& edges = m_graph->get_edges(minPos);
        for (const side &to : edges) {
            int weight = to.get_val(option); // 根据选项（距离/费用/时间）取权重
            if (dis[to.u] > dis[minPos] + weight) {
                dis[to.u] = dis[minPos] + weight;
                pre[to.u] = {minPos, to.wd, to.wc, to.wt}; // 记录前驱
                // 更新线段树中的距离值
                st.update(to.u, Node(dis[to.u], to.u));
            }
        }
    }
    // 5. 路径回溯（从终点到起点）
    if (dis[endId] == INF) { // 改用全局INF
        return resultPath; // 无路径
    }
    std::stack<side> pathStack;
    for (int i = endId; i != startId; i = pre[i].u) {
        pathStack.push({i, pre[i].wd, pre[i].wc, pre[i].wt});
    }

    // 6. 转换为向量（从起点到终点）
    while (!pathStack.empty()) {
        resultPath.append(pathStack.top());
        pathStack.pop();
    }

    return resultPath;
}
// 查找最短路径（修复累计值计算和节点关联）
QVector<TrafficAlgorithm::PathResult> TrafficAlgorithm::findShortestPath(
    const QString &start, const QString &end, int option)
{
    QVector<PathResult> results;
    // 获取格式化后的城市ID（确保大小写匹配）
    int startId = getCityId(start);
    int endId = getCityId(end);

    // 边界判断（城市不存在或图未初始化）
    if (startId == -1 || endId == -1 || !m_graph) {
        qDebug() << "[查询失败] 城市不存在或图未初始化:" << start << "→" << end;
        return results;
    }

    // 1. 执行Dijkstra算法，获取路径边集合（从起点到终点的边）
    QVector<side> pathEdges = runDijkstra(startId, endId, option);
    if (pathEdges.isEmpty()) {
        qDebug() << "[查询失败] 无路径:" << start << "→" << end;
        return results;
    }

    // 2. 初始化起点（首行数据）
    PathResult startResult;
    startResult.city = formatCityName(start); // 统一起点格式
    // 起点的累计值为0，但需关联第一段路径的权重
    if (!pathEdges.isEmpty()) {
        const side &firstEdge = pathEdges.first();
        startResult.distance = static_cast<double>(firstEdge.wd);
        startResult.cost = static_cast<double>(firstEdge.wc);
        startResult.time = static_cast<double>(firstEdge.wt);
    } else {
        startResult.distance = 0.0;
        startResult.cost = 0.0;
        startResult.time = 0.0;
    }
    results.append(startResult);
    // 3. 遍历路径边，计算累计值并添加后续节点（修复中间行数值和终点显示）
    double totalDist = startResult.distance;
    double totalCost = startResult.cost;
    double totalTime = startResult.time;

    // 定义并初始化formattedEnd（确保和城市名格式统一）
    QString formattedEnd = formatCityName(end); // 调用formatCityName统一格式（和startResult.city保持一致）

    for (int i = 0; i < pathEdges.size(); ++i) {
        const side &currEdge = pathEdges[i];
        // 获取当前边的终点城市名（从ID映射表中取）
        QString currCity = m_cities[currEdge.u];
        if (currCity.isEmpty()) {
            qDebug() << "[警告] 无效城市ID:" << currEdge.u;
            continue;
        }
        // 确保currCity的格式和formattedEnd一致（调用formatCityName）
        currCity = formatCityName(currCity);

        // 判断当前城市是否为目标终点，若已到达则不追加
        if (currCity == formattedEnd) {
            break; // 停止遍历，避免生成冗余的"终点行"
        }

        // 累加后续路径的权重
        if (i < pathEdges.size() - 1) {
            const side &nextEdge = pathEdges[i + 1];
            totalDist += static_cast<double>(nextEdge.wd);
            totalCost += static_cast<double>(nextEdge.wc);
            totalTime += static_cast<double>(nextEdge.wt);
        }

        // 构造当前节点的结果（对应表格中的每一行）
        PathResult currResult;
        currResult.city = currCity;
        currResult.distance = totalDist;
        currResult.cost = totalCost;
        currResult.time = totalTime;
        results.append(currResult);
    }

    return results;
}
// 获取所有城市
QVector<QString> TrafficAlgorithm::getAllCities() const
{
    QVector<QString> cities;
    // 从映射中获取所有城市名
    for (const auto &city : m_cityMap.keys()) {
        cities.append(city);
    }

    // 排序（不区分大小写，按字母顺序）
    std::sort(cities.begin(), cities.end(), [](const QString &a, const QString &b) {
        return a.compare(b, Qt::CaseInsensitive) < 0;
    });

    return cities;
}
// 检查城市是否存在（支持大小写不敏感）
bool TrafficAlgorithm::cityExists(const QString &city) const
{
    QString formattedCity = formatCityName(city);
    return m_cityMap.contains(formattedCity);
}

// 工具函数：int转QString（
QString TrafficAlgorithm::intToString(int x)
{
    return QString::number(x);
}
