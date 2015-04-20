#include "glvdebug_output.h"
#include <QTextEdit>

glvdebug_output gs_OUTPUT;

glvdebug_output::glvdebug_output()
{
}

glvdebug_output::~glvdebug_output()
{
}

void glvdebug_output::init(QTextEdit* pTextEdit)
{
    m_pTextEdit = pTextEdit;
}

void glvdebug_output::message(const QString& message)
{
    if (m_pTextEdit != NULL)
    {
        m_pTextEdit->append(message);
    }
}

void glvdebug_output::warning(const QString& warning)
{
    if (m_pTextEdit != NULL)
    {
        QString msg = QString("Warning: %1").arg(warning);
        m_pTextEdit->append(msg);
    }
}

void glvdebug_output::error(const QString& error)
{
    if (m_pTextEdit != NULL)
    {
        QString msg = QString("ERROR: %1").arg(error);
        m_pTextEdit->append(msg);
    }
}
