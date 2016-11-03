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
    mNbSlot = 0;
}

CXagyl::~CXagyl()
{

}

int CXagyl::Connect(const char *szPort)
{
    int err = SB_OK;
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

    // if any of this fails we're not properly connected or there is a hardware issue.
    
    err = getFirmwareVersion(firmwareVersion, SERIAL_BUFFER_SIZE);
    if(err) {
        if (bDebugLog) {
            snprintf(mLogBuffer,LOG_BUFFER_SIZE,"[Xagyl::Connect] Error Getting Firmware.\n");
            mLogger->out(mLogBuffer);
        }
        bIsConnected = false;
        pSerx->close();
        return FIRMWARE_NOT_SUPPORTED;
    }

    if (bDebugLog) {
        snprintf(mLogBuffer,LOG_BUFFER_SIZE,"[Xagyl::Connect] Got Firmware.\n");
        mLogger->out(mLogBuffer);
    }

    // get the number of slots
    err = getNumbersOfSlots(mNbSlot);
    if(err) {
        if (bDebugLog) {
            snprintf(mLogBuffer,LOG_BUFFER_SIZE,"[Xagyl::Connect] Error Getting the number of available slots.\n");
            mLogger->out(mLogBuffer);
        }
        bIsConnected = false;
        pSerx->close();
        return ERR_CMDFAILED;
    }
    
    // get jitter and pulse width
    err = getGlobalPraramsFromDevice(mWheelParams);
    if(err) {
        if (bDebugLog) {
            snprintf(mLogBuffer,LOG_BUFFER_SIZE,"[Xagyl::Connect] Error Getting the jitter and pulse width values.\n");
            mLogger->out(mLogBuffer);
        }
        bIsConnected = false;
        pSerx->close();
        return ERR_CMDFAILED;
    }

    mFilterParams = new filter_params[mNbSlot];
    err = getFiltersPraramsFromDevice(mFilterParams, mNbSlot);
    if(err) {
        if (bDebugLog) {
            snprintf(mLogBuffer,LOG_BUFFER_SIZE,"[Xagyl::Connect] Error Getting the filters parameters.\n");
            mLogger->out(mLogBuffer);
        }
        bIsConnected = false;
        pSerx->close();
        return ERR_CMDFAILED;
    }
    return err;
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
    } while (*bufPtr++ != 0xA && totalBytesRead < bufferLen );

    *bufPtr = 0; //remove the 0xA
    return err;
}


int CXagyl::filterWheelCommand(const char *cmd, char *result, int resultMaxLen)
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

    if(result) {
        // read response
        if (bDebugLog) {
            snprintf(mLogBuffer,LOG_BUFFER_SIZE,"[CXagyl::domeCommand] Getting response.\n");
            mLogger->out(mLogBuffer);
        }
        err = readResponse(resp, SERIAL_BUFFER_SIZE);
        if(err)
            return err;

        if(result)
            strncpy(result, &resp[1], resultMaxLen);
    }
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

    err = filterWheelCommand("I1", resp, SERIAL_BUFFER_SIZE);
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

    err = filterWheelCommand("I0", resp, SERIAL_BUFFER_SIZE);
    if(err)
        return err;

    strncpy(model, resp, strMaxLen);
    return err;
}

int CXagyl::getFilterCount(int &count)
{
    int err = 0;
    char resp[SERIAL_BUFFER_SIZE];
    count = 0;
    err = filterWheelCommand("I8", resp, SERIAL_BUFFER_SIZE);
    if(err)
        return err;
    return err;
}


#pragma mark - Filter Wheel move commands

int CXagyl::moveToFilterIndex(int nTargetPosition)
{
    int err = 0;
    
    err = filterWheelCommand("Gx", NULL, 0);
    if(err)
        return err;
    mTargetFilterIndex = nTargetPosition;
    return err;

}

int CXagyl::isMoveToComplete(bool &complete)
{
    int err = 0;
    int filterIndex;
    char resp[SERIAL_BUFFER_SIZE];
    
    complete = false;
    
    err = filterWheelCommand("I2", resp, SERIAL_BUFFER_SIZE);
    if(err)
        return err;
    // check mTargetFilterIndex against current filter wheel position.
    err = sscanf(resp, "P%d", &filterIndex);
    if(err)
        return err;
    
    if(filterIndex == mTargetFilterIndex)
        complete = true;

    return err;
}

#pragma mark - Filter Wheel config commands

// increasePulseWidth

#pragma mark - filters and device params functions

int CXagyl::getNumbersOfSlots(int &nbSlots)
{
    int err = 0;
    
    return err;
}

int CXagyl::getFilterParams(int index, filter_params &params)
{
    int err = 0;
    
    return err;
}

int CXagyl::getFiltersPraramsFromDevice(filter_params *filterParams, int nbSlots)
{
    int err = 0;
    int i = 0;
    
    for(i = 0; i < nbSlots; i++){
        filterParams[i].offset = 0;
        filterParams[i].threshold = 0;
    }

    return err;
}

int CXagyl::setFilterParamsOnDevice(int fiterIndex, int offset, int threshold)
{
    int err = 0;
    
    return err;
}

int CXagyl::getGlobalPraramsFromDevice(wheel_params &wheelParams)
{
    int err = SB_OK;
    
    return err;
}




