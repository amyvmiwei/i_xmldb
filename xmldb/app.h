#pragma once

#include "def.h"

class XMLDBApp
{
public:
	XMLDBApp() {}
	static XMLDBApp& Instance() {static XMLDBApp _app; return _app;}

	void setParam(Param & p) {m_param = p;}
	const Param& getParam() const {return m_param;}

	void execute();
	
	
	void _addInVCProj(DataCenter& AData);
private:
	Param m_param;

	int m_currentVersion;
};