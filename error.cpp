#include "error.h"

namespace error_handle
{
    err_code include_handle(const QString &line, quint16 include_pos)
    {
        for(quint16 i = 0; i < include_pos; ++i)            // Проверка на символы до include
            if(line[i] != ' ' && line[i] != '\t')
                return SYMBOL_BEFORE_INCLUDE;

        bool open_quote  = false;   // Была ли открывающая кавычка
        bool close_quote = false;   // Была ли закрывающая кавычка

        include_pos += QString("include").size();

        for(; include_pos < line.size(); ++include_pos)
        {
            if(open_quote == false)
            {
                if(line[include_pos] == '\t' || line[include_pos] == ' ')
                    continue;

                else if(line[include_pos] == '\"')
                    open_quote = true;

                else
                    return SYMB_BTWN_INCLUDE_AND_PATH;
            }

            else if(open_quote == true)
            {
                if(close_quote == false && line[include_pos] == '\"')
                    close_quote = true;

                else if(line[include_pos] != '\t' && line[include_pos] != ' ' && close_quote == true)
                    return SYMB_AFTER_PATH;
            }
        }

        if(open_quote == false)
            return NO_PATH;

        else if(close_quote == false)
            return NO_CLOSE_QUOT;

        return NO_ERROR;
    }

    err_code extern_label_handle(const QString &line, quint16 label_pos, quint8 par_num)
    {
        bool colon_sign  = false;
               bool symbol_sign = false;
               bool before_lbl  = true;

               for(quint16 count = 0; count < label_pos; ++count) // Проверка до метки
               {
                   if(line[count].isLetterOrNumber() || line[count] == '_')
                   {
                       if(colon_sign == true)
                           return SYMB_BTWN_LBL_AND_LIBLBL;

                       symbol_sign = true;
                       before_lbl  = false;
                   }

                   else if(line[count] == ':')
                   {
                       if(symbol_sign == false)
                           return NO_LBL_NAME;
                       else if(colon_sign == true)
                           return COLON_OVERLAP;

                       colon_sign = true;
                   }

                   else if(line[count] == ' ' || line[count] == '\t')
                   {
                       if(symbol_sign == true && colon_sign == false && before_lbl == false)
                           return INCORRECT_SYMBOL;
                   }

                   else
                       return INCORRECT_SYMBOL;
               }

               if(symbol_sign == true && colon_sign == false)
                   return NO_COLON;

        for(; label_pos < line.size() && line[label_pos] != ' ' && line[label_pos] != '\t'; ++label_pos); // Перемещаемся за метку

        if(label_pos == line.size() - 1 || label_pos == line.size())
        {
            if(par_num == 0)
                return NO_ERROR;
            else
                return PARAM_MISMATCH;
        }

        bool   first_symb_sign = true;
        bool   comma_sign      = false;

        quint8  act_par_num = 0;
        quint16 line_end;

        for(line_end = line.size() - 1; line[line_end] == ' ' || line[line_end] == '\t'; --line_end); // Возвращаемся обратно

        for(quint16 count = label_pos; count <= line_end; ++count)
        {
            if(count == line_end)
            {
                if(line[count] == ',')
                    return INCORRECT_SYMBOL;
                else if (line[count].isLetterOrNumber() || line[count] == '_')
                    ++act_par_num;
                else
                    return INCORRECT_SYMBOL;
            }

            else if(line[count] == ',')
            {
                if(first_symb_sign == true)
                    return INCORRECT_SYMBOL;

                if(comma_sign == true)
                    return COMMA_OVERLAP;

                comma_sign = true;
                ++act_par_num;
            }

            else if(line[count].isLetterOrNumber() || line[count] == '_' || line[count] == '#')
            {
                first_symb_sign = false;
                comma_sign      = false;
            }

            else if(line[count] == ' ' || line[count] == '\t')
            {
                if(comma_sign == false && !first_symb_sign == true)
                    return INCORRECT_SYMBOL;
            }

            else
                return INCORRECT_SYMBOL;
        }



        if(par_num != act_par_num)
            return PARAM_MISMATCH;

        return NO_ERROR;
    }

    err_code macro_handle(const QString& line, quint16 macro_pos, bool& is_macro_defined, bool& is_mend_defined)
    {
        if(is_macro_defined == true)
            return MACRO_OVERLAP;

        is_macro_defined = true;
        is_mend_defined  = false; // Отмена действия "mend"

        bool lbl_over = false;
        bool any_symb = false;

        for(quint16 i = 0; i < macro_pos; ++i) // Находим имя метки
        {
            if(lbl_over == false)
            {
                if(line[i] == ':')
                {
                    if(any_symb == false)
                        return NO_LBL_NAME;

                    else
                        lbl_over = true;
                }

                else if((line[i] == ' ' || line[i] == '\t'))
                {
                    if(any_symb == false)
                        continue;
                    else
                        return INCORRECT_LBL_NAME;
                }

                else if(line[i].isLetterOrNumber() || (line[i] == '_'))
                    any_symb = true;

                else
                    return INCORRECT_SYMBOL;
            }

            else
            {
                if(line[i] == '\t' || line[i] == ' ')
                    continue;
                else
                    return SYMB_BTWN_LBL_AND_MACRO;
            }
        }

        if(!lbl_over)
            return INCORRECT_LBL_NAME;

        macro_pos += 5; // Перепрыгиваем за macro

        quint16 line_end;

        for(line_end = line.size() - 1; line[line_end] == ' ' || line[line_end] == '\t'; --line_end); // Возвращаемся обратно до последнего параметра / слова macro

        bool    comma_sign      = false; // Действует на пред. символ (с учетом \t и [whitespace])
        bool    first_symb_sign = true;  // Действует на пред. символ
        bool    ampersand_sign  = false; // Действует на пред. символ
        bool    word_amp_sign   = false; // Действует на весь параметр

        for(quint16 count = macro_pos; count <= line_end; ++count)
        {
            if(count == line_end)
            {
                if(line[count] == '&')
                    return INCORRECT_SYMBOL;
                else if(line[count] == ',')
                    return INCORRECT_SYMBOL;
                else if(line[count].isLetterOrNumber() || line[count] == '_')
                    continue;
                else
                    return INCORRECT_SYMBOL;
            }

            else if(line[count] == '&')
            {
                if(ampersand_sign == true)
                    return AMPERSAND_OVERLAP;

                else if(word_amp_sign == true)
                    return INCORRECT_SYMBOL;
                else if(comma_sign == false && first_symb_sign == false)
                    return INCORRECT_SYMBOL;

                ampersand_sign  = true;
                word_amp_sign   = true;
                comma_sign      = false;
                first_symb_sign = false;
            }

            else if(line[count] == ',')
            {
                if(ampersand_sign == true)
                    return NO_PARAM_NAME;  // ПРОВЕРИТЬ [ &, ]
                else if(word_amp_sign == false)
                    return INCORRECT_SYMBOL;
                else if(comma_sign == true)
                    return COMMA_OVERLAP;

                comma_sign    = true;
                word_amp_sign = false;
            }

            else if (line[count] == ' ' || line[count] == '\t')
            {
                if(first_symb_sign == true)
                    continue;

                else if(word_amp_sign == true)
                    return INCORRECT_SYMBOL;
            }

            else if(line[count].isLetterOrNumber() || line[count] == '#' || line[count] == '_')
            {
                if(word_amp_sign == false)
                    return NO_AMPERSAND;

                ampersand_sign = false;
            }

            else
                return INCORRECT_SYMBOL;
        }

        return NO_ERROR;
    }

    err_code mend_handle(const QString &line, quint16 mend_pos, bool &is_macro_defined, bool &is_mend_defined)
    {
        if(is_mend_defined)
            return error_handle::MEND_OVERLAP;

        is_mend_defined  = true;
        is_macro_defined = false; // Отменяем действие "macro"

        for(quint16 count = 0; count < mend_pos; ++count)
            if(line[count] != ' ' && line[count] != '\t')
                return SYMB_BEFORE_MEND;

        for(quint16 count = mend_pos + 4; count < line.size(); ++count)
            if(line[count] != ' ' && line[count] != '\t')
                return SYMB_AFTER_MEND;


        return NO_ERROR;
    }
}
