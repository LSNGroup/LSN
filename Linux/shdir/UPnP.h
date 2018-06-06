#ifndef __KICKER_MYUPNP_H__
#define __KICKER_MYUPNP_H__

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <string>
using namespace std;

#include "platform.h"


/*
mapping.externalPort = ((myUPnP.GetLocalIP() & 0xff000000) >> 24) | ((2049 + (65535 - 2049) * rand() / (RAND_MAX + 1)) & 0xffffff00);
*/

typedef enum{
	UNAT_OK,						// Successfull
	UNAT_ERROR,						// Error, use GetLastError() to get an error description
	UNAT_EXTERNAL_PORT_IN_USE,		// Error, you are trying to add a port mapping with an external port in use
	UNAT_NOT_IN_LAN					// Error, you aren't in a LAN -> no router or firewall
} UPNPNAT_RETURN;

typedef enum{
	UNAT_TCP,						// TCP Protocol
	UNAT_UDP						// UDP Protocol
} UPNPNAT_PROTOCOL;

typedef struct{
	WORD internalPort;				// Port mapping internal port
	WORD externalPort;				// Port mapping external port
	int  protocol;		// Protocol-> TCP (UPNPNAT_PROTOCOL:UNAT_TCP) || UDP (UPNPNAT_PROTOCOL:UNAT_UDP)
	std::string description;			// Port mapping description
} UPNPNAT_MAPPING;


class MyUPnP
{
public:
	MyUPnP();
	~MyUPnP();


	bool		Search(int version=1);

	UPNPNAT_RETURN GetNATExternalIp(char retExternalIp[16]);


	UPNPNAT_RETURN GetNATSpecificEntry(UPNPNAT_MAPPING *mapping, BOOL *lpExists);


	/////////////////////////////////////////////////////////////////////////////////
	// Adds a NAT Port Mapping
	// Params:
	//		UPNPNAT_MAPPING *mapping  ->  Port Mapping Data
	//			If mapping->externalPort is 0, then
	//			mapping->externalPort gets the value of mapping->internalPort
	//		bool tryRandom:
	//			If mapping->externalPort is in use, tries to find a free
	//			random external port.
	//
	// Return:
	//		UNAT_OK:
	//			Successfull.
	//		UNAT_EXTERNAL_PORT_IN_USE:
	//			Error, you are trying to add a port mapping with an external port
	//			in use.
	//		UNAT_NOT_IN_LAN:
	//			Error, you aren't in a LAN -> no router or firewall
	//		UNAT_ERROR:
	//			Error, use GetLastError() to get an error description.
	/////////////////////////////////////////////////////////////////////////////////
	UPNPNAT_RETURN AddNATPortMapping(UPNPNAT_MAPPING *mapping, bool tryRandom = false);

	/////////////////////////////////////////////////////////////////////////////////
	// Removes a NAT Port Mapping
	// Params:
	//		UPNPNAT_MAPPING mapping  ->  Port Mapping Data
	//			Should be the same struct passed to AddNATPortMapping
	//
	// Return:
	//		UNAT_OK:
	//			Successfull.
	//		UNAT_NOT_IN_LAN:
	//			Error, you aren't in a LAN -> no router or firewall
	//		UNAT_ERROR:
	//			Error, use GetLastError() to get an error description.
	/////////////////////////////////////////////////////////////////////////////////
	UPNPNAT_RETURN RemoveNATPortMapping(UPNPNAT_MAPPING mapping);


	std::string	GetLastError();
	std::string	GetLocalIPStr();
	u_long		GetLocalIP();
	bool		IsLANIP(u_long nIP);

protected:
	int			InitLocalIP();
	void		SetLastError(std::string error);

	bool getExtIp(std::string& extIp);

	bool getSpecificEntry(int eport, const std::string& type, std::string& descri);

	bool addPortmap(int eport, int iport, const std::string& iclient,
					   const std::string& descri, const std::string& type);
	bool deletePortmap(int eport, const std::string& type);

	bool		isComplete() const { return !m_controlurl.empty(); }

	bool		GetDescription();
	std::string	GetProperty(const std::string& name, std::string& response);
	bool		InvokeCommand(const std::string& name, const std::string& args);

	bool		Valid()const{return (!m_name.empty() && !m_description.empty());}
	bool		InternalSearch(int version);
	std::string m_externalip;
	std::string m_mappingdesc;

	std::string	m_devicename;
	std::string	m_name;
	std::string	m_description;
	std::string	m_baseurl;
	std::string	m_controlurl;
	std::string	m_friendlyname;
	std::string	m_modelname;
	int			m_version;

private:
	std::string	m_slocalIP;
	std::string	m_slastError;
	u_long		m_uLocalIP;
};
#endif
