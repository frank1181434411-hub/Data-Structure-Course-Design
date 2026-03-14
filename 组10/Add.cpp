#include "Add.h"
#include "ui_Add.h"
#include <MainWindow.h>
#include <QFile>
#include <QCloseEvent>
#include <QTextStream>
#include <QMessageBox>
#include <QRegularExpression>
#include <QTimer>
#include <QPalette>
#include <QDebug>

Add::Add(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Add)
{
    ui->setupUi(this);
    // 初始隐藏消息标签
    ui->label_tip->setVisible(false);
    connect(ui->pushButton_Cancel, &QPushButton::clicked,
            this, &Add::on_pushButton_cancel_clicked);
    connect(ui->pushButton_add, &QPushButton::clicked,
            this, &Add::on_pushButton_add_clicked);
}

Add::~Add()
{
    delete ui;
}

// 重载 closeEvent，当关闭 Add 窗口时重新显示主界面
void Add::closeEvent(QCloseEvent *event)
{
    // 获取主窗口指针并显示
    MainWindow *mainWindow = qobject_cast<MainWindow *>(parent());
    if (mainWindow) {
        mainWindow->show();  // 重新显示主界面
    }
    // 调用父类的 closeEvent 以确保窗口正常关闭
    event->accept();
}

void Add::on_pushButton_cancel_clicked()
{
    this->close();
}

void Add::on_pushButton_add_clicked()
{
    ui->label_tip->setVisible(false);
    // 获取用户输入的内容
    QString startPoint = ui->lineEdit_StartPoint->text().trimmed();  // 起点
    QString endPoint = ui->lineEdit_EndPoint->text().trimmed();      // 终点
    QString distance = ui->lineEdit_Distance->text().trimmed();      // 距离
    QString time = ui->lineEdit_Time->text().trimmed();              // 时间
    QString cost = ui->lineEdit_Cost->text().trimmed();              // 费用
    // 输入验证
    if (startPoint.isEmpty() || endPoint.isEmpty() || distance.isEmpty() || time.isEmpty() || cost.isEmpty()) {
        ui->label_tip->setText("请完整填写所有字段！");
        ui->label_tip->setStyleSheet("color: red; font-weight: bold;");
        ui->label_tip->setVisible(true);
        // 2秒后自动隐藏错误消息
        QTimer::singleShot(2000, this, [this]() {
            ui->label_tip->setVisible(false);
        });
        return;
    }
    // 验证起点和终点为字母
    QRegularExpression cityRegex("^[A-Za-z]+$");
    if (!cityRegex.match(startPoint).hasMatch() || !cityRegex.match(endPoint).hasMatch()) {
        ui->label_tip->setText("起点和终点必须是英文城市名！");
        ui->label_tip->setStyleSheet("color: red; font-weight: bold;");
        ui->label_tip->setVisible(true);
        QTimer::singleShot(2000, this, [this]() {
            ui->label_tip->setVisible(false);
        });
        return;
    }
    // 验证距离、时间、费用为数字
    bool validDistance, validTime, validCost;
    double distanceValue = distance.toDouble(&validDistance);
    double timeValue = time.toDouble(&validTime);
    double costValue = cost.toDouble(&validCost);
    if (!validDistance || !validTime || !validCost) {
        // 使用label_tip显示错误，而不是QMessageBox
        ui->label_tip->setText("距离、时间和费用必须是数字！");
        ui->label_tip->setStyleSheet("color: red; font-weight: bold;");
        ui->label_tip->setVisible(true);
        QTimer::singleShot(2000, this, [this]() {
            ui->label_tip->setVisible(false);
        });
        return;
    }
    // 检查是否为正数
    if (distanceValue <= 0 || timeValue <= 0 || costValue <= 0) {
        ui->label_tip->setText("距离、时间和费用必须为正数！");
        ui->label_tip->setStyleSheet("color: red; font-weight: bold;");
        ui->label_tip->setVisible(true);
        QTimer::singleShot(2000, this, [this]() {
            ui->label_tip->setVisible(false);
        });
        return;
    }
    // 检查起点和终点是否相同（大小写不敏感）
    if (startPoint.compare(endPoint, Qt::CaseInsensitive) == 0) {
        ui->label_tip->setText("起点和终点不能相同！");
        ui->label_tip->setStyleSheet("color: red; font-weight: bold;");
        ui->label_tip->setVisible(true);
        QTimer::singleShot(2000, this, [this]() {
            ui->label_tip->setVisible(false);
        });
        return;
    }
    // 格式化起点和终点（统一为首字母大写，其余小写）
    auto formatCity = [](const QString &city) -> QString {
        if (city.isEmpty()) return "";
        QString lowerCity = city.toLower();
        lowerCity[0] = lowerCity[0].toUpper();
        return lowerCity;
    };
    QString formattedStart = formatCity(startPoint);
    QString formattedEnd = formatCity(endPoint);
    // 检查文件中的路径是否已存在（大小写不敏感）
    QFile file("D:/QtProject/Traffic_Inquiry_System/traffic_sample.txt");
    if (file.exists()) {
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            bool pathExists = false;
            while (!in.atEnd()) {
                QString line = in.readLine().trimmed();
                if (line.isEmpty()) continue;
                QStringList parts = line.split(" ", Qt::SkipEmptyParts);
                if (parts.size() >= 2) {
                    QString existingStart = formatCity(parts[0]);
                    QString existingEnd = formatCity(parts[1]);
                    // 检查路径是否已存在（双向检查：A-B 和 B-A 视为相同路径）
                    //情况1：输入的“起点-终点”和文件中的“起点-终点”完全一致
                    // 情况2：输入的“起点-终点”和文件中的“终点-起点”一致（双向视为同路径）
                    if ((existingStart == formattedStart && existingEnd == formattedEnd) ||
                        (existingStart == formattedEnd && existingEnd == formattedStart)) {
                        pathExists = true;
                        break;
                    }
                }
            }
            file.close();
            if (pathExists) {
                ui->label_tip->setText("路径已存在！");
                ui->label_tip->setStyleSheet("color: red; font-weight: bold;");
                ui->label_tip->setVisible(true);
                QTimer::singleShot(2000, this, [this]() {
                    ui->label_tip->setVisible(false);
                });
                return;
            }
        }
    }
    // 打开文件进行写入
    if (!file.open(QIODevice::Append | QIODevice::Text)) {
        ui->label_tip->setText("无法打开文件进行写入！");
        ui->label_tip->setStyleSheet("color: red; font-weight: bold;");
        ui->label_tip->setVisible(true);
        QTimer::singleShot(2000, this, [this]() {
            ui->label_tip->setVisible(false);
        });
        return;
    }

    // 处理文件末尾换行
    QTextStream out(&file);
    // 检查文件当前是否为空，如果不为空则先移动到末尾并检查最后一个字符是否为换行
    if (file.size() > 0) {
        file.seek(file.size() - 1);
        char lastChar;
        file.read(&lastChar, 1);
        if (lastChar != '\n') {
            out << "\n"; // 如果最后不是换行，添加一个换行
        }
    }
    // 发射添加路径请求信号（使用格式化后的城市名）
    emit addPathRequested(formattedStart, formattedEnd, distanceValue, costValue, timeValue);
    // 写入文件（使用格式化后的城市名，确保文件中格式统一）
    out << formattedStart << " " << formattedEnd << " "
        << QString::number(distanceValue, 'f', 2) << " "
        << QString::number(costValue, 'f', 2) << " "
        << QString::number(timeValue, 'f', 2) << "\n";
    file.close();
    ui->label_tip->setText("数据已经保存");
    ui->label_tip->setStyleSheet("color: green; font-weight: bold;");
    ui->label_tip->setVisible(true);
    // 保存成功后发射信号
    emit dataAdded();
    // 直接关闭窗口，无需延迟
    this->close();
    // 清空输入框内容
    ui->lineEdit_StartPoint->clear();
    ui->lineEdit_EndPoint->clear();
    ui->lineEdit_Distance->clear();
    ui->lineEdit_Time->clear();
    ui->lineEdit_Cost->clear();
}
