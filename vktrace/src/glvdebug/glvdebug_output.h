#ifndef GLVDEBUG_OUTPUT_H
#define GLVDEBUG_OUTPUT_H

#include <QString>
#include <QTextBrowser>
extern "C"
{
#include "glv_platform.h"
#include "glv_tracelog.h"
}

class QTextEdit;

class glvdebug_output
{
public:
    glvdebug_output();
    ~glvdebug_output();

    void init(QTextBrowser* pTextEdit);

    void message(uint64_t packetIndex, const QString& message);
    void warning(uint64_t packetIndex, const QString& warning);
    void error(uint64_t packetIndex, const QString& error);

private:
    QString convertToHtml(QString message);
    void moveCursorToEnd();
    QTextBrowser* m_pTextEdit;
};

extern glvdebug_output gs_OUTPUT;

inline void glvdebug_output_init(QTextBrowser* pTextEdit) { gs_OUTPUT.init(pTextEdit); }

inline void glvdebug_output_message(uint64_t packetIndex, const QString& message) { gs_OUTPUT.message(packetIndex, message); }
inline void glvdebug_output_message(const QString& message) { gs_OUTPUT.message(-1, message); }

inline void glvdebug_output_warning(uint64_t packetIndex, const QString& warning) { gs_OUTPUT.warning(packetIndex, warning); }
inline void glvdebug_output_warning(const QString& warning) { gs_OUTPUT.warning(-1, warning); }

inline void glvdebug_output_error(uint64_t packetIndex, const QString& error) { gs_OUTPUT.error(packetIndex, error); }
inline void glvdebug_output_error(const QString& error) { gs_OUTPUT.error(-1, error); }
inline void glvdebug_output_deinit() { gs_OUTPUT.init(0); }

#endif // GLVDEBUG_OUTPUT_H
