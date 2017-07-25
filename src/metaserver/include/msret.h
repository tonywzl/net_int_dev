#ifndef MSRET_H
#define MSRET_H

typedef enum {
    retOk,
    retFail,
    retInvalidArg,
    retTmout,
    retNotFound,
    retCephErrBase = 0x01<<24,
    retCassErrBase = 0x10<<24,
} MsRet;

#endif
