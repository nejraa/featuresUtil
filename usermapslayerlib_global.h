#ifndef USERMAPSLAYERLIB_GLOBAL_H
#define USERMAPSLAYERLIB_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(USERMAPSLAYERLIB_LIBRARY)
#  define USERMAPSLAYERLIB_API Q_DECL_EXPORT
#else
#  define USERMAPSLAYERLIB_API Q_DECL_IMPORT
#endif

#endif //USERMAPSLAYERLIB_GLOBAL_H
