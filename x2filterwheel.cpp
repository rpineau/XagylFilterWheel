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
    m_nWheelState = IDLE;
    m_Xagyl.SetSerxPointer(pSerX);
    m_Xagyl.setLogger(pLogger);

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
    int nNbSlots = 0;
    char szComboString[16];
    X2ModalUIUtil uiutil(this, GetTheSkyXFacadeForDrivers());
    X2GUIInterface*					ui = uiutil.X2UI();
    X2GUIExchangeInterface*			dx = NULL;//Comes after ui is loaded
    bool bPressedOK = false;
    char szTmpBuf[SERIAL_BUFFER_SIZE];
    wheel_params FilterWheelParams;
    int nCurSlot = 0;
    int nTmpSlot = 0;
    int nTimeout = 0;
    bool bFilterChangeCompleted;

    if (NULL == ui)
        return ERR_POINTER;

    if ((nErr = ui->loadUserInterface("XagylFilterWheel.ui", deviceType(), m_nPrivateMulitInstanceIndex)))
        return nErr;

    if (NULL == (dx = uiutil.X2DX()))
        return ERR_POINTER;

    m_bUiEnabled = false;

    //Intialize the user interface
    if(m_bLinked) {
        // get global filter wheel params
        m_Xagyl.getFilterWheelParams(FilterWheelParams);
        m_Xagyl.getModel(szTmpBuf,16);
        dx->setPropertyString("model","text", szTmpBuf);
        m_Xagyl.getFirmwareVersion(szTmpBuf,16);
        dx->setPropertyString("firmware","text", szTmpBuf);
        m_Xagyl.getSerialnumber(szTmpBuf,16);
        dx->setPropertyString("serialNumber","text", szTmpBuf);

        if( m_Xagyl.hasPulseWidthControl()){
            dx->setEnabled("label_3",true);
            dx->setPropertyInt("pulseWidth", "value", FilterWheelParams.nPulseWidth);
        }
        else{
            dx->setEnabled("label_3",false);
            dx->setPropertyInt("pulseWidth", "value", 0);
        }

        enableWheelControls(dx, true);
        
        dx->setPropertyInt("rotationSpeed", "value", FilterWheelParams.nRotationSpeed);
        dx->setPropertyInt("jitter", "value", FilterWheelParams.nJitter);
        dx->setPropertyInt("positionThreshold", "value", FilterWheelParams.nThreshold);

        enableFilterControls(dx, true);

        //Populate the combo box and set the current index (selection)
        dx->invokeMethod("comboBox","clear");
        m_Xagyl.getNumbersOfSlots(nNbSlots);
        for (i=0; i< nNbSlots; i++){
            snprintf(szComboString, 16, "Slot %d", i+1);
            dx->comboBoxAppendString("comboBox", szComboString);
            
        }

        m_Xagyl.getCurrentSlot(nCurSlot); // we need it to restore the position after the settings are done.
        updateFilterControls(dx);
        m_nWheelState = IDLE;

    }
    else {
        snprintf(szTmpBuf,16,"NA");
        dx->setPropertyString("model","text", szTmpBuf);
        dx->setPropertyString("firmware","text", szTmpBuf);
        dx->setPropertyString("serialNumber","text", szTmpBuf);
        dx->setEnabled("pushButton",false);
        
        enableWheelControls(dx, false);
        enableFilterControls(dx, false);
        snprintf(szTmpBuf,16,"Sensors -- --");
        dx->setPropertyString("sensorValues","text", szTmpBuf);
    }

    X2MutexLocker ml(GetMutex());
    
    //Display the user interface
    m_bUiEnabled = true;
    if ((nErr = ui->exec(bPressedOK)))
        return nErr;
    m_bUiEnabled = false;

    //Retreive values from the user interface
    if (bPressedOK) {
        if(m_bLinked) {
            m_Xagyl.getCurrentSlot(nTmpSlot);
            if(nTmpSlot != nCurSlot) {
                // move back to nCurSlot as this is what TheSkyX think is selected
                m_Xagyl.moveToFilterIndex(nCurSlot);
                do {
                    m_Xagyl.isMoveToComplete(bFilterChangeCompleted);
                    if(bFilterChangeCompleted)
                        break; // no need to pause there :)
                    m_pSleeper->sleep(1000);
                    nTimeout++;
                    if (nTimeout > MAX_FILTER_CHANGE_TIMEOUT)
                        break;
                } while (!bFilterChangeCompleted);
            }
            
        }
    }

    return nErr;
}

void X2FilterWheel::uiEvent(X2GUIExchangeInterface* uiex, const char* pszEvent)
{
    int nErr = SB_OK;
    int filterCombBoxIndex;
    bool bFilterChangeCompleted;
    bool calibrationComplete;
    bool resetDefaultNeedsCal;
    wheel_params filterWheelParams;
    filter_params filterParams;

    // on_pushButton_clicked -> calibrate
    // on_pushButton_2_clicked -> apply on wheel settings
    // on_comboBox_currentIndexChanged -> filter change
    // on_pushButton_3_clicked -> apply filter slot changes
    // on_pushButton_4_clicked -> reset all to default

    // the test for m_bUiEnabled is done because even if the UI is not displayed we get events on the comboBox changes when we fill it.
    if(!m_bLinked | !m_bUiEnabled)
        return;

    switch(m_nWheelState)  {
        case MOVING :
            // are we done moving ?
            m_Xagyl.isMoveToComplete(bFilterChangeCompleted);
            if(bFilterChangeCompleted) {
                printf("filter change complete\n");
                m_nWheelState = IDLE;
                enableFilterControls(uiex, true);
                // update filter data
                updateFilterControls(uiex);
            }
            break;

        case CALIBRATING:
            nErr = m_Xagyl.isCalibrationComplete(calibrationComplete);
            if(nErr)
                return;
            if(calibrationComplete) {
                m_nWheelState = MOVING;
                enableWheelControls(uiex, true);
                uiex->setText("pushButton", "Calibrate");
                if(m_bResetingDefault) {
                    m_Xagyl.getFilterWheelParams(filterWheelParams);
                    if(m_Xagyl.hasPulseWidthControl()) {
                        uiex->propertyInt("pulseWidth", "value", filterWheelParams.nPulseWidth);
                    }
                    uiex->setPropertyInt("rotationSpeed", "value", filterWheelParams.nRotationSpeed);
                    uiex->setPropertyInt("jitter", "value", filterWheelParams.nJitter);
                    uiex->setPropertyInt("positionThreshold", "value", filterWheelParams.nThreshold);
                    m_bResetingDefault = false;
                }
            }
            break;

        case IDLE:
        default:
            break;
    }
    
    // Calibrate
    if (!strcmp(pszEvent, "on_pushButton_clicked")) {
        nErr = m_Xagyl.startCalibration();
        if (nErr)
            return;
        m_nWheelState = CALIBRATING;
        enableFilterControls(uiex, false);
        enableWheelControls(uiex, false);
        uiex->setText("pushButton", "Calibrating");
    }
    // apply wheel global seetings
    else if (!strcmp(pszEvent, "on_pushButton_2_clicked")) {
        if(m_Xagyl.hasPulseWidthControl()) {
            uiex->propertyInt("pulseWidth", "value", filterWheelParams.nPulseWidth);
        }
        uiex->propertyInt("rotationSpeed", "value", filterWheelParams.nRotationSpeed);
        uiex->propertyInt("jitter", "value", filterWheelParams.nJitter);
        uiex->propertyInt("positionThreshold", "value", filterWheelParams.nThreshold);
        nErr = m_Xagyl.setFilterWheelParams(filterWheelParams);
        if(nErr)
            return;
    }
    // apply filter settings.
    else if (!strcmp(pszEvent, "on_pushButton_3_clicked")) {
        filterCombBoxIndex = uiex->currentIndex("comboBox");
        uiex->propertyInt("positionOffset", "value", filterParams.nOffset);
        m_Xagyl.setSlotParams(filterCombBoxIndex+1, filterParams.nOffset);
    }
    // change filter
    else if (!strcmp(pszEvent, "on_comboBox_currentIndexChanged")) {
        filterCombBoxIndex = uiex->currentIndex("comboBox");
        m_Xagyl.moveToFilterIndex(filterCombBoxIndex+1);
        m_nWheelState = MOVING;
        enableFilterControls(uiex, false);
    }

    else if (!strcmp(pszEvent, "on_pushButton_4_clicked")) {
        nErr = m_Xagyl.resetAllToDefault(resetDefaultNeedsCal);
        if(nErr)
            return;
        if(resetDefaultNeedsCal) {
            printf("Resest to default is calibrating\n");
            m_bResetingDefault = true;
            m_nWheelState = CALIBRATING;
            enableFilterControls(uiex, false);
            enableWheelControls(uiex, false);
            uiex->setText("pushButton", "Calibrating");
        }
        else {
            m_nWheelState = IDLE;
            updateFilterControls(uiex);
            m_Xagyl.getFilterWheelParams(filterWheelParams);
            if(m_Xagyl.hasPulseWidthControl()) {
                uiex->propertyInt("pulseWidth", "value", filterWheelParams.nPulseWidth);
            }
            uiex->setPropertyInt("rotationSpeed", "value", filterWheelParams.nRotationSpeed);
            uiex->setPropertyInt("jitter", "value", filterWheelParams.nJitter);
            uiex->setPropertyInt("positionThreshold", "value", filterWheelParams.nThreshold);
        }
    }
}

void X2FilterWheel::enableFilterControls(X2GUIExchangeInterface* dx, bool bEnable) {

    if(bEnable) {
        dx->setEnabled("comboBox",true);
        dx->setEnabled("positionOffset",true);
        dx->setEnabled("pushButton_3",true);
    }
    else {
        dx->setEnabled("comboBox",false);
        dx->setEnabled("positionOffset",false);
        dx->setEnabled("pushButton_3",false);
    }
}

void X2FilterWheel::enableWheelControls(X2GUIExchangeInterface* dx, bool bEnable)
{
    if(bEnable) {
        dx->setEnabled("pushButton",true);
        dx->setEnabled("pushButton_4",true);
        if( m_Xagyl.hasPulseWidthControl())
            dx->setEnabled("pulseWidth",true);
        dx->setEnabled("rotationSpeed",true);
        dx->setEnabled("jitter",true);
        dx->setEnabled("positionThreshold",true);
        dx->setEnabled("pushButton_2",true);
    }
    else {
        dx->setEnabled("pushButton",false);
        dx->setEnabled("pushButton_4",false);
        if( m_Xagyl.hasPulseWidthControl())
            dx->setEnabled("pulseWidth",false);
        dx->setEnabled("rotationSpeed",false);
        dx->setEnabled("jitter",false);
        dx->setEnabled("positionThreshold",false);
        dx->setEnabled("pushButton_2",false);

    }
}
void X2FilterWheel::updateFilterControls(X2GUIExchangeInterface* dx)
{
    int nCurSlot = 0;
    filter_params   FilterParams;
    char szTmpBuf[SERIAL_BUFFER_SIZE];

    // what is the current filter ?
    m_Xagyl.getCurrentSlot(nCurSlot);
    if(nCurSlot == 0)
        return;

    dx->setCurrentIndex("comboBox",nCurSlot-1);
    // get filter params
    m_Xagyl.getSlotParams(nCurSlot, FilterParams);
    dx->setEnabled("positionOffset",true);
    dx->setPropertyInt("positionOffset", "value", FilterParams.nOffset);
    snprintf(szTmpBuf,16,"Sensors %d %d", FilterParams.nLL, FilterParams.nRR);
    dx->setPropertyString("sensorValues","text", szTmpBuf);

}

#pragma mark - LinkInterface

int	X2FilterWheel::establishLink(void)
{
    int nErr;
    char szPort[DRIVER_MAX_STRING];

    X2MutexLocker ml(GetMutex());
    // get serial port device name
    portNameOnToCharPtr(szPort,DRIVER_MAX_STRING);
    nErr = m_Xagyl.Connect(szPort);
    if(nErr)
        m_bLinked = false;
    else
        m_bLinked = true;

    return nErr;
}

int	X2FilterWheel::terminateLink(void)
{
    X2MutexLocker ml(GetMutex());
    m_Xagyl.Disconnect();
    m_bLinked = false;
    return SB_OK;
}

bool X2FilterWheel::isLinked(void) const
{
    X2FilterWheel* pMe = (X2FilterWheel*)this;
    X2MutexLocker ml(pMe->GetMutex());
    return pMe->m_bLinked;
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
        m_Xagyl.getFirmwareVersion(cFirmware, SERIAL_BUFFER_SIZE);
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
        m_Xagyl.getModel(cModel, SERIAL_BUFFER_SIZE);
        str = cModel;
    }
    else
        str = "N/A";
}

#pragma mark - FilterWheelMoveToInterface

int	X2FilterWheel::filterCount(int& nCount)
{
    int nErr = SB_OK;
    X2MutexLocker ml(GetMutex());
    nErr = m_Xagyl.getFilterCount(nCount);
    if(nErr) {
        nErr = ERR_CMDFAILED;
    }
    return nErr;
}

int	X2FilterWheel::defaultFilterName(const int& nIndex, BasicStringInterface& strFilterNameOut)
{
	X2MutexLocker ml(GetMutex());
	strFilterNameOut = "";
	return SB_OK;
}

int	X2FilterWheel::startFilterWheelMoveTo(const int& nTargetPosition)
{
    int nErr = SB_OK;
    
    if(m_bLinked) {
        X2MutexLocker ml(GetMutex());
        // nTargetPosition is 0 based, Xagyl wants 1 based position.
        nErr = m_Xagyl.moveToFilterIndex(nTargetPosition+1);
        if(nErr)
            nErr = ERR_CMDFAILED;
    }
    return nErr;
}

int	X2FilterWheel::isCompleteFilterWheelMoveTo(bool& bComplete) const
{
    int nErr = SB_OK;

    if(m_bLinked) {
        X2FilterWheel* pMe = (X2FilterWheel*)this;
        X2MutexLocker ml(pMe->GetMutex());
        nErr = pMe->m_Xagyl.isMoveToComplete(bComplete);
        if(nErr)
            nErr = ERR_CMDFAILED;
    }
    return nErr;
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




