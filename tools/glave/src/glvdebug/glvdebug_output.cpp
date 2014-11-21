#include "glvdebug_output.h"
#include <QTextEdit>

glvdebug_output gs_OUTPUT;

glvdebug_output::glvdebug_output()
{
}

glvdebug_output::~glvdebug_output()
{
}

void glvdebug_output::message(const char* pMessage, bool bRefresh)
{
    if (m_pTextEdit != NULL)
    {
        m_pTextEdit->append(pMessage);
        if (bRefresh)
        {
            m_pTextEdit->repaint();
        }
    }
}

void glvdebug_output::warning(const char* pWarning, bool bRefresh)
{
    if (m_pTextEdit != NULL)
    {
        QString msg = QString("Warning: %1").arg(pWarning);
        m_pTextEdit->append(msg);
        if (bRefresh)
        {
            m_pTextEdit->repaint();
        }
    }
}

void glvdebug_output::error(const char* pError, bool bRefresh)
{
    if (m_pTextEdit != NULL)
    {
        QString msg = QString("ERROR: %1").arg(pError);
        m_pTextEdit->append(msg);
        if (bRefresh)
        {
            m_pTextEdit->repaint();
        }
    }
}
