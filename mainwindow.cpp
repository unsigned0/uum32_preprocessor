#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) : QWidget(parent), postfix(0), ui(new Ui::MainWindow)
{
    ui -> setupUi(this);

    ui -> start_btn -> setEnabled(false);

    fileRequester = new QFileDialog(this, QString(), QString(), "Исходный код программы на макроассемблере для УУМ-32 (*.uum32masm)");

    connect(ui -> open_btn,  SIGNAL(clicked()),  fileRequester,     SLOT(exec()));
    connect(ui -> clear_btn, SIGNAL(clicked()),  ui -> textBrowser, SLOT(clear()));
    connect(fileRequester,   SIGNAL(accepted()), SLOT(fileSelected()));
    connect(fileRequester,   SIGNAL(rejected()), SLOT(fileRejected()));
    connect(ui -> start_btn, SIGNAL(clicked()),  SLOT(handle()));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::fileSelected()
{
    ui -> path_lbl  -> setText(fileRequester -> selectedFiles().at(0));
    ui -> start_btn -> setEnabled(true);
}

void MainWindow::fileRejected()
{
    ui -> start_btn   -> setEnabled(false);
    ui -> textBrowser -> setPlainText("");
}

void MainWindow::handle()
{
    QFile masm_file(fileRequester -> selectedFiles().at(0));

    if(!masm_file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QMessageBox msg("Ошибка", "Невозможно открыть файл", QMessageBox::Critical, QMessageBox::Ok, QMessageBox::NoButton, QMessageBox::NoButton);
        msg.exec();

        return;
    }

    QTextStream stream(&masm_file);

    stream.setCodec("UTF-8");

    QString line;

    for(masm_curr_line = 1; stream.readLineInto(&line); ++masm_curr_line)
        if(parseLine(line) == false)
        {
            resetState();
            return;
        }

    stream.seek(0);

    if(lib_info.isEmpty()) // Если библиотека пустая -> нужно просто копировать один файл в другой (ПРОВЕРИТЬ СЛУЧАЙ С ПУСТОЙ БИБЛИОТЕКОЙ)
    {
        QString path_to_asm_file = fileRequester -> selectedFiles().at(0);

        path_to_asm_file.truncate(fileRequester -> selectedFiles().at(0).size() - 9);
        path_to_asm_file += "uum32asm";

        QFile asm_file(path_to_asm_file);
        asm_file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text);

        QTextStream asm_stream(&asm_file);
        asm_stream.setCodec("UTF-8");

        while(stream.readLineInto(&line))
            asm_stream << line << endl;

        masm_file.close();
        asm_file.close();

        ui -> textBrowser -> append("Файл успешно сгенерирован");

        return;
    }

    curr_lib_num = 0;

    while(stream.readLineInto(&line))
        if(translateLine(line) == false)
        {
            resetState();
            return;
        }

    for(auto lib_iter: lib_info)
    {
        for(auto macro_iter: lib_iter)
        {
            qDebug() << "Информация о метке                      : " << macro_iter.first.label;
            qDebug() << "Количество формальных параметров        : " << macro_iter.first.arg_num;
            qDebug() << "Количество использований макроса в коде : " << macro_iter.first.use_num;
            qDebug() << "Фактические параметры                   : " << macro_iter.first.actualParamList;
            qDebug() << "Код метки                               : " << macro_iter.second;
            qDebug() << "-----------------------------------";
        }
    }

    QString path_to_asm_file = fileRequester -> selectedFiles().at(0);

    path_to_asm_file.truncate(fileRequester -> selectedFiles().at(0).size() - 9);
    path_to_asm_file += "uum32asm";

    QFile asm_file(path_to_asm_file);
    asm_file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text);

    QTextStream asm_stream(&asm_file);
    asm_stream.setCodec("UTF-8");

    for(auto iter: outLst)
        asm_stream << iter << endl;

    masm_file.close();
    asm_file.close();

    resetState();

    ui -> textBrowser -> append("Файл успешно сгенерирован");
}

bool MainWindow::parseLine(QString &line)
{
    parser::deleteComm(line);

    if(line.size() == 0)
        return true;

    int keyw_pos = parser::findKeyword(line, "include");

    if (keyw_pos != -1)
    {
        error_handle::err_code e_code = error_handle::include_handle(line, keyw_pos);

        if(e_code != error_handle::NO_ERROR)
        {
            outError(e_code);
            return false;
        }

        QString path;
        quint16 count = 0;

        while(line[count] != '\"') ++count; ++count;
        while(line[count] != '\"') path += line[count], ++count;

        if(!lookUpModule(path))
            return false;
    }

    else
    {
        for (quint8 i = 0; i < lib_info.size(); ++i)
        {
            for(quint16 j = 0; j < lib_info[i].size(); ++j)
            {
                int label_pos = parser::findKeyword(line, lib_info[i][j].first.label);

                if (label_pos == -1)
                    continue;

                error_handle::err_code e_code = error_handle::extern_label_handle(line, label_pos, lib_info[i][j].first.arg_num);

                if(e_code != error_handle::NO_ERROR)
                {
                    outError(e_code);
                    return false;
                }

                for (; line[label_pos] != ' ' && line[label_pos] != '\t' && label_pos <= line.size() - 1; ++label_pos);

                QStringList temp_list = parser::popParam(line, label_pos);

                for (quint8 par_count = 0; par_count < temp_list.size(); ++par_count)
                    lib_info[i][j].first.actualParamList.push_back(temp_list[par_count]);

                //++lib_info[i][j].first.use_num;
            }
        }
    }

    return true;
}

bool MainWindow::lookUpModule(const QString &path)
{
    QFile mlb_file(fileRequester -> directory().path() + "/" + path + ".uum32mlb");

    masm_curr_lib = path;

    if(!mlb_file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        outError(error_handle::LIB_NOT_FOUND);
        return false;
    }

    QTextStream stream(&mlb_file);
    QString line;

    bool is_macro_def = false;
    bool is_mend_def  = true;

    lib_info.push_back(LibInfo()); // Добавляем "пустую" библиотеку

    for(mlb_curr_line = 1; stream.readLineInto(&line); ++mlb_curr_line)
        if(!parseModuleLine(line, is_macro_def, is_mend_def))
            return false;

    if(is_mend_def == false)
    {
        outError(error_handle::NO_MEND);
        return false;
    }

    mlb_file.close();

    mlb_curr_line = 0;

    if(lib_info.last().isEmpty())
        lib_info.pop_back();

    return true;
}

bool MainWindow::parseModuleLine(QString &line, bool &is_macro_defined, bool &is_mend_defined)
{
    parser::deleteComm(line);

    if(line.size() == 0)
        return true;

    int keyw_pos = parser::findKeyword(line, "macro");

    if(keyw_pos != -1) // Обрабатываем "macro"
    {
        error_handle::err_code e_code = error_handle::macro_handle(line, keyw_pos, is_macro_defined, is_mend_defined);

        if(e_code != error_handle::NO_ERROR)
        {
            outError(e_code, true);
            return false;
        }

        QString external_lbl;

        for(quint16 count = 0; line[count] != ':'; ++count)             // Получение имени метки
            external_lbl += line[count];

        quint8 par_num = 0;

        for(quint16 count = keyw_pos + 5; count < line.size(); ++count) // Получение кол - ва параметров
            if(line[count] == '&')
                ++par_num;

        lib_info.last().push_back({MacroLabel(external_lbl, par_num), QStringList()});
    }

    else // Обрабатываем mend
    {
        keyw_pos = parser::findKeyword(line, "mend");

        if(keyw_pos != -1)
        {
            error_handle::err_code e_code = error_handle::mend_handle(line, keyw_pos, is_macro_defined, is_mend_defined);

            if(e_code != error_handle::NO_ERROR)
            {
                outError(e_code, true);
                return false;
            }
        }
    }

    return true;
}

void MainWindow::outError(error_handle::err_code err_code, bool lib_sign)
{
    using namespace error_handle;
    QString error;

    switch(err_code)
    {
        case SYMBOL_BEFORE_INCLUDE:
            error = "Некорректный символ перед <font color = #00BBFF> include </font>";
            break;
        case SYMB_BTWN_INCLUDE_AND_PATH:
            error = "Некорректный символ между <font color = #00BBFF> include </font> и открывающей кавычкой";
            break;
        case SYMB_AFTER_PATH:
            error = "Некорректный символ после закрывающей кавычки";
            break;
        case NO_PATH:
            error = "Не указан путь к библиотеке";
            break;
        case NO_CLOSE_QUOT:
            error = "Нет закрывающей кавчки";
            break;
        case SYMB_BTWN_LBL_AND_LIBLBL:
            error = "Некорректный символ между мектой и именем макроса";
            break;
        case NO_LBL_NAME:
            error = "Нет имени метки";
            break;
        case COLON_OVERLAP:
            error = "Повторное использование двоеточия";
            break;
        case INCORRECT_SYMBOL:
            error = "Некорректный символ";
            break;
        case NO_COLON:
            error = "Пропущена запятая";
            break;
        case PARAM_MISMATCH:
            error = "Несовпадение количества фактических и формальных параметров";
            break;
        case COMMA_OVERLAP:
            error = "Повторное использование запятой";
            break;
        case LIB_NOT_FOUND:
            error = "Не найдена библиотека \"" + masm_curr_lib + "\"";
            break;
        case NO_MEND:
            error = "Нет закрывающего mend";
            break;
        case MACRO_OVERLAP:
            error = "Двойное включение <font color = #00BBFF> macro </font>";
            break;
        case INCORRECT_LBL_NAME:
            error = "Некорректное имя метки";
            break;
        case INCORRECT_PARAM_DEFINITION:
            error = "Некорректное определение параметров";
            break;
        case NO_PARAM_NAME:
            error = "Нет названия параметра";
            break;
        case COMMA_AFTER_PARAM:
            error = "Запятая после определения параметров";
            break;
        case SYMB_BTWN_LBL_AND_MACRO:
            error = "";
            break;
        case AMPERSAND_OVERLAP:
            error = "Повторное использование амперсанда";
            break;
        case NO_AMPERSAND:
            error = "Не найден &";
            break;
        case MEND_OVERLAP:
            error = "Некорректное включение <font color = #00BBFF> mend </font>";
            break;
        case NO_DOLLAR:
            error = "Метка должна начинаться со знака $";
            break;
        case SYMB_BEFORE_MEND:
            error = "Некорректный перед <font color = #00BBFF> mend </font>";
            break;
        case SYMB_AFTER_MEND:
            error = "Некорректный символ после <font color = #00BBFF> mend </font>";
            break;
        default:
            error = "Критическая ошибка макропроцессора";
            break;
    }

    if(lib_sign == false)
        ui -> textBrowser -> append("<font color = #FF0000> Ошибка </font> в строке [" + QString::number(masm_curr_line) + "]: " + error);
    else
        ui -> textBrowser -> append("<font color = #FF0000> Ошибка </font> в библиотеке \"" + masm_curr_lib + "\" в строке [" + QString::number(mlb_curr_line) + "]: " + error);
}

void MainWindow::resetState()
{
    outLst.clear();
    lib_info.clear();
    postfix = 0;
}

bool MainWindow::translateLine(QString &line)
{
    parser::deleteComm(line);

    if(line.size() == 0)
        return true;

    int keyw_pos = parser::findKeyword(line, "include");

    if(keyw_pos != -1)
    {
        QString path;
        quint16 count = 0;

        while(line[count] != '\"') ++count; ++count;
        while(line[count] != '\"') path += line[count], ++count;

        if(translateModule(path) == false)
            return false;
    }

    else
    {
        bool any_replacement = false;

        for (quint8 i = 0; i < lib_info.size() && any_replacement == false; ++i)
        {
            for(quint16 j = 0; j < lib_info[i].size() && any_replacement == false; ++j)
            {
                int label_pos = parser::findKeyword(line, lib_info[i][j].first.label);

                if (label_pos == -1)
                    continue;

                any_replacement = true;

                QVector <QPair <QString, QString>> replacement_table;
                QStringList form_param_list;

                for(quint16 count = 0; count < lib_info[i][j].second.size(); ++count)
                {
                    if(count != 0 && count != lib_info[i][j].second.size() - 1) // Избежание включения macro и mend
                    {
                        QString pseud_label = parser::findPseudLabel(lib_info[i][j].second[count]);
                        QString buff_line   = lib_info[i][j].second[count];

                        if(!pseud_label.isEmpty()) // Подстановка псевдометок
                        {
                            quint16 _count;
                            for(_count = 0; _count < replacement_table.size(); ++_count)
                                if(pseud_label == replacement_table[_count].first)
                                    break;

                            quint16 dollar_pos;       // Позиция знака '$'

                            if(_count == replacement_table.size()) // Метка не найдена
                            {
                                replacement_table.push_back({pseud_label, "__" + pseud_label + "_0x" + QString::number(postfix, 16).toUpper()});

                                ++postfix;

                                for(dollar_pos = 0; buff_line[dollar_pos] != '$'; ++dollar_pos);

                                buff_line.insert(dollar_pos, replacement_table.last().second);
                            }

                            else    // Метка найдена
                            {
                                for(dollar_pos = 0; buff_line[dollar_pos] != '$'; ++dollar_pos);

                                buff_line.insert(dollar_pos, replacement_table[_count].second);
                            }

                            buff_line.remove("$" + pseud_label);
                        }

                        // Связывание параметров
                        QStringList replace_param_list = parser::popReplaceParam(buff_line);

                        qDebug() << replace_param_list;

                        if(!replace_param_list.isEmpty())
                        {
                            for(quint16 _count = 0; _count < replace_param_list.size(); ++_count)
                            { 
                                qDebug() << form_param_list.indexOf(QRegExp(replace_param_list[_count])) + lib_info[i][j].first.use_num * lib_info[i][j].first.arg_num;
                                buff_line.insert(buff_line.indexOf("&" + replace_param_list[_count]), lib_info[i][j].first.actualParamList[form_param_list.indexOf(QRegExp(replace_param_list[_count])) + lib_info[i][j].first.use_num * lib_info[i][j].first.arg_num]);
                                buff_line.remove("&" + replace_param_list[_count], Qt::CaseInsensitive);
                            }
                        }

                        outLst.push_back(buff_line);
                    }

                    else // Случаи MACRO + MEND
                    {
                        QString out(";=================");

                        if(count == 0) // Macro
                        {
                            int macro_index = parser::findKeyword(lib_info[i][j].second[count], "macro");

                            macro_index += QString("macro").size();

                            out += lib_info[i][j].second[count].simplified();

                            bool colon_sign = false;
                            QString add_line; // Если на строке с макросом стоит метка (masm)
                            for(quint16 _i = 0; !colon_sign && _i < label_pos; ++_i)
                            {
                                add_line += line[_i];
                                if(line[_i] == ':')
                                    colon_sign = true;
                            }

                            outLst.push_back(add_line);

                            form_param_list = parser::popParam(lib_info[i][j].second[count], macro_index);
                        }

                        else // Mend
                        {
                            ++lib_info[i][j].first.use_num;
                            out += "MEND";
                        }

                        out += "=================";

                        outLst.push_back(out);
                    }
                }
            }
        }

        if(any_replacement == false)
            outLst.push_back(line);
    }

    return true;
}

bool MainWindow::translateModule(const QString& path)
{
    QFile mlb_file(fileRequester -> directory().path() + "/" + path + ".uum32mlb");

    masm_curr_lib = path;

    if(!mlb_file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        outError(error_handle::LIB_NOT_FOUND);
        return false;
    }

    QTextStream stream(&mlb_file);
    QString line;

    bool    in_macro_sign = false;
    qint32  curr_macro    = -1;

    for(mlb_curr_line = 1; stream.readLineInto(&line); ++mlb_curr_line)
        if(!translateModuleLine(line, in_macro_sign, curr_macro))
            return false;

    ++curr_lib_num;

    return true;
}

bool MainWindow::translateModuleLine(QString& line, bool& in_macro_sign, qint32& curr_macro)
{
    parser::deleteComm(line);

    if(line.size() == 0)
        return true;

    int keyw_pos = parser::findKeyword(line, "macro");

    if(keyw_pos != -1)
    {
        in_macro_sign = true;
        ++curr_macro;

        lib_info[curr_lib_num][curr_macro].second.push_back(line);
    }

    else
    {
        if(in_macro_sign == false)
            return true;

        keyw_pos = parser::findKeyword(line, "mend");

        if(keyw_pos != -1)
            in_macro_sign = false;

        lib_info[curr_lib_num][curr_macro].second.push_back(line);
    }

    return true;
}
