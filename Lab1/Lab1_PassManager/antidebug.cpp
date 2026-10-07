#include "antidebug.h"
#include <QtGlobal>  // определяет Q_OS_WIN (без этого заголовка макрос не виден в этом TU,
                     // и функция ниже всегда возвращала бы false через ветку #else)

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

} // namespace AntiDebug
