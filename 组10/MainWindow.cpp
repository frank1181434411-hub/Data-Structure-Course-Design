#include "MainWindow.h"
#include "ui_MainWindow.h"
#include <QTextStream>
#include <QSet>
#include <QList>
#include <QMessageBox>
#include <QTableWidgetItem>
#include <QComboBox>
#include <QRegularExpression>
#include <QMessageBox>

// 主窗口构造函数
// 功能：初始化UI，创建算法实例，连接信号与槽，设置查询方式下拉框和表格，加载数据
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_algorithm(new TrafficAlgorithm(this))
{
    ui->setupUi(this);

    // 连接信号
    connect(m_algorithm, &TrafficAlgorithm::dataLoaded,
            this, &MainWindow::onDataLoaded);
    connect(m_algorithm, &TrafficAlgorithm::pathAdded,
            this, &MainWindow::onPathAdded);

    // 设置查询方式下拉框 - 修复初始设置
    ui->comboBox_Option->clear();
    ui->comboBox_Option->addItem("最短路径", 1);    // 假设0表示最短路径
    ui->comboBox_Option->addItem("最少交通费用", 2); // 假设1表示最少费用
    ui->comboBox_Option->addItem("最短时间", 3);     // 假设2表示最短时间

    // 设置表格
    // 设置表格（保持原样，因为UI已经定义）
    ui->tableWidget_Result->horizontalHeader()->setStretchLastSection(true);
    reloadData();
}

// 主窗口析构函数
// 功能：释放UI资源
MainWindow::~MainWindow()
{
    delete ui;
}

// "添加"按钮点击事件处理函数
// 功能：隐藏主窗口，创建并显示添加路径窗口，连接添加窗口的信号与主窗口的槽函数
void MainWindow::on_pushButton_Add_clicked()
{
    this->hide();
    Add* addWindow = new Add(this); // 创建 Add 窗口对象
    addWindow->show(); // 显示 Add 窗口

    // 连接 Add 窗口的关闭信号，重新显示主界面
    connect(addWindow, &Add::destroyed, this, &MainWindow::show);
    // 保留旧的dataAdded信号连接，用于兼容
    connect(addWindow, &Add::dataAdded, this, &MainWindow::reloadData);
    // 连接添加路径请求信号
    connect(addWindow, &Add::addPathRequested, this, &MainWindow::handleAddPathRequest);
}

// 从数据文件中获取所有唯一城市名
// 功能：读取数据文件，提取起点和终点城市，统一格式（首字母大写，其余小写）并去重，验证城市名格式（仅字母）
// 返回值：包含所有唯一城市名的QSet集合
QSet<QString> MainWindow::getUniqueCities()
{
    QSet<QString> cities; // 存储最终去重后的城市（首字母大写格式）
    QSet<QString> lowerCities; // 存储小写格式，用于去重判断
    // 打开文件
    QFile file("D:/QtProject/Traffic_Inquiry_System/traffic_sample.txt");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "文件打开失败", "无法读取城市数据文件！");
        return cities;
    }
    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine();
        QStringList parts = line.split(" ", Qt::SkipEmptyParts);
        if (parts.size() >= 2) {
            // 处理每个城市：统一转为首字母大写，其余小写
            auto formatCity = [](const QString &city) -> QString {
                if (city.isEmpty()) return "";
                QString lowerCity = city.toLower();
                lowerCity[0] = lowerCity[0].toUpper();
                return lowerCity;
            };
            QString startCity = formatCity(parts[0]);
            QString endCity = formatCity(parts[1]);
            // 验证城市名格式（仅字母）
            QRegularExpression cityRegex("^[A-Za-z]+$");
            // 去重判断：通过小写格式判断是否已存在
            if (cityRegex.match(startCity).hasMatch() && !lowerCities.contains(startCity.toLower())) {
                lowerCities.insert(startCity.toLower());
                cities.insert(startCity);
            }
            if (cityRegex.match(endCity).hasMatch() && !lowerCities.contains(endCity.toLower())) {
                lowerCities.insert(endCity.toLower());
                cities.insert(endCity);
            }
        }
    }
    file.close();
    return cities;
}

// 重新加载数据
// 功能：从指定路径的文件加载数据到算法类，加载成功则刷新城市下拉框，失败则显示错误提示
void MainWindow::reloadData()
{
    QString filePath = "D:/QtProject/Traffic_Inquiry_System/traffic_sample.txt";
    if (m_algorithm->loadFromFile(filePath)) {  // 注意这里修改了判断条件
        refreshCityComboBoxes();  // 数据加载成功后刷新下拉框
    }
    else {
        QMessageBox::warning(this, "错误", "无法加载数据文件！");
    }
}

// "查询"按钮点击事件处理函数
// 功能：获取用户选择的起点、终点和查询方式，验证输入合法性，执行路径查询并显示结果
void MainWindow::on_pushButton_Search_clicked()
{
    // 获取选择的起点、终点和查询方式
    QString start = ui->comboBox_StartPoint->currentText();
    QString end = ui->comboBox_EndPoint->currentText();
    int option = ui->comboBox_Option->currentData().toInt();

    // 验证输入
    if (start == "请选择..." || end == "请选择..." ||
        start.isEmpty() || end.isEmpty()) {
        QMessageBox::warning(this, "输入错误", "请选择起点和终点！");
        return;
    }

    if (start == end) {
        QMessageBox::warning(this, "输入错误", "起点和终点不能相同！");
        return;
    }

    // 清空表格
    clearTable();

    // 显示等待提示
    QApplication::processEvents();

    // 执行查询
    QVector<TrafficAlgorithm::PathResult> results =
        m_algorithm->findShortestPath(start, end, option);

    // 显示结果
    if (results.isEmpty()) {

        QMessageBox::information(this, "查询结果",
                                 "没有找到从 " + start + " 到 " + end + " 的路径！");
    } else {
        displayPathResult(results);


        // 显示总统计
        if (!results.isEmpty()) {
            const auto &last = results.last();
            QString totalInfo = QString("总距离: %1 | 总费用: %2 | 总时间: %3")
                                    .arg(last.distance)
                                    .arg(last.cost)
                                    .arg(last.time);

        }
    }
}

// 显示路径查询结果
// 功能：将查询得到的路径结果填充到表格中，包括各段路径的起点、终点、距离、费用、时间，以及总计信息
// 参数：results - 包含路径查询结果的向量
void MainWindow::displayPathResult(const QVector<TrafficAlgorithm::PathResult> &results)
{
    if (results.isEmpty()) return;

    // 获取用户查询的起点和终点
    QString queryStart = ui->comboBox_StartPoint->currentText();
    QString queryEnd = ui->comboBox_EndPoint->currentText();

    // 设置表格行数：每段路径一行 + 最后的总计一行
    ui->tableWidget_Result->setRowCount(results.size());
    QStringList headers = {"起点", "终点", "距离/km", "费用/¥", "时间/h"};
    ui->tableWidget_Result->setHorizontalHeaderLabels(headers);

    // 遍历每段路径，显示独立信息
    for (int i = 0; i < results.size(); ++i) {
        const auto &result = results[i];

        // 起点列：当前节点
        QTableWidgetItem *startItem = new QTableWidgetItem(result.city);
        startItem->setTextAlignment(Qt::AlignCenter);
        startItem->setFlags(startItem->flags() & ~Qt::ItemIsEditable);

        // 终点列
        QString endCity;
        if (i < results.size() - 1) {
            // 如果不是最后一段路径，终点是下一段的起点
            endCity = results[i + 1].city;
        } else {
            // 最后一段路径，终点是查询的终点
            endCity = queryEnd;
        }
        QTableWidgetItem *endItem = new QTableWidgetItem(endCity);
        endItem->setTextAlignment(Qt::AlignCenter);
        endItem->setFlags(endItem->flags() & ~Qt::ItemIsEditable);

        // 计算当前段的距离、费用、时间
        double segmentDist, segmentCost, segmentTime;

        if (i == 0) {
            // 第一段：直接用结果中的值
            segmentDist = result.distance;
            segmentCost = result.cost;
            segmentTime = result.time;
        } else {
            // 后续段：用当前结果减去前一段结果
            const auto &prevResult = results[i - 1];
            segmentDist = result.distance - prevResult.distance;
            segmentCost = result.cost - prevResult.cost;
            segmentTime = result.time - prevResult.time;
        }

        // 设置数值（显示每段的独立值）
        QString distText = QString::number(segmentDist, 'f', 2);
        QString costText = QString::number(segmentCost, 'f', 2);
        QString timeText = QString::number(segmentTime, 'f', 2);

        QTableWidgetItem *distItem = new QTableWidgetItem(distText);
        QTableWidgetItem *costItem = new QTableWidgetItem(costText);
        QTableWidgetItem *timeItem = new QTableWidgetItem(timeText);

        // 设置对齐和禁止编辑
        distItem->setTextAlignment(Qt::AlignCenter);
        costItem->setTextAlignment(Qt::AlignCenter);
        timeItem->setTextAlignment(Qt::AlignCenter);
        distItem->setFlags(distItem->flags() & ~Qt::ItemIsEditable);
        costItem->setFlags(costItem->flags() & ~Qt::ItemIsEditable);
        timeItem->setFlags(timeItem->flags() & ~Qt::ItemIsEditable);

        // 添加到表格
        ui->tableWidget_Result->setItem(i, 0, startItem);
        ui->tableWidget_Result->setItem(i, 1, endItem);
        ui->tableWidget_Result->setItem(i, 2, distItem);
        ui->tableWidget_Result->setItem(i, 3, costItem);
        ui->tableWidget_Result->setItem(i, 4, timeItem);
    }

    // 如果是最后一段路径的显示，需要特殊处理
    if (!results.isEmpty()) {
        const auto &lastResult = results.last();

        // 更新最后一段的终点为查询的终点
        QTableWidgetItem *lastEndItem = ui->tableWidget_Result->item(results.size() - 1, 1);
        if (lastEndItem) {
            lastEndItem->setText(queryEnd);
        }

        // 在最后一行下方添加总计行
        int totalRow = results.size();
        ui->tableWidget_Result->setRowCount(totalRow + 1);

        // 总计行的起点和终点

        QTableWidgetItem *totalStartItem = new QTableWidgetItem(queryStart);
        QTableWidgetItem *totalEndItem = new QTableWidgetItem(queryEnd);

        // 设置加粗字体
        QFont boldFont;
        boldFont.setBold(true);
        totalStartItem->setFont(boldFont);
        totalEndItem->setFont(boldFont);
        totalStartItem->setTextAlignment(Qt::AlignCenter);
        totalEndItem->setTextAlignment(Qt::AlignCenter);
        totalStartItem->setFlags(totalStartItem->flags() & ~Qt::ItemIsEditable);
        totalEndItem->setFlags(totalEndItem->flags() & ~Qt::ItemIsEditable);

        // 总计行的数值（累计值）
        QString totalDistText = QString("累计%1").arg(QString::number(lastResult.distance, 'f', 2));
        QString totalCostText = QString("累计%1").arg(QString::number(lastResult.cost, 'f', 2));
        QString totalTimeText = QString("累计%1").arg(QString::number(lastResult.time, 'f', 2));

        QTableWidgetItem *totalDistItem = new QTableWidgetItem(totalDistText);
        QTableWidgetItem *totalCostItem = new QTableWidgetItem(totalCostText);
        QTableWidgetItem *totalTimeItem = new QTableWidgetItem(totalTimeText);

        // 设置加粗和对齐
        totalDistItem->setFont(boldFont);
        totalCostItem->setFont(boldFont);
        totalTimeItem->setFont(boldFont);
        totalDistItem->setTextAlignment(Qt::AlignCenter);
        totalCostItem->setTextAlignment(Qt::AlignCenter);
        totalTimeItem->setTextAlignment(Qt::AlignCenter);
        totalDistItem->setFlags(totalDistItem->flags() & ~Qt::ItemIsEditable);
        totalCostItem->setFlags(totalCostItem->flags() & ~Qt::ItemIsEditable);
        totalTimeItem->setFlags(totalTimeItem->flags() & ~Qt::ItemIsEditable);

        // 添加到总计行
        ui->tableWidget_Result->setItem(totalRow, 0, totalStartItem);
        ui->tableWidget_Result->setItem(totalRow, 1, totalEndItem);
        ui->tableWidget_Result->setItem(totalRow, 2, totalDistItem);
        ui->tableWidget_Result->setItem(totalRow, 3, totalCostItem);
        ui->tableWidget_Result->setItem(totalRow, 4, totalTimeItem);
    }

    ui->tableWidget_Result->resizeColumnsToContents();
}

// 清空结果表格
// 功能：将结果表格的行数设置为0，清除所有内容
void MainWindow::clearTable()
{
    ui->tableWidget_Result->setRowCount(0);
}

// 数据加载完成信号的槽函数
// 功能：根据数据加载是否成功，输出调试信息或显示错误提示
// 参数：success - 数据加载是否成功的标志
void MainWindow::onDataLoaded(bool success)
{
    if (success) {
        qDebug() << "数据加载成功";
    } else {
        QMessageBox::warning(this, "错误", "数据加载失败！");
    }
}

// 路径添加完成信号的槽函数
// 功能：如果路径添加成功，则刷新城市下拉框
// 参数：success - 路径添加是否成功的标志
void MainWindow::onPathAdded(bool success)
{
    if (success) {
        refreshCityComboBoxes();  // 路径添加成功后刷新下拉框
    }
}

// 处理添加路径请求
// 功能：调用算法类添加路径，添加成功则将路径信息写入数据文件，重新加载数据并刷新城市下拉框
// 入口：当Add窗口触发addPathRequested信号时被调用
void MainWindow::handleAddPathRequest(const QString &start, const QString &end,
                                      double distance, double cost, double time)
{
    if (m_algorithm->addPath(start, end, distance, cost, time)) {
        QFile file("D:/QtProject/Traffic_Inquiry_System/traffic_sample.txt");
        if (file.open(QIODevice::Append | QIODevice::Text)) {
            QTextStream out(&file);
            out << start << " " << end << " "
                << QString::number(distance, 'f', 2) << " "
                << QString::number(cost, 'f', 2) << " "
                << QString::number(time, 'f', 2) << "\n";
            file.close();
            qDebug() << "路径已保存到文件：" << start << "->" << end;
        }
        reloadData();  // 重新加载数据
        refreshCityComboBoxes();  // 强制刷新下拉框
    } else {
        qDebug() << "路径添加失败：" << start << "->" << end;
    }
}

// 刷新城市下拉框
// 功能：清空起点和终点下拉框，添加提示项，从算法类获取所有城市并添加到下拉框中
void MainWindow::refreshCityComboBoxes()
{
    ui->comboBox_StartPoint->clear();
    ui->comboBox_EndPoint->clear();
    ui->comboBox_StartPoint->addItem("请选择...");
    ui->comboBox_EndPoint->addItem("请选择...");

    // 从算法获取已去重、统一格式的城市列表
    QVector<QString> cities = m_algorithm->getAllCities();

    qDebug() << "下拉框加载城市数量：" << cities.size(); // 调试用，可查看是否获取到城市

    if (cities.isEmpty()) {
        ui->comboBox_StartPoint->addItem("暂无城市数据");
        ui->comboBox_EndPoint->addItem("暂无城市数据");
        return;
    }

    for (const QString &city : cities) {
        ui->comboBox_StartPoint->addItem(city);
        ui->comboBox_EndPoint->addItem(city);
    }
}
