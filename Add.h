#ifndef ADD_H
#define ADD_H
#include <QMainWindow>
#include <QCloseEvent>

namespace Ui {
class Add;
}

class Add : public QMainWindow
{
    Q_OBJECT
public:
    explicit Add(QWidget *parent = nullptr);
    ~Add();
signals:
    void dataAdded();  // 添加数据成功的信号
    void addPathRequested(const QString &start, const QString &end,
                          double distance, double cost, double time);
protected:
    // 声明 closeEvent 函数
    void closeEvent(QCloseEvent *event) override;
private slots:
    void on_pushButton_cancel_clicked();
    void on_pushButton_add_clicked();
private:
    Ui::Add *ui;
};

#endif // ADD_H
