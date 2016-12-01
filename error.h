#ifndef ERROR_H
#define ERROR_H

#include <QString>

namespace error_handle
{
    enum err_code { SYMBOL_BEFORE_INCLUDE,  SYMB_BTWN_INCLUDE_AND_PATH, SYMB_AFTER_PATH, NO_PATH,
                    NO_CLOSE_QUOT, SYMB_BTWN_LBL_AND_LIBLBL, NO_LBL_NAME, COLON_OVERLAP, INCORRECT_SYMBOL,
                    NO_COLON, PARAM_MISMATCH, COMMA_OVERLAP, LIB_NOT_FOUND, NO_ERROR, NO_MEND, MACRO_OVERLAP,
                    INCORRECT_LBL_NAME, INCORRECT_PARAM_DEFINITION, NO_PARAM_NAME, COMMA_AFTER_PARAM,
                    SYMB_BTWN_LBL_AND_MACRO, AMPERSAND_OVERLAP, NO_AMPERSAND, MEND_OVERLAP, NO_DOLLAR,
                    SYMB_BEFORE_MEND, SYMB_AFTER_MEND };

    err_code include_handle      (const QString& line, quint16 include_pos);                                                // Обработка ошибок, связанных с ключевым словом include
    err_code extern_label_handle (const QString& line, quint16 label_pos, quint8 par_num);                                  // Обработка ошибок, связанных с внешними метками (внутри .uum32mlb)
    err_code macro_handle        (const QString& line, quint16 macro_pos, bool &is_macro_defined, bool &is_mend_defined);   // Обработка ошибок, связанных с ключевым словом macro
    err_code mend_handle         (const QString& line, quint16 mend_pos,  bool &is_macro_defined, bool &is_mend_defined);   // Обработка ошибок, связанных с ключевым словом mend
}

#endif
