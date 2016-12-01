#include "parser.h"

namespace parser
{
    void deleteComm(QString &line)
    {
        qint32 comm_pos = 0;

        while(comm_pos < line.size() && line[comm_pos] != ';') // Ищем позициию символа начала комментария ";"
            ++comm_pos;

        line.truncate(comm_pos); // Обрезаем комментарий
    }

    int findKeyword(const QString& line, const QString& keyword)
    {
        QVector <QPair <QString, quint16>> word_list;
        QString curr_word;

        QRegExp find_string("'");                        // Нужно, чтобы избежать нахождения ключевых слов в строках с кавычками

        int string_sign = find_string.indexIn(line);

        if(string_sign != -1)
            return -1;

        for(quint16 i = 0; i < line.size(); ++i)
        {
            if(line[i] == ' ' || line[i] == '\t' || (i + 1 == line.size()))
            {
                if(i + 1 == line.size() && line[i] != ' ' && line[i] != '\t')
                    curr_word += line[i],
                    i++;

                word_list.push_back(QPair <QString, quint16> (curr_word, i - curr_word.size()));
                curr_word.clear();
            }

            else
                curr_word += line[i];
        }

        for(int i = 0; i < word_list.size(); ++i)
            if(word_list[i].first.toLower() == keyword.toLower())
                return word_list[i].second;

        return -1;
    }

    QStringList popParam(const QString& line, quint16 label_pos)
    {
        QString     new_param;
        QStringList act_param_list;
        quint16     line_end;

        for(line_end = line.size() - 1; line[line_end] == ' ' || line[line_end] == '\t'; --line_end); // Возвращаемся с конца до первого символа

        for (; label_pos <= line_end; ++label_pos)
        {
            if(line[label_pos] == '&') // НЕПРОТЕСТИРОВАННОЕ УСЛОВИЕ!!! (ИСП. ДЛЯ НАХ. ПАРАМ. ОРИГ. ФАЙЛА)
                continue;

            if (label_pos == line_end)
            {
                new_param += line[label_pos];
                act_param_list.push_back(new_param);
            }

            else if (line[label_pos] == ' ' || line[label_pos] == '\t')
                continue;

            else if (line[label_pos].isLetterOrNumber() || line[label_pos] == '_' || line[label_pos] == '#')
                new_param += line[label_pos];

            else
            {
                act_param_list.push_back(new_param);
                new_param.clear();
            }
        }

        return qMove(act_param_list);
    }

    QString findPseudLabel(const QString &line)
    {
        quint16 count = 0;

        while(count < line.size() && line[count] != '$') ++count;

        if(count == line.size())
            return QString();

        QString label;

        ++count;

        while(count < line.size() && line[count] != ' ' && line[count] != '\t' && line[count] != ':' && line[count] != ',')
            label += line[count++];

        return qMove(label);
    }

    QStringList popReplaceParam(const QString &line)
    {
        QStringList param_list;

        bool is_param_scope = false;

        QString curr_param;

        for(quint16 i = 0; i < line.size(); ++i)
        {
            if(i == line.size() - 1)
            {
                if(is_param_scope == true)
                {
                    if(line[i].isLetterOrNumber() || line[i] == '_')
                        curr_param += line[i];

                    param_list.push_back(curr_param);
                }

            }

            else if(line[i] == '&')
                is_param_scope = true;

            else if(line[i] == ' ' || line[i] == '\t' || line[i] == ',' || line[i] == '[')
            {
                if(is_param_scope == true)
                {
                    param_list.push_back(curr_param);
                    is_param_scope = false;
                    curr_param.clear();
                }
            }

            else
            {
                if(is_param_scope == true)
                    curr_param += line[i];
            }

        }

        return qMove(param_list);
    }
}

