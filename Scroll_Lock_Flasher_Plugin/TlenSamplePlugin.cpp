/*
Released under terms of GNU LGPL 2.1
Author : Marcin Ka³at http://mkalat.pl
Notice : Some very small portions of code for operating on keyboard leds via dos device were taken from http://www.codeguru.com/cpp/w-p/system/keyboard/article.php/c2825/ . 
Thanks to Mark Mc Ginty. Pieces of code taken from Mark McGinty might be released under other terms than this library, please be advised!

*/


#include "TlenSamplePlugin.h"

#include <QTextCodec>
#include <QFile>
#include <windows.h>
#include <process.h>
#include <tchar.h>
#include <core/util.h>
#include <winioctl.h>
#include <gui/TlenContextMenu.h>
#include <plugin/TlenAccountConnection.h>
#include <plugin/TlenAccountManager.h>
#include <plugin/TlenPluginManager.h>
#include <plugin/TlenProtocol.h>
#include <roster/TlenRoster.h>
#include <debug/TlenDebug.h>
#include <data/TlenBuddy.h>
#include <gui/TlenChatManager.h>
#include <gui/TlenChatWindow.h>
#include <settings/TlenSettingsManager.h>

void Flash_Action(void *MyID);
void Flash_Action_NS(void* bud_id);
bool CheckWNDS(void);
HANDLE OpenKeyboardDevice(int* ErrorNumber);
int CloseKeyboardDevice(HANDLE hndKbdDev);

int ThreadNr = 0;
int MAX_THREADS = 1;
DWORD sleep_time = 500;
int flash_count = 10;
bool n_stop = false;
QString bud_id_g;
bool fl_STOP = false;
int counter = 0;
QFile errlog;

#define IOCTL_KEYBOARD_SET_INDICATORS        CTL_CODE(FILE_DEVICE_KEYBOARD, 0x0002, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_KEYBOARD_QUERY_TYPEMATIC       CTL_CODE(FILE_DEVICE_KEYBOARD, 0x0008, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_KEYBOARD_QUERY_INDICATORS      CTL_CODE(FILE_DEVICE_KEYBOARD, 0x0010, METHOD_BUFFERED, FILE_ANY_ACCESS)


typedef struct _KEYBOARD_INDICATOR_PARAMETERS {
    USHORT UnitId;          // Unit identifier.
    USHORT    LedFlags;     // LED indicator state.

} KEYBOARD_INDICATOR_PARAMETERS, *PKEYBOARD_INDICATOR_PARAMETERS;

#define KEYBOARD_CAPS_LOCK_ON     4
#define KEYBOARD_NUM_LOCK_ON      2
#define KEYBOARD_SCROLL_LOCK_ON   1

UINT LedFlag;



ScrollLockFlasherPlugin::ScrollLockFlasherPlugin()
{
	QTextCodec *codec = QTextCodec::codecForName("Windows-1250");
	QStringList null_list;
	null_list.append("");
	null_list.append(""); 
	QStringList id;
	QStringList etyk;
	id.append("0");
	id.append("1");
	id.append("2");
	etyk.append("Scroll Lock");
	etyk.append("Caps Lock");
	etyk.append("Num Lock");
	addPluginSettingsField(TlenField::spinBox(codec->toUnicode("Iloœæ migniêæ"),"fl_count",999,1,10,"",""));
	addPluginSettingsField(TlenField::spinBox(codec->toUnicode("Odstêp czasu (ms)"),"tm_count",999,100,500,"","")); 
	addPluginSettingsField(TlenField::checkBox(codec->toUnicode("Migaj bez przerwy dopóki nie otworzê okna rozmowy"),"n_stop",false,"",null_list));
	addPluginSettingsField(TlenField::comboBox(codec->toUnicode("Wybierz lampkê"),"light",id,etyk,"0",""));
	addPluginSettingsField(TlenField::checkBox(codec->toUnicode("Sumuj migniêcia kiedy przychodz¹ nowe wiad."),"fl_sum",false,"",null_list));

}

bool ScrollLockFlasherPlugin::load()
{	
	errlog.setFileName("slfp_log.txt"); 
	errlog.open(QFile::WriteOnly | QFile::Text);
	errlog.write("SCROLL LOCK FLASHER PLUGIN 0.0.14 - LOGGING STARTED \n");

	TlenPluginManager *pm = TlenPluginManager::getInstance();

	errlog.write("TlenPluginManager instance \n");

	foreach (TlenProtocol *proto, pm->getRegisteredProtocols()) {		
		slotConnect(proto->getPluginId(), MESSAGE_RECEIVED, TLEN_SLOT(ScrollLockFlasherPlugin::messageReceived));
		slotConnect(proto->getPluginId(), CONF_MESSAGE_RECEIVED, TLEN_SLOT(ScrollLockFlasherPlugin::conference_message_received));
		slotConnect(proto->getPluginId(), NEW_MAIL, TLEN_SLOT(ScrollLockFlasherPlugin::got_new_mail));
	}

	errlog.write("Protocol Signals Connected \n");
	bool result = slotConnect(TLEN_PLUGIN_CORE, LOADED_PROTOCOL, TLEN_SLOT(ScrollLockFlasherPlugin::protocolLoaded));
	//result = result && slotConnect(TLEN_PLUGIN_GUI, CHAT_WINDOW_CREATED, TLEN_SLOT(ScrollLockFlasherPlugin::chat_window_created));
	slotConnect(TLEN_PLUGIN_CORE,APP_QUITTING,TLEN_SLOT(ScrollLockFlasherPlugin::T7exit));

	errlog.write("Tlen Plugin Core Slot Connected \n");



	return result;
}

void ScrollLockFlasherPlugin::unload()
{
errlog.write("PLUGIN UNLOADING - LOGGING STOPPED... \n");
errlog.close();
	
	
}

QString ScrollLockFlasherPlugin::name() const
{
	return QString("ScrollLockFlasherPlugin");
}

QString ScrollLockFlasherPlugin::friendlyName() const
{
	return QObject::tr("Scroll Lock Flasher Plugin");	
}

QString ScrollLockFlasherPlugin::icon(int size) const
{
	return QString("");
}

QString ScrollLockFlasherPlugin::author() const
{
	QTextCodec *codec = QTextCodec::codecForName("Windows-1250");
	return QString(codec->toUnicode("Marcin Ka³at 2010 - 2011"));
}

QString ScrollLockFlasherPlugin::description() const
{
	QTextCodec *codec = QTextCodec::codecForName("Windows-1250");

	
	return QString(codec->toUnicode("Plugin miga lampk¹ Scroll Lock, kiedy przyjdzie wiadomoœæ."));
	
}

int ScrollLockFlasherPlugin::version() const
{
	return TLEN_PLUGIN_VERSION(0, 0, 14);
}

QString ScrollLockFlasherPlugin::web() const
{
	return QString("http://www.mkalat.waw.pl");
}

QString ScrollLockFlasherPlugin::email() const
{
	return QString("support@mkalat.waw.pl");
}
QString ScrollLockFlasherPlugin::getLicenseName() const
{
	return QString("GNU LGPL 2.1");
}






TLEN_DEFINE_SLOT(ScrollLockFlasherPlugin, protocolLoaded)
{	
	errlog.write("Signal - Protocol Loaded beeing handled.... \n");
	QString protocol = args[1].toPlugin()->getPluginId();
	slotConnect(protocol, MESSAGE_RECEIVED, TLEN_SLOT(ScrollLockFlasherPlugin::messageReceived));
	slotConnect(protocol, CONF_MESSAGE_RECEIVED, TLEN_SLOT(ScrollLockFlasherPlugin::conference_message_received));
	slotConnect(protocol, NEW_MAIL, TLEN_SLOT(ScrollLockFlasherPlugin::got_new_mail));
	errlog.write("Signal - Protocol Loaded processed \n");
}

TLEN_DEFINE_SLOT(ScrollLockFlasherPlugin, messageReceived)
{
	errlog.write("Signal - Message Recieved beeing handled... \n");
	TlenAccountConnection *acc = args[0].toAccount();
	errlog.write("Connection \n");
	QString bid = args[1].toString();
	errlog.write("bid \n");
	QString protocol = acc->getProtocolName();	
	errlog.write("proto \n");

	bud_id_g = bid;
	
	TlenField tf, tf_fl_sum;
	tf = getPluginPref("fl_count");
	flash_count = tf.getIntValue();
	tf = getPluginPref("tm_count");
	sleep_time = tf.getIntValue(); 
	tf = getPluginPref("n_stop");
	n_stop = tf.isChecked(); 
	tf = getPluginPref("light");

	errlog.write("Tlen Fields read \n");
	if (tf.getSelectedId() == "0")
	{
		errlog.write("Selected Scroll Lock \n");
		LedFlag = KEYBOARD_SCROLL_LOCK_ON;
	}
	else if (tf.getSelectedId() == "1")
	{
		errlog.write("Selected Caps Lock \n");
		LedFlag = KEYBOARD_CAPS_LOCK_ON;
	}
	else if (tf.getSelectedId() == "2")
	{
		errlog.write("Selected Num Lock \n");
		LedFlag = KEYBOARD_NUM_LOCK_ON;
	}

	tf_fl_sum = getPluginPref("fl_sum");
	
	if (tf_fl_sum.isChecked() == true)
	{
		counter = 0;
		
	}
	
	
	if (n_stop == true)
	{
		errlog.write("Non-stop flashing action selected \n");
		if (ThreadNr < MAX_THREADS)
		{
			ThreadNr++;
			//MessageBox(NULL,(LPCTSTR)"uruchamiam watek",NULL,MB_OK);
			fl_STOP = false;
			_beginthread(Flash_Action_NS, 2048, &ThreadNr);
			errlog.write("thread non_stop flashing launched \n");
		}
	}
	else 
	{
		errlog.write("STD Flashing action selected \n");
		if (ThreadNr < MAX_THREADS)
		{
			ThreadNr++;
			
			_beginthread(Flash_Action ,2048, &ThreadNr);
			errlog.write("STD flashing action thread lauinched \n");
		

		}
	}

	
}

TLEN_DEFINE_SLOT(ScrollLockFlasherPlugin, got_new_mail)
{
	TlenAccountConnection *acc = args[0].toAccount();
	QString bid = args[1].toString();
	QString protocol = acc->getProtocolName();	

	bud_id_g = bid;
	
	TlenField tf, tf_fl_sum;
	tf = getPluginPref("fl_count");
	flash_count = tf.getIntValue();
	tf = getPluginPref("tm_count");
	sleep_time = tf.getIntValue(); 
	tf = getPluginPref("n_stop");
	n_stop = tf.isChecked(); 
	tf = getPluginPref("light");

	if (tf.getSelectedId() == "0")
	{
		LedFlag = KEYBOARD_SCROLL_LOCK_ON;
	}
	else if (tf.getSelectedId() == "1")
	{
		LedFlag = KEYBOARD_CAPS_LOCK_ON;
	}
	else if (tf.getSelectedId() == "2")
	{
		LedFlag = KEYBOARD_NUM_LOCK_ON;
	}

	tf_fl_sum = getPluginPref("fl_sum");
	
	if (tf_fl_sum.isChecked() == true)
	{
		counter = 0;
		
	}
	
	
	if (n_stop == true)
	{
		if (ThreadNr < MAX_THREADS)
		{
			//ThreadNr++;
			////MessageBox(NULL,(LPCTSTR)"uruchamiam watek",NULL,MB_OK);
			//fl_STOP = false;
			//_beginthread(Flash_Action_NS, 2048, &ThreadNr);

			ThreadNr++;
			_beginthread(Flash_Action ,2048, &ThreadNr);
		}
	}
	else 
	{
		if (ThreadNr < MAX_THREADS)
		{
			ThreadNr++;
			
			_beginthread(Flash_Action ,2048, &ThreadNr);
		

		}
	}




}






void Flash_Action(void *MyID)
{

	errlog.write("Inside STD Flashing action \n");
	KEYBOARD_INDICATOR_PARAMETERS InputBuffer;    // Input buffer for DeviceIoControl
    KEYBOARD_INDICATOR_PARAMETERS OutputBuffer;   // Output buffer for DeviceIoControl
    UINT                LedFlagsMask;
    BOOL                Toggle;
    ULONG               DataLength = sizeof(KEYBOARD_INDICATOR_PARAMETERS);
    ULONG               ReturnedLength; // Number of bytes returned in output buffer
    

    InputBuffer.UnitId = 0;
    OutputBuffer.UnitId = 0;
	
	HANDLE hKbdDev = OpenKeyboardDevice(NULL);

	
	
	if (hKbdDev != NULL)
	{
		errlog.write("Got keyboard device handle \n");
		// Preserve current indicators' state
		//
		if (!DeviceIoControl(hKbdDev, IOCTL_KEYBOARD_QUERY_INDICATORS,
                &InputBuffer, DataLength,
                &OutputBuffer, DataLength,
                &ReturnedLength, NULL))
        {
			ThreadNr--;
			_endthread();
		}

	  // Mask bit for light to be manipulated
	   //
	   LedFlagsMask = (OutputBuffer.LedFlags & (~LedFlag));

	   // Set toggle variable to reflect current state.
	   //
	   Toggle = (OutputBuffer.LedFlags & LedFlag);

		errlog.write("Reached toggling loop \n");

		for (counter = 0; counter < flash_count; counter++)
		{

			Toggle ^= 1;
			InputBuffer.LedFlags = (LedFlagsMask | (LedFlag * Toggle));

			if (!DeviceIoControl(hKbdDev, IOCTL_KEYBOARD_SET_INDICATORS,
                    &InputBuffer, DataLength,
                    NULL,   0,  &ReturnedLength, NULL))
			{
				errlog.write("Toggling kbdlight impossible - escaping flashing thread \n");
				ThreadNr--;
				_endthread();
			}
			errlog.write("kbdlight toggled \n");
			Sleep(sleep_time);

			Toggle ^= 1;
			InputBuffer.LedFlags = (LedFlagsMask | (LedFlag * Toggle));

			if (!DeviceIoControl(hKbdDev, IOCTL_KEYBOARD_SET_INDICATORS,
                    &InputBuffer, DataLength,
                    NULL,   0,  &ReturnedLength, NULL))
			{
				errlog.write("Toggling kbdlight impossible - escaping flashing thread \n");
				ThreadNr--;
				_endthread();
			}
			
			errlog.write("kbdlight toggled 2 \n");

			Sleep(sleep_time);
		}

	}
	errlog.write("Programmed toggling steps completed - escaping thread \n");
	ThreadNr--;
	CloseKeyboardDevice(hKbdDev);
	errlog.write("Closed kbddevice \n");
	_endthread();
}

void Flash_Action_NS(void* bud_id)
{
	errlog.write("Inside non-stop flashing thread \n");
	KEYBOARD_INDICATOR_PARAMETERS InputBuffer;    // Input buffer for DeviceIoControl
    KEYBOARD_INDICATOR_PARAMETERS OutputBuffer;   // Output buffer for DeviceIoControl
    UINT                LedFlagsMask;
    BOOL                Toggle;
    ULONG               DataLength = sizeof(KEYBOARD_INDICATOR_PARAMETERS);
    ULONG               ReturnedLength; // Number of bytes returned in output buffer
    int             i;

    InputBuffer.UnitId = 0;
    OutputBuffer.UnitId = 0;

	HANDLE hKbdDev = OpenKeyboardDevice(NULL);

	if (hKbdDev != NULL)
	{
		errlog.write("Got keyboard device handle \n");
	    // Preserve current indicators' state
	   //
	   if (!DeviceIoControl(hKbdDev, IOCTL_KEYBOARD_QUERY_INDICATORS,
                &InputBuffer, DataLength,
                &OutputBuffer, DataLength,
                &ReturnedLength, NULL))
		{	
			ThreadNr--;
			_endthread();
		}
        

		// Mask bit for light to be manipulated
		//
		LedFlagsMask = (OutputBuffer.LedFlags & (~LedFlag));

		// Set toggle variable to reflect current state.
		//
		Toggle = (OutputBuffer.LedFlags & LedFlag);
		errlog.write("Reached checking loop \n");
		while (1)
		{
			errlog.write("Checking if window is activated \n");
			if (CheckWNDS() == true)
			{
				errlog.write("Window activated - stopping flash action \n");
				break;
			}
			errlog.write("Reached toggling loop \n");
			for (i = 0; i < 2; i++)
			{
				Toggle ^= 1;
				InputBuffer.LedFlags = (LedFlagsMask | (LedFlag * Toggle));
				
				

				if (!DeviceIoControl(hKbdDev, IOCTL_KEYBOARD_SET_INDICATORS,
                    &InputBuffer, DataLength,
                    NULL,   0,  &ReturnedLength, NULL))
				{
					errlog.write("kbddevice is not valid - closing thread \n");
					ThreadNr--;
					_endthread();
				}
				errlog.write("Toggling.... \n");

				Sleep(sleep_time);
			}

		}
	}
	errlog.write("Reached non stop flashing thread closing point \n");
	ThreadNr--;
	CloseKeyboardDevice(hKbdDev);
	errlog.write("kbddevice closed - closing thread \n");
	_endthread();


}
TLEN_DEFINE_SLOT(ScrollLockFlasherPlugin, conference_message_received)
{
	TlenAccountConnection *acc = args[0].toAccount();
	QString room_id = args[1].toString();
	QString bid = args[2].toString();
	QString protocol = acc->getProtocolName();	

	TlenField tf;
	tf = getPluginPref("fl_count");
	flash_count = tf.getIntValue();
	tf = getPluginPref("tm_count");
	sleep_time = tf.getIntValue(); 
	tf = getPluginPref("n_stop");
	n_stop = tf.isChecked(); 
	if (n_stop == true)
	{
		if (ThreadNr < MAX_THREADS)
		{
			ThreadNr++;
			fl_STOP = false;
			_beginthread(Flash_Action_NS, 2048, &ThreadNr);
		}

	}
	else 
	{
		if (ThreadNr < MAX_THREADS)
		{
			ThreadNr++;
			
			_beginthread(Flash_Action ,2048, &ThreadNr);
	
		}
	}	
	



}

TLEN_DEFINE_SLOT(ScrollLockFlasherPlugin, T7exit)
{

errlog.write("T7 QUITING - LOGGING STOPPED... \n");
errlog.close();





}

bool CheckWNDS(void)
{
		errlog.write("Checkig active windows func started \n");
		TlenChatManager* cht_man = TlenChatManager::getInstance();
		QList<TlenChatWindow*> cht_wnds;
		TlenBuddy cht_buddy;
		cht_wnds = cht_man->getExistingChatWindows();
		
		errlog.write("Reached windows walking loop \n");
		for each (TlenChatWindow* cht_wnd in cht_wnds)
		{
			if (bud_id_g == buddyId(cht_wnd->getBuddy()))
			{
				if (cht_wnd->hasFocus() == true)//|| cht_wnd->isVisible() == true || )
				{
					fl_STOP = true;
					errlog.write("Window is active - return true \n");
					return true;
					

				}
				else
				{
					fl_STOP = false;
					errlog.write("No window activated - return false \n");
					return false;
					
				}
			}

		}
		fl_STOP = false;
		errlog.write("No windows ? - return false \n");
		return false;



}

HANDLE OpenKeyboardDevice(int* ErrorNumber)
{
	errlog.write("Entered kbddevice function \n");
	HANDLE  hndKbdDev;
    int     *LocalErrorNumber;
    int     Dummy;

    if (ErrorNumber == NULL)
	{
        LocalErrorNumber = &Dummy;
		errlog.write("no errors on check 1 \n");
	}
    else
	{
        LocalErrorNumber = ErrorNumber;
		errlog.write("found error on check 1 \n");
	}

    *LocalErrorNumber = 0;
    
	errlog.write("trying to get kbd handle \n");

    if (!DefineDosDevice (DDD_RAW_TARGET_PATH, "Kbd",
                "\\Device\\KeyboardClass0"))
    {
        *LocalErrorNumber = GetLastError();
		errlog.write("Unable to define local device, USB keyboard ? \n");
        return INVALID_HANDLE_VALUE;
    }

    hndKbdDev = CreateFile("\\\\.\\Kbd", GENERIC_WRITE, 0,
                NULL,   OPEN_EXISTING,  0,  NULL);
    
    if (hndKbdDev == INVALID_HANDLE_VALUE)
	{
		errlog.write("INCVALID_HANDLE_VALUE !!!!! \n");
        *LocalErrorNumber = GetLastError();
	}

    return hndKbdDev;


}

int CloseKeyboardDevice(HANDLE hndKbdDev)
{
    int e = 0;

    if (!DefineDosDevice (DDD_REMOVE_DEFINITION, "Kbd", NULL))
        e = GetLastError();

    if (!CloseHandle(hndKbdDev))                    
        e = GetLastError();
	errlog.write("closed kbd handle - exiting function \n");
    return e;
}



TLEN_INIT_PLUGIN(ScrollLockFlasherPlugin)




