#ifndef INTEGRITYCHECK_H
#define INTEGRITYCHECK_H

#include <QByteArray>

namespace IntegrityCheck {

    QByteArray calculateTextSegmentHash();

    bool verify();

}

#endif
