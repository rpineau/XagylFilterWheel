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
#include <time.h>
#ifdef SB_MAC_BUILD
#include <unistd.h>
#endif

#include "../../licensedinterfaces/sberrorx.h"
#include "../../licensedinterfaces/serxinterface.h"
#include "../../licensedinterfaces/loggerinterface.h"

#define SERIAL_BUFFER_SIZE 32
#define MAX_TIMEOUT 1000            // in miliseconds
#define MAX_FILTER_CHANGE_TIMEOUT 25 // in seconds
#define LOG_BUFFER_SIZE 256

#ifdef SB_OK
enum XagylFilterWheelErrors { XA_OK=SB_OK, XA_NOT_CONNECTED, XA_CANT_CONNECT, XA_BAD_CMD_RESPONSE, XA_COMMAND_FAILED};
#else
enum XagylFilterWheelErrors { XA_OK=0, XA_NOT_CONNECTED, XA_CANT_CONNECT, XA_BAD_CMD_RESPONSE, XA_COMMAND_FAILED};
#endif


typedef struct {
    int offset;
    int LL;
    int RR;
} filter_params;

typedef struct {
    int pulseWidth;
    int jitter;
    int rotationSpeed;
    int threshold;
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
    int             getCurrentSlot(int &slot);

    int             getSlotParams(int slotNumber, filter_params &params);
    int             setSlotParams(int slotNumber, int offset);

    int             getFilterWheelParams(wheel_params &filterWheelParams);
    int             setFilterWheelParams(wheel_params filterWheelParams);

    int             resetAllToDefault();
    bool            hasPulseWidthControl();

protected:
    SerXInterface   *pSerx;
    LoggerInterface *mLogger;

    bool            bIsConnected;
    bool            bDebugLog;

    char            firmwareVersion[SERIAL_BUFFER_SIZE];
    char            mLogBuffer[LOG_BUFFER_SIZE];

    bool            bCalibrating;
    bool            mHasPulseWidthControl;
    int             mCurentFilterSlot;
    int             mTargetFilterSlot;
    
    int             mNbSlot;
    time_t          mStartMoveTime;
    wheel_params    mWheelParams;

    int             getNumbersOfSlotsFromDevice(int &nbSlots);

    void            hexdump(unsigned char* inputData, unsigned char *outBuffer, int size);

};
#endif /* xagyl_h */
