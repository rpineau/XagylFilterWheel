//
//  xagyl.cpp
//  XagylFilterWheel
//
//  Created by Rodolphe Pineau on 26/10/16.
//  Copyright © 2016 RTI-Zone. All rights reserved.
//

#include "xagyl.h"

CXagyl::CXagyl()
{
    m_bIsConnected = false;
    m_bCalibrating = false;
    m_bHasPulseWidthControl = false;
    m_bDebugLog = false;
    m_nCurentFilterSlot = -1;
    m_nTargetFilterSlot = 0;
    m_nNbSlot = 0;
}

CXagyl::~CXagyl()
{

}

int CXagyl::Connect(const char *szPort)
{
    int nErr = XA_OK;
    // 9600 8N1
    if(m_pSerx->open(szPort, 9600, SerXInterface::B_NOPARITY, "-DTR_CONTROL 1") == 0)
        m_bIsConnected = true;
    else
        m_bIsConnected = false;

    if(!m_bIsConnected)
        return ERR_COMMNOLINK;

    if (m_bDebugLog) {
        snprintf(m_szLogBuffer,LOG_BUFFER_SIZE,"[Xagyl::Connect] Connected.\n");
        m_pLogger->out(m_szLogBuffer);

        snprintf(m_szLogBuffer,LOG_BUFFER_SIZE,"[Xagyl::Connect] Getting Firmware.\n");
        m_pLogger->out(m_szLogBuffer);
    }

    // if any of this fails we're not properly connected or there is a hardware issue.
    nErr = getFirmwareVersion(m_szFirmwareVersion, SERIAL_BUFFER_SIZE);
    if(nErr) {
        if (m_bDebugLog) {
            snprintf(m_szLogBuffer,LOG_BUFFER_SIZE,"[Xagyl::Connect] Error Getting Firmware.\n");
            m_pLogger->out(m_szLogBuffer);
        }
        m_bIsConnected = false;
        m_pSerx->close();
        return FIRMWARE_NOT_SUPPORTED;
    }

    if (m_bDebugLog) {
        snprintf(m_szLogBuffer,LOG_BUFFER_SIZE,"[Xagyl::Connect] Got Firmware.\n");
        m_pLogger->out(m_szLogBuffer);
    }
    // convert it to a float for testing
    convertFirmwareToFloat(m_szFirmwareVersion);

    // get the number of slots
    nErr = getNumbersOfSlots(m_nNbSlot);
    if(nErr) {
        if (m_bDebugLog) {
            snprintf(m_szLogBuffer,LOG_BUFFER_SIZE,"[Xagyl::Connect] Error Getting the number of available slots.\n");
            m_pLogger->out(m_szLogBuffer);
        }
        m_bIsConnected = false;
        m_pSerx->close();
        return ERR_CMDFAILED;
    }
    if (m_bDebugLog) {
        snprintf(m_szLogBuffer,LOG_BUFFER_SIZE,"[Xagyl::Connect] Got number of slots : %d.\n", m_nNbSlot);
        m_pLogger->out(m_szLogBuffer);
    }

    nErr = getFilterWheelParams(m_WheelParams);
    if(nErr) {
        if (m_bDebugLog) {
            snprintf(m_szLogBuffer,LOG_BUFFER_SIZE,"[Xagyl::Connect] Error Getting filter wheel gobal params.\n");
            m_pLogger->out(m_szLogBuffer);
        }
        m_bIsConnected = false;
        m_pSerx->close();
        return ERR_CMDFAILED;
    }

    // set wheel to verbose mode
    // char resp[SERIAL_BUFFER_SIZE];
    // filterWheelCommand("V1", resp, SERIAL_BUFFER_SIZE);
    return nErr;
}



void CXagyl::Disconnect()
{
    if(m_bIsConnected) {
        m_pSerx->purgeTxRx();
        m_pSerx->close();
    }
    m_bIsConnected = false;
}


#pragma mark - communication functions
int CXagyl::readResponse(char *szRespBuffer, int nBufferLen)
{
    int nErr = XA_OK;
    unsigned long ulBytesRead = 0;
    unsigned long ulTotalBytesRead = 0;
    char *szBufPtr;
    
    memset(szRespBuffer, 0, (size_t) nBufferLen);
    szBufPtr = szRespBuffer;

    do {
        nErr = m_pSerx->readFile(szBufPtr, 1, ulBytesRead, MAX_TIMEOUT);
        if(nErr) {
            if (m_bDebugLog) {
                snprintf(m_szLogBuffer,LOG_BUFFER_SIZE,"[CXagyl::readResponse] readFile error.\n");
                m_pLogger->out(m_szLogBuffer);
            }
            return nErr;
        }

        if (m_bDebugLog) {
            snprintf(m_szLogBuffer,LOG_BUFFER_SIZE,"[CXagyl::readResponse] respBuffer = %s\n",szRespBuffer);
            m_pLogger->out(m_szLogBuffer);
        }

        if (ulBytesRead !=1) {// timeout
            if (m_bDebugLog) {
                snprintf(m_szLogBuffer,LOG_BUFFER_SIZE,"[CXagyl::readResponse] readFile Timeout.\n");
                m_pLogger->out(m_szLogBuffer);
            }
            nErr = XA_BAD_CMD_RESPONSE;
            break;
        }
        ulTotalBytesRead += ulBytesRead;
        if (m_bDebugLog) {
            snprintf(m_szLogBuffer,LOG_BUFFER_SIZE,"[CXagyl::readResponse] ulBytesRead = %lu\n",ulBytesRead);
            m_pLogger->out(m_szLogBuffer);
            snprintf(m_szLogBuffer,LOG_BUFFER_SIZE,"[CXagyl::readResponse] ulTotalBytesRead = %lu\n",ulTotalBytesRead);
            m_pLogger->out(m_szLogBuffer);
        }
    } while (*szBufPtr++ != 0xA && ulTotalBytesRead < (unsigned long)nBufferLen );

    if(ulTotalBytesRead) {
        *(szBufPtr-2) = 0; //remove the 0xD
        *(szBufPtr-1) = 0; //remove the 0xA
    }
    return nErr;
}


int CXagyl::filterWheelCommand(const char *szCmd, char *szResult, int nResultMaxLen)
{
    int nErr = XA_OK;
    char szResp[SERIAL_BUFFER_SIZE];
    unsigned long  ulBytesWrite;

    m_pSerx->purgeTxRx();
    if (m_bDebugLog) {
        snprintf(m_szLogBuffer,LOG_BUFFER_SIZE,"[CXagyl::filterWheelCommand] Sending %s\n",szCmd);
        m_pLogger->out(m_szLogBuffer);
    }
    nErr = m_pSerx->writeFile((void *)szCmd, strlen(szCmd), ulBytesWrite);
    m_pSerx->flushTx();

    // printf("Command %s sent. wrote %lu bytes\n", cmd, nBytesWrite);
    if(nErr){
        if (m_bDebugLog) {
            snprintf(m_szLogBuffer,LOG_BUFFER_SIZE,"[CXagyl::filterWheelCommand] writeFile error.\n");
            m_pLogger->out(m_szLogBuffer);
        }
        return nErr;
    }

    if(szResult) {
        // read response
        if (m_bDebugLog) {
            snprintf(m_szLogBuffer,LOG_BUFFER_SIZE,"[CXagyl::filterWheelCommand] Getting response.\n");
            m_pLogger->out(m_szLogBuffer);
        }
        nErr = readResponse(szResp, SERIAL_BUFFER_SIZE);
        if(nErr){
            if (m_bDebugLog) {
                snprintf(m_szLogBuffer,LOG_BUFFER_SIZE,"[CXagyl::filterWheelCommand] readResponse error.\n");
                m_pLogger->out(m_szLogBuffer);
            }
        }
        // printf("Got response : %s\n",resp);
        strncpy(szResult, szResp, nResultMaxLen);
    }
    return nErr;
    
}

#pragma mark - Filter Wheel info commands

int CXagyl::getFirmwareVersion(char *szVersion, int nStrMaxLen)
{
    int nErr = 0;
    char szResp[SERIAL_BUFFER_SIZE];

    if(!m_bIsConnected)
        return XA_NOT_CONNECTED;

    if(m_bCalibrating)
        return XA_OK;

    nErr = filterWheelCommand("I1", szResp, SERIAL_BUFFER_SIZE);
    if(nErr) {
        if (m_bDebugLog) {
            snprintf(m_szLogBuffer,LOG_BUFFER_SIZE,"[CXagyl::getFirmwareVersion] Error Getting response from filterWheelCommand.\n");
            m_pLogger->out(m_szLogBuffer);
        }
        return nErr;
    }
    strncpy(szVersion, szResp, nStrMaxLen);

    return nErr;
}

int CXagyl::getModel(char *szModel, int nStrMaxLen)
{
    int nErr = 0;
    char szResp[SERIAL_BUFFER_SIZE];

    if(!m_bIsConnected)
        return XA_NOT_CONNECTED;

    if(m_bCalibrating)
        return XA_OK;

    nErr = filterWheelCommand("I0", szResp, SERIAL_BUFFER_SIZE);
    if(nErr)
        return nErr;

    strncpy(szModel, szResp, nStrMaxLen);
    return nErr;
}

int CXagyl::getSerialnumber(char *szSerialNumber, int nStrMaxLen)
{
    int nErr = 0;
    char szResp[SERIAL_BUFFER_SIZE];
    
    if(!m_bIsConnected)
        return XA_NOT_CONNECTED;
    
    if(m_bCalibrating)
        return XA_OK;
    
    nErr = filterWheelCommand("I3", szResp, SERIAL_BUFFER_SIZE);
    if(nErr)
        return nErr;
    
    strncpy(szSerialNumber, szResp, nStrMaxLen);
    return nErr;
}


int CXagyl::getFilterCount(int &nCount)
{
    int nErr = XA_OK;
    
    if (m_bIsConnected)
        nErr = getNumbersOfSlotsFromDevice(nCount);
    else
        nCount = m_nNbSlot;
    return nErr;
}


#pragma mark - Filter Wheel move commands

int CXagyl::moveToFilterIndex(int nTargetPosition)
{
    int nErr = 0;
    char szCmd[SERIAL_BUFFER_SIZE];

    snprintf(szCmd,SERIAL_BUFFER_SIZE, "G%d", nTargetPosition);
    nErr = filterWheelCommand(szCmd, NULL, 0);
    if(nErr)
        return nErr;

    m_nTargetFilterSlot = nTargetPosition;
    m_tStartMoveTime = time(NULL);
    return nErr;
}

int CXagyl::isMoveToComplete(bool &bComplete)
{
    int nErr = XA_OK;
    int rc = 0;
    int nFilterSlot;
    char szResp[SERIAL_BUFFER_SIZE];
    time_t tNow;
    int nbWaitingByte;

    bComplete = false;
    if(m_nCurentFilterSlot == m_nTargetFilterSlot) {
        bComplete = true;
        return nErr;
    }

    // firmare 3.3.x is non responsive durring movement so we need to wait until there is something to read.
    if(m_fFirmwareVersion <= 3.4) {
        m_pSerx->bytesWaitingRx(nbWaitingByte);
        if (nbWaitingByte) {
            tNow = time(NULL);
            if(!bComplete && ((tNow - m_tStartMoveTime) > MAX_FILTER_CHANGE_TIMEOUT)) {
                m_nTargetFilterSlot = m_nCurentFilterSlot; // to stop the queries
                return XA_COMMAND_FAILED;
            }
        }
    }

    nErr = filterWheelCommand("I2", szResp, SERIAL_BUFFER_SIZE);
    if(nErr) {
        return nErr;
    }
    // are we still moving ?
    if(strstr(szResp,"Moving")) {
        return XA_OK;
    }
    // check mTargetFilterIndex against current filter wheel position.
    rc = sscanf(szResp, "P%d", &nFilterSlot);
    if(rc == 0) {
        return XA_OK; // this is fine as this mean we got some other response and we can ignore it as the next call will catch the proper I2 response
    }

    if (m_nTargetFilterSlot == 0)
        m_nTargetFilterSlot = nFilterSlot;

    if(nFilterSlot == m_nTargetFilterSlot) {
        bComplete = true;
        m_nCurentFilterSlot = nFilterSlot;
    }
    tNow = time(NULL);
    if(!bComplete && ((tNow - m_tStartMoveTime) > MAX_FILTER_CHANGE_TIMEOUT)) {
        m_nTargetFilterSlot = nFilterSlot; // to stop the queries
        
        return XA_COMMAND_FAILED;
    }

    return nErr;
}

#pragma mark - Filter Wheel config commands

// increasePulseWidth

#pragma mark - filters and device params functions

int CXagyl::getNumbersOfSlots(int &nNbSlots)
{
    int nErr = XA_OK;

    if (m_bIsConnected) {
        nErr = getNumbersOfSlotsFromDevice(m_nNbSlot);
    }
    nNbSlots = m_nNbSlot;
    
    return nErr;
}

int CXagyl::getCurrentSlot(int &nSlot)
{
    int nErr = XA_OK;
    int rc = 0;
    char szResp[SERIAL_BUFFER_SIZE];

    nErr = filterWheelCommand("I2", szResp, SERIAL_BUFFER_SIZE);
    if(nErr) {
        return nErr;
    }
    rc = sscanf(szResp, "P%d", &nSlot);
    if(rc == 0) {
        return XA_COMMAND_FAILED;
    }

    return nErr;
}


int CXagyl::getSlotParams(int nSlotNumber, filter_params &Params)
{
    int nErr = XA_OK;
    int rc = 0;
    int nSlot;
    char szResp[SERIAL_BUFFER_SIZE];
    char szCmd[SERIAL_BUFFER_SIZE];

    // get current slot number
    nErr = filterWheelCommand("I2", szResp, SERIAL_BUFFER_SIZE);
    if(nErr)
        return nErr;
    rc = sscanf(szResp, "P%d", &nSlot);
    if(rc == 0) {
        return XA_COMMAND_FAILED;
    }

    // are we on the right slot ?
    while (nSlotNumber != nSlot) {
        // we need to move to the requested slot
        snprintf(szCmd, SERIAL_BUFFER_SIZE, "G%d", nSlotNumber);
        nErr = filterWheelCommand(szCmd, szResp, 0);
        if(nErr)
            return nErr;
        rc = sscanf(szResp, "P%d", &nSlot);
        if(rc == 0) {
            return XA_COMMAND_FAILED;
        }

    }

    // get position offset of current filter
    nErr = filterWheelCommand("I6", szResp, SERIAL_BUFFER_SIZE);
    if(nErr)
        return nErr;

    rc = sscanf(szResp, "P%d Offset %d", &nSlot, &Params.nOffset);
    if(rc == 0) {
        return XA_COMMAND_FAILED;
    }

    // get sensors LL and RR of current filter.
    nErr = filterWheelCommand("T0", szResp, SERIAL_BUFFER_SIZE);
    if(nErr)
        return nErr;

    rc = sscanf(szResp, "Sensors %d %d", &Params.nLL, &Params.nRR);
    if(rc == 0) {
        return XA_COMMAND_FAILED;
    }

    return nErr;
}

int CXagyl::setSlotParams(int nSlotNumber, int nOffset)
{
    int nErr = XA_OK;
    int i;
    filter_params Params;
    int nbDec;
    int nbInc;

    char resp[SERIAL_BUFFER_SIZE];

    // get slot params, this will move the the right slot if needed
    getSlotParams(nSlotNumber, Params);

    // set position offset
    if(Params.nOffset > nOffset) {
        nbDec = (Params.nOffset - nOffset);
        for(i = 0; i < nbDec; i++) {
            nErr = filterWheelCommand(")0", resp, SERIAL_BUFFER_SIZE);
            if(nErr)
                return nErr;
        }
    }
    else if (Params.nOffset < nOffset) {
        nbInc = (nOffset - Params.nOffset);
        for(i = 0; i < nbInc; i++) {
            nErr = filterWheelCommand("(0", resp, SERIAL_BUFFER_SIZE);
            if(nErr)
                return nErr;
        }
    }
    return nErr;
}

int CXagyl::getFilterWheelParams(wheel_params &FilterWheelParams)
{
    int nErr = XA_OK;
    int rc = 0;
    char szResp[SERIAL_BUFFER_SIZE];

    nErr = filterWheelCommand("I5", szResp, SERIAL_BUFFER_SIZE);
    if(nErr)
        return nErr;
    rc = sscanf(szResp, "Jitter %d", &FilterWheelParams.nJitter );
    if(rc == 0) {
        FilterWheelParams.nJitter = 1;
        return XA_COMMAND_FAILED;
    }

    nErr = filterWheelCommand("I9", szResp, SERIAL_BUFFER_SIZE);
    if(nErr)
        return nErr;

    rc = sscanf(szResp, "Pulse Width %dmS", &FilterWheelParams.nPulseWidth);
    if(rc == 0) {
        FilterWheelParams.nPulseWidth = 0;
        return XA_COMMAND_FAILED;
    }

    if(FilterWheelParams.nPulseWidth == 0){
        // no pulse width control.
        m_bHasPulseWidthControl = false;
    }
    else {
        m_bHasPulseWidthControl = true;
    }

    nErr = filterWheelCommand("I4", szResp, SERIAL_BUFFER_SIZE);
    if(nErr)
        return nErr;
    rc = sscanf(szResp, "MaxSpeed %d", &FilterWheelParams.nRotationSpeed );
    if(rc == 0) {
        FilterWheelParams.nRotationSpeed = 100;
        return XA_COMMAND_FAILED;
    }

    // get position threshiold of current filter.
    nErr = filterWheelCommand("I7", szResp, SERIAL_BUFFER_SIZE);
    if(nErr)
        return nErr;

    rc = sscanf(szResp, "Threshold %d", &FilterWheelParams.nThreshold);
    if(rc == 0) {
        return XA_COMMAND_FAILED;
    }

    return nErr;
}

int CXagyl::setFilterWheelParams(wheel_params FilterWheelParams)
{
    int nErr = XA_OK;
    int nbDec;
    int nbInc;
    int i;
    int nRotSpeed;
    int rc;
    char szCmd[SERIAL_BUFFER_SIZE];
    char szResp[SERIAL_BUFFER_SIZE];

    if(hasPulseWidthControl()) {
        if(m_WheelParams.nPulseWidth > FilterWheelParams.nPulseWidth) {
            nbDec = (m_WheelParams.nPulseWidth - FilterWheelParams.nPulseWidth);
            for(i = 0; i < nbDec; i++) {
                nErr = filterWheelCommand("N0", szResp, SERIAL_BUFFER_SIZE);
                if(nErr)
                    return nErr;
            }
        }
        else if (m_WheelParams.nPulseWidth < FilterWheelParams.nPulseWidth) {
            nbInc = (FilterWheelParams.nPulseWidth - m_WheelParams.nPulseWidth);
            for(i = 0; i < nbInc; i++) {
                nErr = filterWheelCommand("M0", szResp, SERIAL_BUFFER_SIZE);
                if(nErr)
                    return nErr;
            }
        }
    }

    snprintf(szCmd,SERIAL_BUFFER_SIZE, "S%X", FilterWheelParams.nRotationSpeed/10);
    nErr = filterWheelCommand(szCmd, szResp, SERIAL_BUFFER_SIZE);
    if(nErr)
        return nErr;
    rc = sscanf(szResp, "MaxSpeed %d", &nRotSpeed);
    if(rc == 0)
        return XA_COMMAND_FAILED;

    if(FilterWheelParams.nRotationSpeed != nRotSpeed) {
        // FW 4.2 and up has speed between 0 and 0xF instead of 0 to 0xA, so we're trying to approximate as the steps are the 6.66 and not 10 anymore
        snprintf(szCmd,SERIAL_BUFFER_SIZE, "S%X", (int)( ( (FilterWheelParams.nRotationSpeed/100.0f) *15)+ 0.5));
        nErr = filterWheelCommand(szCmd, szResp, SERIAL_BUFFER_SIZE);
        if(nErr)
            return nErr;
    }

    if(m_WheelParams.nJitter > FilterWheelParams.nJitter) {
        nbDec = (m_WheelParams.nJitter - FilterWheelParams.nJitter);
        for(i = 0; i < nbDec; i++) {
            nErr = filterWheelCommand("[0", szResp, SERIAL_BUFFER_SIZE);
            if(nErr)
                return nErr;
        }
    }
    else if (m_WheelParams.nJitter < FilterWheelParams.nJitter) {
        nbInc = (FilterWheelParams.nJitter - m_WheelParams.nJitter);
        for(i = 0; i < nbInc; i++) {
            nErr = filterWheelCommand("]0", szResp, SERIAL_BUFFER_SIZE);
            if(nErr)
                return nErr;
        }
    }

    // set Threshold
    if(m_WheelParams.nThreshold > FilterWheelParams.nThreshold) {
        nbDec = (m_WheelParams.nThreshold - FilterWheelParams.nThreshold);
        for(i = 0; i < nbDec; i++) {
            nErr = filterWheelCommand("{0", szResp, SERIAL_BUFFER_SIZE);
            if(nErr)
                return nErr;
        }
    }
    else if (m_WheelParams.nThreshold < FilterWheelParams.nThreshold) {
        nbInc = (FilterWheelParams.nThreshold - m_WheelParams.nThreshold);
        for(i = 0; i < nbInc; i++) {
            nErr = filterWheelCommand("}0", szResp, SERIAL_BUFFER_SIZE);
            if(nErr)
                return nErr;
        }
    }
    // update internal params structure
    getFilterWheelParams(m_WheelParams);
    return nErr;
}


int CXagyl::startCalibration()
{
    int nErr = XA_OK;
    char szResp[SERIAL_BUFFER_SIZE];

    nErr = filterWheelCommand("R6", szResp, SERIAL_BUFFER_SIZE);
    if(nErr)
        return nErr;
    
    if(!strstr(szResp,"Calibrating"))
        nErr = XA_COMMAND_FAILED;
    
    return nErr;
}

int CXagyl::isCalibrationComplete(bool &bComplete)
{
    int nErr = XA_OK;
    int nbWaitingByte;
    char szResp[SERIAL_BUFFER_SIZE];
    
    bComplete = false;
    
    // do we have any data waiting ?
    m_pSerx->bytesWaitingRx(nbWaitingByte);
    if (nbWaitingByte) {
        nErr = readResponse(szResp, SERIAL_BUFFER_SIZE);
        if(nErr)
            return nErr;
        if(strstr(szResp,"Done")) {
            bComplete = true;
            m_nCurentFilterSlot = 1; // calibration send us back to filter 1
        }
    }
    return nErr;
}


int CXagyl::resetAllToDefault(bool &bNeedCal)
{
    int nErr = XA_OK;
    int i;
    char szCmd[SERIAL_BUFFER_SIZE];
    char szResp[SERIAL_BUFFER_SIZE];

    bNeedCal = false;
    if(m_fFirmwareVersion >= 4.2) {
        // try R7 if available
        snprintf(szCmd,SERIAL_BUFFER_SIZE, "R%X", 7);
        nErr = filterWheelCommand(szCmd, szResp, SERIAL_BUFFER_SIZE);
        bNeedCal = true;
        return nErr;
    }
    
    // Pre 4.3 firmware do not have the R7 command.
    for (i=2;i<6;i++){
        snprintf(szCmd,SERIAL_BUFFER_SIZE, "R%X", i);
        nErr = filterWheelCommand(szCmd, szResp, SERIAL_BUFFER_SIZE);
        // we ignore the error for now.
    }
    nErr = getFilterWheelParams(m_WheelParams);

    return nErr;
}

bool CXagyl::hasPulseWidthControl()
{
    return m_bHasPulseWidthControl;
}


int CXagyl::getNumbersOfSlotsFromDevice(int &nNbSlots)
{
    int nErr = XA_OK;
    int rc = 0;
    char szResp[SERIAL_BUFFER_SIZE];

    nErr = filterWheelCommand("I8", szResp, SERIAL_BUFFER_SIZE);
    if(nErr) {
        if (m_bDebugLog) {
            snprintf(m_szLogBuffer,LOG_BUFFER_SIZE,"[CXagyl::getNumbersOfSlotsFromDevice] Error Getting response from filterWheelCommand.\n");
            m_pLogger->out(m_szLogBuffer);
        }
        return nErr;
    }
    // FilterSlots X
    rc = sscanf(szResp, "FilterSlots %d", &nNbSlots);
    if(rc == 0) {
        if (m_bDebugLog) {
            snprintf(m_szLogBuffer,LOG_BUFFER_SIZE,"[CXagyl::getNumbersOfSlotsFromDevice] Error converting response.\n");
            m_pLogger->out(m_szLogBuffer);
        }
        nErr = XA_COMMAND_FAILED;
    }
    return nErr;
}


void CXagyl::convertFirmwareToFloat(char *m_szFirmwareVersion)
{
    bool bFirstDot;
    int nLen;
    int i;
    int n;
    char szBuf[SERIAL_BUFFER_SIZE];

    memset(szBuf,0,SERIAL_BUFFER_SIZE);

    nLen = (int)strlen(m_szFirmwareVersion);
    // first remove all extra '.'
    n = 0;
    bFirstDot = true;
    for(i=0;i<nLen;i++) {
        if(m_szFirmwareVersion[i] != '.') {
            szBuf[n++] = m_szFirmwareVersion[i];
        }
        else if (bFirstDot) {
            szBuf[n++] = m_szFirmwareVersion[i];
            bFirstDot = false;
        }
    }
    // convert to float.
    m_fFirmwareVersion = (float)atof(szBuf);
}

void CXagyl::hexdump(unsigned char* szInputData, unsigned char *szOutBuffer, int nSize)
{
    unsigned char *szBuf = szOutBuffer;
    int nIdx=0;

    memset(szOutBuffer,0,nSize);
    for(nIdx=0; nIdx<nSize; nIdx++){
        snprintf((char *)szBuf,4,"%02X ", szInputData[nIdx]);
        szBuf+=3;
    }
    *szBuf = 0;
}


