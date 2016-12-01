#ifndef PARSER_H
#define PARSER_H

#include <QString>
#include <QPair>
#include <QVector>
#include <QStringList>

namespace parser
{
    void        deleteComm      (QString& line);                                // Удаляет комментарий из строки (всё, начиная с ";")
    int         findKeyword     (const QString& line, const QString& keyword);  // Находит ключевое слово keyword в строке line
    QStringList popParam        (const QString& line, quint16 label_pos);       // Выдает параметры макроса
    QString     findPseudLabel  (const QString& line);                          // Выдает имя параметра, начинающегося с $ (если нет - возвр. пустую строку)
    QStringList popReplaceParam (const QString& line);                          // Выдает подставляемые параметры строки кода .uum32mlb
}

#endif
