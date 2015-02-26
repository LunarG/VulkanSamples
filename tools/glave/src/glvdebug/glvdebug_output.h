#ifndef GLVDEBUG_OUTPUT_H
#define GLVDEBUG_OUTPUT_H

#include <QString>

class QTextEdit;

class glvdebug_output
{
public:
    glvdebug_output();
    ~glvdebug_output();

    void init(QTextEdit* pTextEdit);

    void message(const QString& message);
    void warning(const QString& warning);
    void error(const QString& error);

private:
    QTextEdit* m_pTextEdit;
};

extern glvdebug_output gs_OUTPUT;

inline void glvdebug_output_init(QTextEdit* pTextEdit) { gs_OUTPUT.init(pTextEdit); }
inline void glvdebug_output_message(const QString& message) { gs_OUTPUT.message(message); }
inline void glvdebug_output_warning(const QString& warning) { gs_OUTPUT.warning(warning); }
inline void glvdebug_output_error(const QString& error) { gs_OUTPUT.error(error); }
inline void glvdebug_output_deinit() { gs_OUTPUT.init(0); }

#endif // GLVDEBUG_OUTPUT_H
