#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "Add.h"
#include "TrafficAlgorithm.h"
#include <QFile>
#include <QTableWidget>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_pushButton_Add_clicked();
    void on_pushButton_Search_clicked();
    void onDataLoaded(bool success);//数据加载完成
    void onPathAdded(bool success);//路径添加完成信号
    void handleAddPathRequest(const QString &start, const QString &end,
                              double distance, double cost, double time);

private:
    Ui::MainWindow *ui;
    TrafficAlgorithm *m_algorithm;
    QSet<QString> getUniqueCities();
    void refreshCityComboBoxes();
    void displayPathResult(const QVector<TrafficAlgorithm::PathResult> &results);
    void clearTable();
    void reloadData();
};

#endif // MAINWINDOW_H
