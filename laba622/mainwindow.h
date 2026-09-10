#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStandardItemModel>

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
    void on_loadButton_clicked();
    void on_saveValidButton_clicked();
    void on_saveInvalidButton_clicked();

private:
    Ui::MainWindow *ui;

    QStandardItemModel *validModel;
    QStandardItemModel *invalidModel;

    void setupTables();
    void loadAndValidateData(const QString &filePath);
    bool validateItem(const QJsonObject &item, QString &errorMessage);
    void addToTable(QStandardItemModel *model, const QJsonObject &item, const QString &errorMessage = "");
};

#endif
