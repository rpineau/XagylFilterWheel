//
//  xagyl.h
//  XagylFilterWheel
//
//  Created by roro on 26/10/16.
//  Copyright © 2016 RTI-Zone. All rights reserved.
//

#ifndef xagyl_h
#define xagyl_h
#include <string.h>
#include "../../licensedinterfaces/sberrorx.h"
#include "../../licensedinterfaces/serxinterface.h"
#include "../../licensedinterfaces/loggerinterface.h"

#define SERIAL_BUFFER_SIZE 20
#define MAX_TIMEOUT 5000
#define XAGYL_LOG_BUFFER_SIZE 256

class CXagyl
{
public:
    CXagyl();
    ~CXagyl();

    int        Connect(const char *szPort);
    void        Disconnect(void);
    bool        IsConnected(void) { return bIsConnected; }

    void        SetSerxPointer(SerXInterface *p) { pSerx = p; }
    void        setLogger(LoggerInterface *pLogger) { mLogger = pLogger; };

protected:
    SerXInterface   *pSerx;
    LoggerInterface *mLogger;

    bool            bIsConnected;
    bool            bDebugLog;

    char            firmwareVersion[SERIAL_BUFFER_SIZE];
    char            mLogBuffer[XAGYL_LOG_BUFFER_SIZE];

};
#endif /* xagyl_h */
