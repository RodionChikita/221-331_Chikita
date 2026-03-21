#ifndef INTEGRITYCHECK_H
#define INTEGRITYCHECK_H

#include <QByteArray>

namespace IntegrityCheck {

    // Computes SHA-256 hash of the .text segment at runtime
    QByteArray calculateTextSegmentHash();

    // Returns true if the current .text hash matches the reference
    bool verify();

} // namespace IntegrityCheck

#endif // INTEGRITYCHECK_H
