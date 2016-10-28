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
    bIsConnected = false;
    bCalibrating = false;
    bDebugLog = false;
    mTargetFilterIndex = 0;
}

CXagyl::~CXagyl()
{

}

int CXagyl::Connect(const char *szPort)
{
    int err;

    // 9600 8N1
    if(pSerx->open(szPort, 9600, SerXInterface::B_NOPARITY, "-DTR_CONTROL 1") == 0)
        bIsConnected = true;
    else
        bIsConnected = false;

    if(!bIsConnected)
        return ERR_COMMNOLINK;

    if (bDebugLog) {
        snprintf(mLogBuffer,LOG_BUFFER_SIZE,"[Xagyl::Connect] Connected.\n");
        mLogger->out(mLogBuffer);

        snprintf(mLogBuffer,LOG_BUFFER_SIZE,"[Xagyl::Connect] Getting Firmware.\n");
        mLogger->out(mLogBuffer);
    }
    // if this fails we're not properly connected.
    /*
    err = getFirmwareVersion(firmwareVersion, SERIAL_BUFFER_SIZE);
    if(err) {
        if (bDebugLog) {
            snprintf(mLogBuffer,LOG_BUFFER_SIZE,"[CNexDome::Connect] Error Getting Firmware.\n");
            mLogger->out(mLogBuffer);
        }
        bIsConnected = false;
        pSerx->close();
        return FIRMWARE_NOT_SUPPORTED;
    }

    if (bDebugLog) {
        snprintf(mLogBuffer,LOG_BUFFER_SIZE,"[CNexDome::Connect] Got Firmware.\n");
        mLogger->out(mLogBuffer);
    }
    */
    return SB_OK;
}



void CXagyl::Disconnect()
{
    if(bIsConnected) {
        pSerx->purgeTxRx();
        pSerx->close();
    }
    bIsConnected = false;
}


#pragma mark - communication functions
int CXagyl::readResponse(char *respBuffer, int bufferLen)
{
    int err = XA_OK;
    unsigned long nBytesRead = 0;
    unsigned long totalBytesRead = 0;
    char *bufPtr;

    memset(respBuffer, 0, (size_t) bufferLen);
    bufPtr = respBuffer;

    do {
        err = pSerx->readFile(bufPtr, 1, nBytesRead, MAX_TIMEOUT);
        if(err) {
            if (bDebugLog) {
                snprintf(mLogBuffer,LOG_BUFFER_SIZE,"[CXagyl::readResponse] readFile error.\n");
                mLogger->out(mLogBuffer);
            }
            return err;
        }

        if (bDebugLog) {
            snprintf(mLogBuffer,LOG_BUFFER_SIZE,"[CXagyl::readResponse] respBuffer = %s\n",respBuffer);
            mLogger->out(mLogBuffer);
        }

        if (nBytesRead !=1) {// timeout
            if (bDebugLog) {
                snprintf(mLogBuffer,LOG_BUFFER_SIZE,"[CXagyl::readResponse] readFile Timeout.\n");
                mLogger->out(mLogBuffer);
            }
            err = XA_BAD_CMD_RESPONSE;
            break;
        }
        totalBytesRead += nBytesRead;
        if (bDebugLog) {
            snprintf(mLogBuffer,LOG_BUFFER_SIZE,"[CXagyl::readResponse] nBytesRead = %lu\n",nBytesRead);
            mLogger->out(mLogBuffer);
        }
    } while (*bufPtr++ != '\n' && totalBytesRead < bufferLen );

    *bufPtr = 0; //remove the \n
    return err;
}


int CXagyl::filterWheelCommand(const char *cmd, char *result, char respCmdCode, int resultMaxLen)
{
    int err = 0;
    char resp[SERIAL_BUFFER_SIZE];
    unsigned long  nBytesWrite;

    pSerx->purgeTxRx();
    if (bDebugLog) {
        snprintf(mLogBuffer,LOG_BUFFER_SIZE,"[CXagyl::domeCommand] Sending %s\n",cmd);
        mLogger->out(mLogBuffer);
    }
    err = pSerx->writeFile((void *)cmd, strlen(cmd), nBytesWrite);
    if(err)
        return err;
    // read response
    if (bDebugLog) {
        snprintf(mLogBuffer,LOG_BUFFER_SIZE,"[CXagyl::domeCommand] Getting response.\n");
        mLogger->out(mLogBuffer);
    }
    err = readResponse(resp, SERIAL_BUFFER_SIZE);
    if(err)
        return err;

    if(resp[0] != respCmdCode)
        err = XA_BAD_CMD_RESPONSE;

    if(result)
        strncpy(result, &resp[1], resultMaxLen);
    
    return err;
    
}

#pragma mark - Filter Wheel info commands

int CXagyl::getFirmwareVersion(char *version, int strMaxLen)
{
    int err = 0;
    char resp[SERIAL_BUFFER_SIZE];

    if(!bIsConnected)
        return XA_NOT_CONNECTED;

    if(bCalibrating)
        return SB_OK;

    // err = filterWheelCommand("I1", resp, 'V', SERIAL_BUFFER_SIZE);
    if(err)
        return err;

    strncpy(version, resp, strMaxLen);
    return err;
}

int CXagyl::getModel(char *model, int strMaxLen)
{
    int err = 0;
    char resp[SERIAL_BUFFER_SIZE];

    if(!bIsConnected)
        return XA_NOT_CONNECTED;

    if(bCalibrating)
        return SB_OK;

    // err = filterWheelCommand("I0", resp, 'V', SERIAL_BUFFER_SIZE);
    if(err)
        return err;

    strncpy(model, resp, strMaxLen);
    return err;
}

int CXagyl::getFilterCount(int &count)
{
    int err = 0;
    count = 0;
    // err = filterWheelCommand("I8", resp, 'V', SERIAL_BUFFER_SIZE);
    if(err)
        return err;

    return err;
}


#pragma mark - Filter Wheel move commands

int CXagyl::moveToFilterIndex(int nTargetPosition)
{
    int err = 0;

    // err = filterWheelCommand("Gx", resp, 'V', SERIAL_BUFFER_SIZE);
    if(err)
        return err;
    mTargetFilterIndex = nTargetPosition;
    return err;

}

int CXagyl::isMoveToComplete(bool &complete)
{
    int err = 0;
    // err = filterWheelCommand("I2", resp, 'V', SERIAL_BUFFER_SIZE);
    if(err)
        return err;
    // check mTargetFilterIndex against current filter wheel position.
    return err;
}

#pragma mark - Filter Wheel config commands

// increasePulseWidth


