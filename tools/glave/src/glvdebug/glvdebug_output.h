#ifndef GLVDEBUG_OUTPUT_H
#define GLVDEBUG_OUTPUT_H

class QTextEdit;

class glvdebug_output
{
public:
    glvdebug_output();
    ~glvdebug_output();

    void init(QTextEdit* pTextEdit) { m_pTextEdit = pTextEdit; }

    void message(const char* pMessage, bool bRefresh);
    void warning(const char* pWarning, bool bRefresh);
    void error(const char* pError, bool bRefresh);

private:
    QTextEdit* m_pTextEdit;
};

extern glvdebug_output gs_OUTPUT;

inline void glvdebug_output_init(QTextEdit* pTextEdit) { gs_OUTPUT.init(pTextEdit); }
inline void glvdebug_output_message(const char* pMessage, bool bRefresh = true) { gs_OUTPUT.message(pMessage, bRefresh); }
inline void glvdebug_output_warning(const char* pWarning, bool bRefresh = true) { gs_OUTPUT.warning(pWarning, bRefresh); }
inline void glvdebug_output_error(const char* pError, bool bRefresh = true) { gs_OUTPUT.error(pError, bRefresh); }
inline void glvdebug_output_deinit() { gs_OUTPUT.init(0); }

#endif // GLVDEBUG_OUTPUT_H
