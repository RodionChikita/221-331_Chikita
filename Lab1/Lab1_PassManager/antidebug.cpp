#include "antidebug.h"
#include <QtGlobal>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace AntiDebug {

bool isDebuggerAttached()
{
#ifdef Q_OS_WIN
    return IsDebuggerPresent() != 0;
#else
    return false;
#endif
}

}
