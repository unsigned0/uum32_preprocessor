#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWidget>
#include <QFileDialog>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QDebug>
#include <QString>
#include <QRegExp>
#include <QStringList>

namespace Ui {
    class MainWindow;
}

class MainWindow : public QWidget
{
    Q_OBJECT
private:
    QFileDialog *fileRequester;
    QStringList outLst;
    QStringList nameTable;

    //QString().isEmpty = true -> ошибок нет, иначе содержит сообщение об ошибке
    QString parseLine(QString &line);         // Проверка строки на наличие ключ. слов
    QString plugModule(const QString &path);  // Подключить модуль к исходному коду
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
private slots:
    void fileSelected();
    void handle();
};

#endif // MAINWINDOW_H
