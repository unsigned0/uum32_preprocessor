#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) : QWidget(parent), nameTable({ "include" }), ui(new Ui::MainWindow)
{
    ui -> setupUi(this);

    ui -> start_btn -> setEnabled(false);

    fileRequester = new QFileDialog(this, QString(), QString(), "Исходный код программы на макроассемблере для УУМ-32 (*.uum32masm)");

    connect(ui -> open_btn,  SIGNAL(clicked()), fileRequester, SLOT(exec()));
    connect(fileRequester,   SIGNAL(accepted()), SLOT(fileSelected()));
    connect(ui -> start_btn, SIGNAL(clicked()), SLOT(handle()));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::fileSelected()
{
    ui -> path_lbl -> setText(fileRequester -> selectedFiles().at(0));
    ui -> start_btn -> setEnabled(true);
}

void MainWindow::handle()
{
    if(fileRequester -> selectedFiles().isEmpty() == true)
    {
        QMessageBox err_msg(windowTitle(), "Предупреждение: файл не выбран", QMessageBox::Warning,
                                                                             QMessageBox::Ok,
                                                                             QMessageBox::NoButton,
                                                                             QMessageBox::NoButton);
        err_msg.exec();
        return;
    }

    QFile masmFile(fileRequester -> selectedFiles().at(0));

    if(!masmFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QMessageBox err_msg(windowTitle(), "Ошибка: файл не найден", QMessageBox::Critical,
                                                                     QMessageBox::Ok,
                                                                     QMessageBox::NoButton,
                                                                     QMessageBox::NoButton);
        err_msg.exec();

        return;
    }

    QTextStream stream(&masmFile);
    QString line;

    for(quint64 str_num = 0; stream.readLineInto(&line); ++str_num)
    {
        QString err_msg = parseLine(line);

        if(!err_msg.isEmpty())
        {
            ui -> textBrowser -> append("Строка [" + QString::number(str_num + 1) + "]: " + err_msg);

            nameTable.clear();
            nameTable.push_back("include");
            outLst.clear();

            masmFile.close();

            return;
        }
    }

    nameTable.clear();
    nameTable.push_back("include");

    masmFile.close();

    QString path_to_handled_file = fileRequester -> selectedFiles().at(0);
    path_to_handled_file.truncate(fileRequester -> selectedFiles().at(0).size() - 9);

    path_to_handled_file += "uum32asm";

    QFile asmFile(path_to_handled_file);

    asmFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate);

    stream.setDevice(&asmFile);
    stream.setCodec("UTF-8");   // Без этой кодировки в файле не будут работать русские буквы

    for(auto source_line: outLst)
    {
        stream << source_line << '\n';
    }
    asmFile.close();

    outLst.clear();
    ui -> textBrowser -> append("Файл успешно сгенерирован");
}


QString MainWindow::parseLine(QString& line)
{
    qint32 comm_pos = 0;
    bool any_keywords = false;

    while(comm_pos < line.size() && line[comm_pos] != ';') // Ищем позициию символа начала комментария ";"
        ++comm_pos;

    if(!comm_pos)            // Если кроме комментария на строке ничего не было - выходим
        return QString();

    line.truncate(comm_pos); // Обрезаем комментарий

    for(quint16 counter = 0; counter < nameTable.size(); ++counter) // Ищем совпадения с ключевыми словами
    {
        QRegExp find_keyword(nameTable[counter]);

        int keyword_pos = find_keyword.indexIn(line);

        if(keyword_pos == -1)
            continue;

        else
        {
            any_keywords = true;
            if(counter == 0) // То есть ключевое слово - include
            {
                for(quint16 i = 0; i != keyword_pos; --i) // Проверка на символы до include
                    if(line[i] != ' ' || line[i] != '\t')
                        return QString("недопустимый символ");

                QString path;

                bool open_quote  = false;   // Была ли открывающая кавычка
                bool close_quote = false;   // Была ли закрывающая кавычка

                keyword_pos += nameTable[counter].size(); // Оказываемся за словом include для нахождения пути подключаемой библиотеки

                for(; keyword_pos < line.size(); ++keyword_pos) // Парсим строку
                {
                    if(open_quote == false)
                    {
                        if(line[keyword_pos] == '\t' || line[keyword_pos] == ' ')
                            continue;

                        else if(line[keyword_pos] == '\"')
                            open_quote = true;

                        else
                            return QString("недопустимый символ");
                    }

                    else if(open_quote == true)
                    {
                        if(close_quote == false)
                        {
                            if(line[keyword_pos] != '\"')
                                path += line[keyword_pos];

                            else
                                close_quote = true;
                        }

                        else if(line[keyword_pos] != '\t' && line[keyword_pos] != ' ')
                            return QString("недопустимый символ");
                    }
                }

                if(open_quote == false)
                    return QString("не указан путь к библиотеке");

                else if(close_quote == false)
                    return QString("нет закрывающей кавычки");

                QString plugResult = plugModule(path);

                if(!plugResult.isEmpty())
                    return plugResult;
            }

            else
            {
                // code for lbls
            }
        }
    }

    if(any_keywords == false)
        outLst.push_back(line);

    return QString();
}

QString MainWindow::plugModule(const QString &path)
{
    QFile libFile(fileRequester -> directory().path() + "/" + path + ".uum32mlb");

    if(!libFile.open(QIODevice::ReadOnly | QIODevice::Text))
        return QString("библиотека \"" + path + "\" не найдена");

    QTextStream stream(&libFile);

    libFile.close();

    return QString();
}
