//
//  xagyl.cpp
//  XagylFilterWheel
//
//  Created by Rodolphe Pineau on 26/10/16.
//  Copyright © 2016 RTI-Zone. All rights reserved.
//

#include <stdio.h>


#include "xagyl.h"
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <memory.h>
#ifdef SB_MAC_BUILD
#include <unistd.h>
#endif

CXagyl::CXagyl()
{

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
        snprintf(mLogBuffer,XAGYL_LOG_BUFFER_SIZE,"[Xagyl::Connect] Connected.\n");
        mLogger->out(mLogBuffer);

        snprintf(mLogBuffer,XAGYL_LOG_BUFFER_SIZE,"[Xagyl::Connect] Getting Firmware.\n");
        mLogger->out(mLogBuffer);
    }
    // if this fails we're not properly connected.
    /*
    err = getFirmwareVersion(firmwareVersion, SERIAL_BUFFER_SIZE);
    if(err) {
        if (bDebugLog) {
            snprintf(mLogBuffer,XAGYL_LOG_BUFFER_SIZE,"[CNexDome::Connect] Error Getting Firmware.\n");
            mLogger->out(mLogBuffer);
        }
        bIsConnected = false;
        pSerx->close();
        return FIRMWARE_NOT_SUPPORTED;
    }

    if (bDebugLog) {
        snprintf(mLogBuffer,XAGYL_LOG_BUFFER_SIZE,"[CNexDome::Connect] Got Firmware.\n");
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


