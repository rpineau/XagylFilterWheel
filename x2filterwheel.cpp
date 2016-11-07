#include "x2filterwheel.h"


X2FilterWheel::X2FilterWheel(const char* pszDriverSelection,
				const int& nInstanceIndex,
				SerXInterface					* pSerX, 
				TheSkyXFacadeForDriversInterface	* pTheSkyX, 
				SleeperInterface					* pSleeper,
				BasicIniUtilInterface			* pIniUtil,
				LoggerInterface					* pLogger,
				MutexInterface					* pIOMutex,
				TickCountInterface				* pTickCount)
{
	m_nPrivateMulitInstanceIndex	= nInstanceIndex;
	m_pSerX							= pSerX;		
	m_pTheSkyXForMounts				= pTheSkyX;
	m_pSleeper						= pSleeper;
	m_pIniUtil						= pIniUtil;
	m_pLogger						= pLogger;	
	m_pIOMutex						= pIOMutex;
	m_pTickCount					= pTickCount;

    m_bLinked = false;

    Xagyl.SetSerxPointer(pSerX);
    Xagyl.setLogger(pLogger);

}

X2FilterWheel::~X2FilterWheel()
{
	if (m_pSerX)
		delete m_pSerX;
	if (m_pTheSkyXForMounts)
		delete m_pTheSkyXForMounts;
	if (m_pSleeper)
		delete m_pSleeper;
	if (m_pIniUtil)
		delete m_pIniUtil;
	if (m_pLogger)
		delete m_pLogger;
	if (m_pIOMutex)
		delete m_pIOMutex;
	if (m_pTickCount)
		delete m_pTickCount;

}


int	X2FilterWheel::queryAbstraction(const char* pszName, void** ppVal)
{
	X2MutexLocker ml(GetMutex());

	*ppVal = NULL;

    if (!strcmp(pszName, LoggerInterface_Name))
        *ppVal = GetLogger();
    else if (!strcmp(pszName, ModalSettingsDialogInterface_Name))
        *ppVal = dynamic_cast<ModalSettingsDialogInterface*>(this);
    else if (!strcmp(pszName, X2GUIEventInterface_Name))
        *ppVal = dynamic_cast<X2GUIEventInterface*>(this);
    else if (!strcmp(pszName, SerialPortParams2Interface_Name))
        *ppVal = dynamic_cast<SerialPortParams2Interface*>(this);

    return SB_OK;
}



#pragma mark - UI binding

int X2FilterWheel::execModalSettingsDialog()
{
    int nErr = SB_OK;
    int i = 0;
    int nbSlots = 0;
    char comboString[16];
    X2ModalUIUtil uiutil(this, GetTheSkyXFacadeForDrivers());
    X2GUIInterface*					ui = uiutil.X2UI();
    X2GUIExchangeInterface*			dx = NULL;//Comes after ui is loaded
    bool bPressedOK = false;
    char tmpBuf[SERIAL_BUFFER_SIZE];

    if (NULL == ui)
        return ERR_POINTER;

    if ((nErr = ui->loadUserInterface("XagylFilterWheel.ui", deviceType(), m_nPrivateMulitInstanceIndex)))
        return nErr;

    if (NULL == (dx = uiutil.X2DX()))
        return ERR_POINTER;

    //Intialize the user interface
    if(m_bLinked) {
        Xagyl.getModel(tmpBuf,16);
        dx->setPropertyString("model","text", tmpBuf);
        Xagyl.getFirmwareVersion(tmpBuf,16);
        dx->setPropertyString("firmware","text", tmpBuf);
        Xagyl.getSerialnumber(tmpBuf,16);
        dx->setPropertyString("serialNumber","text", tmpBuf);

        dx->setEnabled("pushButton",true);
        dx->setEnabled("pulseWidth",true);
        dx->setEnabled("rotationSpeed",true);
        dx->setEnabled("jitter",true);
        
        dx->setEnabled("comboBox",true);
        dx->setEnabled("positionOffset",true);
        dx->setEnabled("positionThreshold",true);


        //Populate the combo box and set the current index (selection)
        Xagyl.getNumbersOfSlots(nbSlots);
        printf("Got %d slot from Xagyl.getNumbersOfSlots\n", nbSlots);
        for (i=0; i< nbSlots; i++){
            snprintf(comboString, 16, "Slot %d", i);
            printf("Adding %s to combo\n", comboString);
            dx->comboBoxAppendString("comboBox", comboString);
            
        }
        dx->setCurrentIndex("comboBox",0);
        dx->setEnabled("positionOffset",true);
        dx->setEnabled("positionThreshold",true);

        
        snprintf(tmpBuf,16,"Sensors -- --");
        dx->setPropertyString("sensorValues","text", tmpBuf);

    }
    else {
        snprintf(tmpBuf,16,"NA");
        dx->setPropertyString("model","text", tmpBuf);
        dx->setPropertyString("firmware","text", tmpBuf);
        dx->setPropertyString("serialNumber","text", tmpBuf);
        dx->setEnabled("pushButton",false);
        dx->setEnabled("pulseWidth",false);
        dx->setEnabled("rotationSpeed",false);
        dx->setEnabled("jitter",false);
        dx->setEnabled("comboBox",false);
        dx->setEnabled("positionOffset",false);
        dx->setEnabled("positionThreshold",false);
        snprintf(tmpBuf,16,"Sensors -- --");
        dx->setPropertyString("sensorValues","text", tmpBuf);
    }

    X2MutexLocker ml(GetMutex());
    
    //Display the user interface
    if ((nErr = ui->exec(bPressedOK)))
        return nErr;

    //Retreive values from the user interface
    if (bPressedOK)
    {
        int nModel;

        //Model
        nModel = dx->currentIndex("comboBox");
    }

    return nErr;
}

void X2FilterWheel::uiEvent(X2GUIExchangeInterface* uiex, const char* pszEvent)
{

}

#pragma mark - LinkInterface

int	X2FilterWheel::establishLink(void)
{
    int err;
    char szPort[DRIVER_MAX_STRING];

    X2MutexLocker ml(GetMutex());
    // get serial port device name
    portNameOnToCharPtr(szPort,DRIVER_MAX_STRING);
    err = Xagyl.Connect(szPort);
    if(err)
        m_bLinked = false;
    else
        m_bLinked = true;

    return err;
}

int	X2FilterWheel::terminateLink(void)
{
    X2MutexLocker ml(GetMutex());
    Xagyl.Disconnect();
    m_bLinked = false;
    return SB_OK;
}

bool X2FilterWheel::isLinked(void) const
{
    X2FilterWheel* pMe = (X2FilterWheel*)this;
    X2MutexLocker ml(pMe->GetMutex());
    return m_bLinked;
}

bool X2FilterWheel::isEstablishLinkAbortable(void) const	{

    return false;
}


#pragma mark - AbstractDriverInfo

#define DISPLAY_NAME "X2 Xagyl Filter Wheel Plug In"
void	X2FilterWheel::driverInfoDetailedInfo(BasicStringInterface& str) const
{
	str = "X2 Xagyl Filter Wheel Plug In";
}

double	X2FilterWheel::driverInfoVersion(void) const
{
	return 1.0;
}

void X2FilterWheel::deviceInfoNameShort(BasicStringInterface& str) const
{
	str = "X2 Xagyl Filter Wheel";
}

void X2FilterWheel::deviceInfoNameLong(BasicStringInterface& str) const
{
	str = "X2 Xagyl Filter Wheel Plug In";

}

void X2FilterWheel::deviceInfoDetailedDescription(BasicStringInterface& str) const
{
	str = "X2 Xagyl Filter Wheel Plug In by Rodolphe Pineau";
	
}

void X2FilterWheel::deviceInfoFirmwareVersion(BasicStringInterface& str)
{
    if(m_bLinked) {
        X2MutexLocker ml(GetMutex());
        char cFirmware[SERIAL_BUFFER_SIZE];
        Xagyl.getFirmwareVersion(cFirmware, SERIAL_BUFFER_SIZE);
        str = cFirmware;
    }
    else
        str = "N/A";
}
void X2FilterWheel::deviceInfoModel(BasicStringInterface& str)				
{
    if(m_bLinked) {
        X2MutexLocker ml(GetMutex());
        char cModel[SERIAL_BUFFER_SIZE];
        Xagyl.getModel(cModel, SERIAL_BUFFER_SIZE);
        str = cModel;
    }
    else
        str = "N/A";
}

#pragma mark - FilterWheelMoveToInterface

int	X2FilterWheel::filterCount(int& nCount)
{
    int err = SB_OK;
    X2MutexLocker ml(GetMutex());
    err = Xagyl.getFilterCount(nCount);
    if(err) {
        err = ERR_CMDFAILED;
    }
    return err;
}

int	X2FilterWheel::defaultFilterName(const int& nIndex, BasicStringInterface& strFilterNameOut)
{
	X2MutexLocker ml(GetMutex());
	return ERR_NOT_IMPL;
}

int	X2FilterWheel::startFilterWheelMoveTo(const int& nTargetPosition)
{
    int err = SB_OK;
    
    if(m_bLinked) {
        X2MutexLocker ml(GetMutex());
        // nTargetPosition is 0 based, Xagyl wants 1 based position.
        err = Xagyl.moveToFilterIndex(nTargetPosition+1);
        if(err)
            err = ERR_CMDFAILED;
    }
    return err;
}

int	X2FilterWheel::isCompleteFilterWheelMoveTo(bool& bComplete) const
{
    int err = SB_OK;

    if(m_bLinked) {
        X2FilterWheel* pMe = (X2FilterWheel*)this;
        X2MutexLocker ml(pMe->GetMutex());
        err = pMe->Xagyl.isMoveToComplete(bComplete);
        if(err)
            err = ERR_CMDFAILED;
    }
    return err;
}

int	X2FilterWheel::endFilterWheelMoveTo(void)
{
	X2MutexLocker ml(GetMutex());
	return SB_OK;
}

int	X2FilterWheel::abortFilterWheelMoveTo(void)
{
	X2MutexLocker ml(GetMutex());
	return SB_OK;
}

#pragma mark -  SerialPortParams2Interface

void X2FilterWheel::portName(BasicStringInterface& str) const
{
    char szPortName[DRIVER_MAX_STRING];

    portNameOnToCharPtr(szPortName, DRIVER_MAX_STRING);

    str = szPortName;

}

void X2FilterWheel::setPortName(const char* szPort)
{
    if (m_pIniUtil)
        m_pIniUtil->writeString(PARENT_KEY, CHILD_KEY_PORTNAME, szPort);

}


void X2FilterWheel::portNameOnToCharPtr(char* pszPort, const int& nMaxSize) const
{
    if (NULL == pszPort)
        return;

    snprintf(pszPort, nMaxSize,DEF_PORT_NAME);

    if (m_pIniUtil)
        m_pIniUtil->readString(PARENT_KEY, CHILD_KEY_PORTNAME, pszPort, pszPort, nMaxSize);
    
}




