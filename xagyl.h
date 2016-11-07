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
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <memory.h>
#ifdef SB_MAC_BUILD
#include <unistd.h>
#endif

#include "../../licensedinterfaces/sberrorx.h"
#include "../../licensedinterfaces/serxinterface.h"
#include "../../licensedinterfaces/loggerinterface.h"

#define SERIAL_BUFFER_SIZE 32
#define MAX_TIMEOUT 5000
#define LOG_BUFFER_SIZE 256

enum XagylFilterWheelErrors { XA_OK=0, XA_NOT_CONNECTED, XA_CANT_CONNECT, XA_BAD_CMD_RESPONSE, XA_COMMAND_FAILED};

typedef struct {
    int offset;
    int threshold;
} filter_params;

typedef struct {
    int pulseWidth;
    int jitter;
} wheel_params;

class CXagyl
{
public:
    CXagyl();
    ~CXagyl();

    int             Connect(const char *szPort);
    void            Disconnect(void);
    bool            IsConnected(void) { return bIsConnected; }

    void            SetSerxPointer(SerXInterface *p) { pSerx = p; }
    void            setLogger(LoggerInterface *pLogger) { mLogger = pLogger; };

    // filter wheel communication
    int             filterWheelCommand(const char *cmd, char *result, int resultMaxLen);
    int             readResponse(char *respBuffer, int bufferLen);

    // Filter Wheel commands
    int             getFirmwareVersion(char *version, int strMaxLen);
    int             getModel(char *model, int strMaxLen);
    int             getSerialnumber(char *serialNumber, int strMaxLen);
    int             getFilterCount(int &nCount);

    int             moveToFilterIndex(int nTargetPosition);
    int             isMoveToComplete(bool &complete);

    int             getNumbersOfSlots(int &nbSlots);
    int             getFilterParams(int index, filter_params &params);
    
protected:
    SerXInterface   *pSerx;
    LoggerInterface *mLogger;

    bool            bIsConnected;
    bool            bDebugLog;

    char            firmwareVersion[SERIAL_BUFFER_SIZE];
    char            mLogBuffer[LOG_BUFFER_SIZE];

    bool            bCalibrating;
    int             mCurentFilterSlot;
    int             mTargetFilterSlot;

    filter_params   *mFilterParams;
    wheel_params    mWheelParams;
    
    int             mNbSlot;
    int             getGlobalPraramsFromDevice(wheel_params &wheelParams);
    int             getFiltersPraramsFromDevice(filter_params *filterParams, int nbSlots);
    int             setFilterParamsOnDevice(int fiterIndex, int offset, int threshold);
    int             getNumbersOfSlotsFromDevice(int &nbSlots);

    void             hexdump(unsigned char* inputData, unsigned char *outBuffer, int size);

};
#endif /* xagyl_h */
