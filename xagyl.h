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
    int nOffset;
    int nLL;
    int nRR;
} filter_params;

typedef struct {
    int nPulseWidth;
    int nJitter;
    int nRotationSpeed;
    int nThreshold;
} wheel_params;

class CXagyl
{
public:
    CXagyl();
    ~CXagyl();

    int             Connect(const char *szPort);
    void            Disconnect(void);
    bool            IsConnected(void) { return m_bIsConnected; };

    void            SetSerxPointer(SerXInterface *p) { m_pSerx = p; };
    void            setLogger(LoggerInterface *pLogger) { m_pLogger = pLogger; };

    // filter wheel communication
    int             filterWheelCommand(const char *szCmd, char *szResult, int nResultMaxLen);
    int             readResponse(char *szRespBuffer, int nBufferLen);

    // Filter Wheel commands
    int             getFirmwareVersion(char *szVersion, int nStrMaxLen);
    int             getModel(char *szModel, int nStrMaxLen);
    int             getSerialnumber(char *szSerialNumber, int nStrMaxLen);
    int             getFilterCount(int &nCount);

    int             moveToFilterIndex(int nTargetPosition);
    int             isMoveToComplete(bool &bComplete);

    int             getNumbersOfSlots(int &nNbSlots);
    int             getCurrentSlot(int &nSlot);

    int             getSlotParams(int nSlotNumber, filter_params &Params);
    int             setSlotParams(int nSlotNumber, int nOffset);

    int             getFilterWheelParams(wheel_params &FilterWheelParams);
    int             setFilterWheelParams(wheel_params FilterWheelParams);

    int             startCalibration();
    int             isCalibrationComplete(bool &bComplete);
    
    int             resetAllToDefault(bool &bNeedCal);
    bool            hasPulseWidthControl();

protected:
    SerXInterface   *m_pSerx;
    LoggerInterface *m_pLogger;

    bool            m_bIsConnected;
    bool            m_bDebugLog;

    char            m_szFirmwareVersion[SERIAL_BUFFER_SIZE];
    float           m_fFirmwareVersion;

    char            m_szLogBuffer[LOG_BUFFER_SIZE];

    bool            m_bCalibrating;
    bool            m_bHasPulseWidthControl;
    int             m_nCurentFilterSlot;
    int             m_nTargetFilterSlot;
    
    int             m_nNbSlot;
    time_t          m_tStartMoveTime;
    wheel_params    m_WheelParams;

    int             getNumbersOfSlotsFromDevice(int &nNbSlots);
    void            convertFirmwareToFloat(char *m_szFirmwareVersion);

    void            hexdump(unsigned char* szInputData, unsigned char *szOutBuffer, int nSize);

};
#endif /* xagyl_h */
