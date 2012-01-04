/*
Released under terms of GNU LGPL 2.1
Author : Marcin Ka³at http://mkalat.pl
Notice : Some very small portions of code for operating on keyboard leds via dos device were taken from http://www.codeguru.com/cpp/w-p/system/keyboard/article.php/c2825/ . 
Thanks to Mark Mc Ginty. Pieces of code taken from Mark McGinty might be released under other terms than this library, please be advised!


*/


#ifndef SCROLL_LOCK_FLASHER_PLUGIN_H
#define SCROLL_LOCK_FLASHER_PLUGIN_H

#include <plugin/TlenPlugin.h>



class ScrollLockFlasherPlugin : public TlenPlugin
{
public:
	ScrollLockFlasherPlugin();

	bool load();
	void unload();

	QString name() const;
	QString friendlyName() const;
	QString icon(int size=0) const;
	QString author() const;
	QString description() const;
	int version() const;
	QString web() const;
	QString email() const;
	QString getLicenseName() const;
	
	
	
	
	TLEN_DECLARE_SLOT(protocolLoaded);
	TLEN_DECLARE_SLOT(messageReceived);
	TLEN_DECLARE_SLOT(conference_message_received);
	TLEN_DECLARE_SLOT(got_new_mail);
	TLEN_DECLARE_SLOT(T7exit);
	//TLEN_DECLARE_SLOT(chat_window_created);

	

	

};

#endif


