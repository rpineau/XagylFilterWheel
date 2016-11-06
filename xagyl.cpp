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
    mTargetFilterSlot = 0;
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
    if (bDebugLog) {
        snprintf(mLogBuffer,LOG_BUFFER_SIZE,"[Xagyl::Connect] Got number of slots : %d.\n", mNbSlot);
        mLogger->out(mLogBuffer);
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
    if (bDebugLog) {
        snprintf(mLogBuffer,LOG_BUFFER_SIZE,"[Xagyl::Connect] Got number of jitter and pulse width\n");
        mLogger->out(mLogBuffer);
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
            snprintf(mLogBuffer,LOG_BUFFER_SIZE,"[CXagyl::readResponse] totalBytesRead = %lu\n",totalBytesRead);
            mLogger->out(mLogBuffer);
        }
    } while (*bufPtr++ != 0xA && totalBytesRead < bufferLen );

    *(bufPtr-2) = 0; //remove the 0xD
    *(bufPtr-1) = 0; //remove the 0xA
    return err;
}


int CXagyl::filterWheelCommand(const char *cmd, char *result, int resultMaxLen)
{
    int err = XA_OK;
    char resp[SERIAL_BUFFER_SIZE];
    unsigned long  nBytesWrite;

    pSerx->purgeTxRx();
    if (bDebugLog) {
        snprintf(mLogBuffer,LOG_BUFFER_SIZE,"[CXagyl::filterWheelCommand] Sending %s\n",cmd);
        mLogger->out(mLogBuffer);
    }
    err = pSerx->writeFile((void *)cmd, strlen(cmd), nBytesWrite);
    if(err){
        if (bDebugLog) {
            snprintf(mLogBuffer,LOG_BUFFER_SIZE,"[CXagyl::filterWheelCommand] writeFile error.\n");
            mLogger->out(mLogBuffer);
        }
        return err;
    }

    if(result) {
        // read response
        if (bDebugLog) {
            snprintf(mLogBuffer,LOG_BUFFER_SIZE,"[CXagyl::filterWheelCommand] Getting response.\n");
            mLogger->out(mLogBuffer);
        }
        err = readResponse(resp, SERIAL_BUFFER_SIZE);
        if(err){
            if (bDebugLog) {
                snprintf(mLogBuffer,LOG_BUFFER_SIZE,"[CXagyl::filterWheelCommand] readResponse error.\n");
                mLogger->out(mLogBuffer);
            }
        }
        strncpy(result, resp, resultMaxLen);
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
    if(err) {
        if (bDebugLog) {
            snprintf(mLogBuffer,LOG_BUFFER_SIZE,"[CXagyl::getFirmwareVersion] Error Getting response from filterWheelCommand.\n");
            mLogger->out(mLogBuffer);
        }
        return err;
    }
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

int CXagyl::getSerialnumber(char *serialNumber, int strMaxLen)
{
    int err = 0;
    char resp[SERIAL_BUFFER_SIZE];
    
    if(!bIsConnected)
        return XA_NOT_CONNECTED;
    
    if(bCalibrating)
        return SB_OK;
    
    err = filterWheelCommand("I3", resp, SERIAL_BUFFER_SIZE);
    if(err)
        return err;
    
    strncpy(serialNumber, resp, strMaxLen);
    return err;
}


int CXagyl::getFilterCount(int &count)
{
    int err = SB_OK;
    
    if (bIsConnected)
        err = getNumbersOfSlotsFromDevice(count);
    else
        count = mNbSlot;
    return err;
}


#pragma mark - Filter Wheel move commands

int CXagyl::moveToFilterIndex(int nTargetPosition)
{
    int err = 0;
    char cmd[SERIAL_BUFFER_SIZE];
    
    snprintf(cmd,SERIAL_BUFFER_SIZE, "G%d", nTargetPosition);
    err = filterWheelCommand(cmd, NULL, 0);
    if(err)
        return err;
    mTargetFilterSlot = nTargetPosition;
    return err;

}

int CXagyl::isMoveToComplete(bool &complete)
{
    int err = SB_OK;
    int rc = 0;
    int filterSlot;
    char resp[SERIAL_BUFFER_SIZE];
    
    complete = false;

    err = filterWheelCommand("I2", resp, SERIAL_BUFFER_SIZE);
    if(err)
        return err;
    // check mTargetFilterIndex against current filter wheel position.
    rc = sscanf(resp, "P%d", &filterSlot);
    if(rc == 0) {
        return XA_COMMAND_FAILED;
    }

    if (mTargetFilterSlot == 0)
        mTargetFilterSlot = filterSlot;

    if(filterSlot == mTargetFilterSlot)
        complete = true;
    // TheSkyX makes to many request to fast.. need to slow it down.
    usleep(200000);

    return err;
}

#pragma mark - Filter Wheel config commands

// increasePulseWidth

#pragma mark - filters and device params functions

int CXagyl::getNumbersOfSlots(int &nbSlots)
{
    int err = SB_OK;
    if (bIsConnected) {
        err = getNumbersOfSlotsFromDevice(mNbSlot);
    }
    nbSlots = mNbSlot;
    
    return err;
}

int CXagyl::getFilterParams(int index, filter_params &params)
{
    int err = 0;
    if(index<mNbSlot) {
        params.offset = mFilterParams[index].offset;
        params.threshold = mFilterParams[index].threshold;
    }
    return err;
}

int CXagyl::getFiltersPraramsFromDevice(filter_params *filterParams, int nbSlots)
{
    int err = SB_OK;
    int i = 0;
    
    for(i = 0; i < nbSlots; i++){
        filterParams[i].offset = 0;
        filterParams[i].threshold = 0;
    }

    return err;
}

int CXagyl::setFilterParamsOnDevice(int fiterIndex, int offset, int threshold)
{
    int err = SB_OK;
    
    return err;
}

int CXagyl::getGlobalPraramsFromDevice(wheel_params &wheelParams)
{
    int err = SB_OK;
    
    return err;
}

int CXagyl::getNumbersOfSlotsFromDevice(int &nbSlots)
{
    int err = SB_OK;
    int rc = 0;
    char resp[SERIAL_BUFFER_SIZE];
    err = filterWheelCommand("I8", resp, SERIAL_BUFFER_SIZE);
    if(err) {
        if (bDebugLog) {
            snprintf(mLogBuffer,LOG_BUFFER_SIZE,"[CXagyl::getNumbersOfSlotsFromDevice] Error Getting response from filterWheelCommand.\n");
            mLogger->out(mLogBuffer);
        }
        return err;
    }
    // FilterSlots X
    rc = sscanf(resp, "FilterSlots %d", &nbSlots);
    if(rc == 0) {
        if (bDebugLog) {
            snprintf(mLogBuffer,LOG_BUFFER_SIZE,"[CXagyl::getNumbersOfSlotsFromDevice] Error converting response.\n");
            mLogger->out(mLogBuffer);
        }
        err = XA_COMMAND_FAILED;
    }
    return err;
}



void CXagyl::hexdump(unsigned char* inputData, unsigned char *outBuffer, int size)
{
    unsigned char *buf = outBuffer;
    int idx=0;
    memset(outBuffer,0,size);
    for(idx=0; idx<size; idx++){
        
        snprintf((char *)buf,4,"%02X ", inputData[idx]);
        buf+=3;
    }
    *buf = 0;
}


