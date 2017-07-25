#ifndef IOFLAG_H
#define IOFLAG_H

#define IOFLAGLIST                              \
    IOFLAG_DEF(Rd)                              \
    IOFLAG_DEF(Wr)                              \
    IOFLAG_DEF(Vec)                             \
    IOFLAG_DEF(Flush)                           \
    IOFLAG_DEF(RdPhase1)                        \
    IOFLAG_DEF(RdPhase2)

typedef enum {
#define IOFLAG_DEF(_flag)                     \
    ioBit ## _flag,
    IOFLAGLIST
#undef IOFLAG_DEF
    ioBitMax,
} IoBits;

typedef enum {
#define IOFLAG_DEF(_flag)                     \
    ioFlag ## _flag = (0x01 << ioBit ## _flag),
    IOFLAGLIST
#undef IOFLAG_DEF
    ioFlagMax,
} IoFlag;

#define IO_IsWr(_f) 	(_f & ioFlagWr)
#define IO_IsRd(_f) 	(_f & ioFlagRd)
#define IO_IsVec(_f)	(_f & ioFlagVec)
#define IO_IsFlush(_f)	(_f & ioFlagFlush)
#define IO_IsRdPhase1(_f)	(_f & ioFlagRdPhase1)
#define IO_IsRdPhase2(_f)	(_f & ioFlagRdPhase2)

#endif
