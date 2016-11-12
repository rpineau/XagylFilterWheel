#pragma once
#include <string.h>
#include <stdio.h>
#include "xagyl.h"
#include "../../licensedinterfaces/filterwheeldriverinterface.h"
#include "../../licensedinterfaces/modalsettingsdialoginterface.h"
#include "../../licensedinterfaces/x2guiinterface.h"
#include "../../licensedinterfaces/serialportparams2interface.h"
#include "../../licensedinterfaces/sberrorx.h"
#include "../../licensedinterfaces/basicstringinterface.h"
#include "../../licensedinterfaces/serxinterface.h"
#include "../../licensedinterfaces/basiciniutilinterface.h"
#include "../../licensedinterfaces/theskyxfacadefordriversinterface.h"
#include "../../licensedinterfaces/sleeperinterface.h"
#include "../../licensedinterfaces/loggerinterface.h"
#include "../../licensedinterfaces/basiciniutilinterface.h"
#include "../../licensedinterfaces/mutexinterface.h"
#include "../../licensedinterfaces/tickcountinterface.h"




// Forward declare the interfaces that the this driver is "given" by TheSkyX
class SerXInterface;
class TheSkyXFacadeForDriversInterface;
class SleeperInterface;
class BasicIniUtilInterface;
class LoggerInterface;
class MutexInterface;
class BasicIniUtilInterface;
class TickCountInterface;

#define DRIVER_VERSION      1.0

#define PARENT_KEY			"XagylFilterWheel"
#define CHILD_KEY_PORTNAME	"PortName"


#if defined(SB_WIN_BUILD)
#define DEF_PORT_NAME					"COM1"
#elif defined(SB_MAC_BUILD)
#define DEF_PORT_NAME					"/dev/cu.KeySerial1"
#elif defined(SB_LINUX_BUILD)
#define DEF_PORT_NAME					"/dev/COM0"
#endif

#define LOG_BUFFER_SIZE 256

enum wheelState { IDLE = 0, MOVING, CALIBRATING};

/*!
\brief The X2FilterWheel example.

\ingroup Example

Use this example to write an X2FilterWheel driver.

Utilizes  ModalSettingsDialogInterface and  X2GUIEventInterface for a basic, cross-platform, user interface.
*/
class X2FilterWheel : public FilterWheelDriverInterface, public SerialPortParams2Interface, public ModalSettingsDialogInterface, public X2GUIEventInterface
{
public:
	/*!Standard X2 constructor*/
	X2FilterWheel(const char* pszDriverSelection,
				const int& nInstanceIndex,
				SerXInterface					* pSerX, 
				TheSkyXFacadeForDriversInterface	* pTheSkyX, 
				SleeperInterface					* pSleeper,
				BasicIniUtilInterface			* pIniUtil,
				LoggerInterface					* pLogger,
				MutexInterface					* pIOMutex,
				TickCountInterface				* pTickCount);

	~X2FilterWheel();

// Operations
public:

	/*!\name DriverRootInterface Implementation
	See DriverRootInterface.*/
	//@{ 
	virtual DeviceType							deviceType(void)							  {return DriverRootInterface::DT_FILTERWHEEL;}
	virtual int									queryAbstraction(const char* pszName, void** ppVal) ;
	//@} 

	/*!\name LinkInterface Implementation
	See LinkInterface.*/
	//@{ 
	virtual int									establishLink(void)						;
	virtual int									terminateLink(void)						;
	virtual bool								isLinked(void) const					;
	virtual bool								isEstablishLinkAbortable(void) const	;
	//@} 

	/*!\name DriverInfoInterface Implementation
	See DriverInfoInterface.*/
	//@{ 
	virtual void								driverInfoDetailedInfo(BasicStringInterface& str) const;
	virtual double								driverInfoVersion(void) const				;
	//@} 

	/*!\name HardwareInfoInterface Implementation
	See HardwareInfoInterface.*/
	//@{ 
	virtual void deviceInfoNameShort(BasicStringInterface& str) const				;
	virtual void deviceInfoNameLong(BasicStringInterface& str) const				;
	virtual void deviceInfoDetailedDescription(BasicStringInterface& str) const	;
	virtual void deviceInfoFirmwareVersion(BasicStringInterface& str)				;
	virtual void deviceInfoModel(BasicStringInterface& str)						;
	//@} 

	/*!\name FilterWheelMoveToInterface Implementation
	See FilterWheelMoveToInterface.*/
	//@{ 
	virtual int									filterCount(int& nCount)							;
	virtual int									defaultFilterName(const int& nIndex, BasicStringInterface& strFilterNameOut);
	virtual int									startFilterWheelMoveTo(const int& nTargetPosition)	;
	virtual int									isCompleteFilterWheelMoveTo(bool& bComplete) const	;
	virtual int									endFilterWheelMoveTo(void)							;
	virtual int									abortFilterWheelMoveTo(void)						;
	//@}

//
	/*!\name ModalSettingsDialogInterface Implementation
	See ModalSettingsDialogInterface.*/
	//@{ 
	virtual int								initModalSettingsDialog(void){return 0;}
	virtual int								execModalSettingsDialog(void);
	//@} 

    //SerialPortParams2Interface
    virtual void			portName(BasicStringInterface& str) const			;
    virtual void			setPortName(const char* szPort)						;
    virtual unsigned int	baudRate() const			{return 9600;};
    virtual void			setBaudRate(unsigned int)	{};
    virtual bool			isBaudRateFixed() const		{return true;}

    virtual SerXInterface::Parity	parity() const				{return SerXInterface::B_NOPARITY;}
    virtual void					setParity(const SerXInterface::Parity& parity){parity;};
    virtual bool					isParityFixed() const		{return true;}

	//
	/*!\name X2GUIEventInterface Implementation
	See X2GUIEventInterface.*/
	//@{ 
	virtual void uiEvent(X2GUIExchangeInterface* uiex, const char* pszEvent);
	//@}

// Implementation
private:	

	SerXInterface                       *GetSerX() {return m_pSerX; }
	TheSkyXFacadeForDriversInterface	*GetTheSkyXFacadeForDrivers() {return m_pTheSkyXForMounts;}
	SleeperInterface					*GetSleeper() {return m_pSleeper; }
	BasicIniUtilInterface				*GetSimpleIniUtil() {return m_pIniUtil; }
	LoggerInterface						*GetLogger() {return m_pLogger; }
	MutexInterface						*GetMutex()  {return m_pIOMutex;}
	TickCountInterface					*GetTickCountInterface() {return m_pTickCount;}

    void                                portNameOnToCharPtr(char* pszPort, const int& nMaxSize) const;
    void                                updateFilterControls(X2GUIExchangeInterface* dx);
    void                                enableFilterControls(X2GUIExchangeInterface* dx, bool enable);
    void                                enableWheelControls(X2GUIExchangeInterface* dx, bool enable);
    void                                updateSlotData(X2GUIExchangeInterface* dx, int slotIndex);
    
	int                                 m_nPrivateMulitInstanceIndex;
	SerXInterface*                      m_pSerX;
	TheSkyXFacadeForDriversInterface* 	m_pTheSkyXForMounts;
	SleeperInterface*					m_pSleeper;
	BasicIniUtilInterface*				m_pIniUtil;
	LoggerInterface*					m_pLogger;
	MutexInterface*						m_pIOMutex;
	TickCountInterface*					m_pTickCount;

    CXagyl                              Xagyl;
    int                                 m_bLinked;
    int                                 mWheelState; // use in the Settings dialog only.
    bool                                mUiEnabled;

};
