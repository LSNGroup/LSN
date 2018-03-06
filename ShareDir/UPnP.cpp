#include "stdafx.h"

#include "UPnP.h"


#define UPNPPORTMAP0   _T("WANIPConnection")
#define UPNPPORTMAP1   _T("WANPPPConnection")
#define UPNPGETEXTERNALIP _T("GetExternalIPAddress")
#define UPNPGETSPECIFICENTRY _T("GetSpecificPortMappingEntry")
#define UPNPADDPORTMAP _T("AddPortMapping")
#define UPNPDELPORTMAP _T("DeletePortMapping")

static const u_long	UPNPADDR = 0xFAFFFFEF;
static const int	UPNPPORT = 1900;
static const char * URNPREFIX = _T("urn:schemas-upnp-org:");


void stringToUpper(std::string &str) {
	for(size_t i = 0; i < str.size(); ++i)
	{
		str[i] = toupper(str[i]);
	}
}

void stringToLower(std::string &str) {
	for(size_t i = 0; i < str.size(); ++i)
	{
		str[i] = tolower(str[i]);
	}
}

std::string LTrim(std::string& str) 
{ 
	int pos = str.find_first_not_of(" \n\r\t");
	if (pos != std::string::npos)
	{
		return str.substr(pos);
	}
	else {
		return std::string("");
	}
} 

std::string RTrim(std::string& str) 
{ 
	int pos = str.find_last_not_of(" \n\r\t");
	if (pos != std::string::npos)
	{
		return str.substr(0, pos + 1); 
	}
	else {
		return std::string("");
	}
} 

std::string trim(std::string& str) 
{ 
	return LTrim(RTrim(str)); 
}

const std::string getString(int i)
{
	char tmp[32];
	snprintf(tmp, sizeof(tmp), "%ld", i);

	return std::string(tmp);
}

const std::string GetArgString(const std::string &name, const std::string &value)
{
	return std::string("<") + name + _T(">") + value + _T("</") + name + _T(">");
}

const std::string GetArgString(const std::string &name, int value)
{
	return std::string("<") + name + _T(">") + getString(value) + _T("</") + name + _T(">");
}

bool SOAP_action(std::string addr, word port, const std::string request, std::string &response)
{
	char buffer[10240];

	int length = request.length();
	strcpy(buffer, request.c_str());

	dword ip = inet_addr(addr.c_str());

	struct sockaddr_in sockaddr;
	memset(&sockaddr, 0, sizeof(sockaddr));
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_port = htons(port);
	sockaddr.sin_addr.S_un.S_addr = ip;

	SOCKET s = socket(AF_INET, SOCK_STREAM, 0);
	int ret = connect(s, (struct sockaddr *)&sockaddr, sizeof(sockaddr));
	u_long lv = 1;
	ioctlsocket(s, FIONBIO, &lv);
	int n = send(s, buffer, length, 0);
	Sleep(500);
	int rlen = recv(s, buffer, sizeof(buffer), 0);
	closesocket(s);
	if (rlen == SOCKET_ERROR) return false;
	if (!rlen) return false;

	buffer[rlen] = '\0';
	response = std::string(buffer);

	return true;
}

int SSDP_sendRequest(SOCKET s, dword ip, word port, const std::string& request)
{
	char buffer[10240];

	int length = request.length();
	strcpy(buffer, request.c_str());

	struct sockaddr_in sockaddr;
	memset(&sockaddr, 0, sizeof(sockaddr));
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_port = htons(port);
	sockaddr.sin_addr.S_un.S_addr = ip;

	return sendto(s, buffer, length, 0, (struct sockaddr *)&sockaddr, sizeof(sockaddr));
}

bool parseHTTPResponse(const std::string& response, std::string& result)
{
	int pos = response.find("\r\n");
	if (pos == std::string::npos) {
		return false;
	}

	std::string status = response.substr(0, pos);
	result = response;
	result.erase(0, pos + 2);

	/*  HTTP/1.1 200 OK  */
	pos = status.find(" ");
	if (pos == std::string::npos) {
		return false;
	}
	status.erase(0, pos + 1);
	pos = status.find(" ");
	if (pos == std::string::npos) {
		return false;
	}
	status = status.substr(0, pos);
	if (status.empty() || status.substr(0, 1) != "2") {
		return false;
	}

	return true;
}

const std::string getProperty(const std::string& all, const std::string& name)
{
	std::string startTag = std::string("<")  + name + ">";
	std::string endTag   = std::string("</") + name + ">";

	int posStart = all.find(startTag, 0);
	if (posStart == std::string::npos) return std::string("");

	int posEnd = all.find(endTag, posStart);
	if (posEnd == std::string::npos || posStart >= posEnd) return std::string("");

	return all.substr(posStart + startTag.length(), posEnd - posStart - startTag.length());
}

MyUPnP::MyUPnP()
: m_version(1)
{
	m_externalip = "";
	m_mappingdesc = "";
	m_uLocalIP = 0;
}

MyUPnP::~MyUPnP()
{

}


bool MyUPnP::InternalSearch(int version)
{
	if(version<=0)version = 1;
	m_version = version;

#define NUMBEROFDEVICES	2
	std::string devices[][2] = {
		{UPNPPORTMAP1, _T("service")},
		{UPNPPORTMAP0, _T("service")},
		{_T("InternetGatewayDevice"), _T("device")},
	};

	SOCKET s = socket(AF_INET, SOCK_DGRAM, 0);
	u_long lv = 1;
	ioctlsocket(s, FIONBIO, &lv);

	int rlen = 0;
	for (int i=0; rlen<=0 && i<100; i++) {
		if (!(i%20)) {
			for (int j=0; j<NUMBEROFDEVICES; j++) {
				char szTemp[1024];
				snprintf(szTemp, sizeof(szTemp), _T("%s%s:%s:%d"), URNPREFIX, devices[j][1].c_str(), devices[j][0].c_str(), version);
				m_name = std::string(szTemp);

				snprintf(szTemp, sizeof(szTemp), _T("M-SEARCH * HTTP/1.1\r\nHOST: 239.255.255.250:1900\r\nMAN: \"ssdp:discover\"\r\nMX: %d\r\nST: %s\r\n\r\n"), 6, m_name.c_str());
				std::string request = std::string(szTemp);

				SSDP_sendRequest(s, UPNPADDR, UPNPPORT, request);
			}
		}

		Sleep(10);

		char buffer[10240];
		rlen = recv(s, buffer, sizeof(buffer), 0);
		if (rlen <= 0) continue;
		closesocket(s);

		buffer[rlen] = '\0';
		std::string response = std::string(buffer);
		std::string result0;
		if (!parseHTTPResponse(response, result0)) return false;

		for (int d=0; d<NUMBEROFDEVICES; d++) {
			char szTemp[1024];
			snprintf(szTemp, sizeof(szTemp), _T("%s%s:%s:%d"), URNPREFIX, devices[d][1].c_str(), devices[d][0].c_str(), version);
			m_name = std::string(szTemp);
			if (result0.find(m_name) != std::string::npos) {
				std::string result = result0;
				for (int pos = 0;;) {
					pos = result.find(_T("\r\n"));
					if (pos == std::string::npos) break;
					std::string line = result.substr(0, pos);
					line = trim(line);
					if (line.empty()) break;
					std::string name = line.substr(0, 9);
					stringToUpper(name);
					if (name == _T("LOCATION:")) {
						line.erase(0, 9);
						m_description = line;
						m_description = trim(m_description);
						return GetDescription();
					}
					result.erase(0, pos + 2);
				}
			}
		}
	}
	closesocket(s);

	return false;
}

bool MyUPnP::Search(int version)
{
	return InternalSearch(version);
}

static std::string NGetAddressFromUrl(const std::string& str, std::string& post, std::string& host, int& port)
{
	std::string s = str;

	post = _T("");
	host = post;
	port = 0;
	int pos = s.find(_T("://"));
	if (std::string::npos == pos) return std::string("");
	s.erase(0, pos + 3);

	pos = s.find('/');
	if (std::string::npos == pos) {
		host = s;
		s = _T("");
	} else {
		host = s.substr(0, pos);
		s.erase(0, pos);
	}

	if (s.empty()) {
		post = _T("");
	} else {
		post = s;
	}

	std::string addr = host.substr(0, host.find(_T(":")));
	if ((pos = host.find(_T(":"))) != std::string::npos)
	{
		s = host.substr(pos + 1);
		port = _tstoi(s.c_str());
	} else {
		port = 80;
	}

	return addr;
}

bool MyUPnP::GetDescription()
{
	if(!Valid())return false;
	std::string post, host, addr;
	int port = 0;
	addr = NGetAddressFromUrl(m_description, post, host, port);
	if(addr.empty())return false;
	std::string request = std::string(_T("GET ")) + post + _T(" HTTP/1.1\r\nHOST: ") + host + _T("\r\nACCEPT-LANGUAGE: en\r\n\r\n");
	std::string response;
	if (!SOAP_action(addr, port, request, response)) return false;
	std::string result;
	if (!parseHTTPResponse(response, result)) return false;

	m_friendlyname = getProperty(result, _T("friendlyName"));
	m_modelname = getProperty(result, _T("modelName"));
	m_baseurl = getProperty(result, _T("URLBase"));
	if(m_baseurl.empty()) m_baseurl = std::string(_T("http://")) + host + _T("/");
	if(m_baseurl[m_baseurl.length() - 1] != '/') m_baseurl += _T("/");
	
	std::string serviceType = std::string(_T("<serviceType>")) + m_name + _T("</serviceType>");
	int pos = result.find(serviceType);
	if (pos != std::string::npos) {
		result.erase(0, pos + serviceType.length());
		pos = result.find(_T("</service>"));
		if (pos != std::string::npos) {
			result = result.substr(0, pos);
			m_controlurl = getProperty(result, _T("controlURL"));
			if (!m_controlurl.empty() && m_controlurl[0] == '/') {
				m_controlurl = m_baseurl + m_controlurl.substr(1);
			}
		}
	}

	return isComplete();
}

std::string MyUPnP::GetProperty(const std::string& name, std::string& response)
{
	if (!isComplete()) return std::string("");
	std::string post, host, addr;
	int port = 0;
	addr = NGetAddressFromUrl(m_controlurl, post, host, port);
	if(addr.empty()) return std::string("");
	std::string cnt("");
	std::string psr("");
	cnt.append(_T("<s:Envelope\r\n    xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\"\r\n    "));
	cnt.append(_T("s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">\r\n  <s:Body>\r\n    <u:"));
	cnt.append(name);
	cnt.append(_T(" xmlns:u=\""));
	cnt.append(m_name);
	cnt.append(_T("\">\r\n    </u:"));
	cnt.append(name);
	cnt.append(_T(">\r\n  </s:Body>\r\n</s:Envelope>\r\n\r\n"));
	psr.append(_T("POST "));
	psr.append(post);
	psr.append(_T(" HTTP/1.1\r\nHOST: "));
	psr.append(host);
	psr.append(_T("\r\nContent-Length: "));
	psr.append(getString(cnt.length()));
	psr.append(_T("\r\nContent-Type: text/xml; charset=\"utf-8\"\r\nSOAPAction: \""));
	psr.append(m_name);
	psr.append(_T("#"));
	psr.append(name);
	psr.append(_T("\"\r\n\r\n"));
	psr.append(cnt);

	std::string request = psr;
	if (!SOAP_action(addr, port, request, response)) return std::string("");
	std::string result;
	if (!parseHTTPResponse(response, result)) return std::string("");

	return getProperty(result, response);
}

bool MyUPnP::InvokeCommand(const std::string& name, const std::string& args)
{
	if(!isComplete()) return false;
	std::string post, host, addr;
	int port = 0;
	addr = NGetAddressFromUrl(m_controlurl, post, host, port);
	if(addr.empty()) return false;
	std::string cnt("");
	std::string psr("");
	cnt.append(_T("<?xml version=\"1.0\"?><s:Envelope\r\n    xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\"\r\n    "));
	cnt.append(_T("s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">\r\n  <s:Body>\r\n    <u:"));
	cnt.append(name);
	cnt.append(_T(" xmlns:u=\""));
	cnt.append(m_name);
	cnt.append(_T("\">\r\n"));
	cnt.append(args);
	cnt.append(_T("    </u:"));
	cnt.append(name);
	cnt.append(_T(">\r\n  </s:Body>\r\n</s:Envelope>\r\n\r\n"));
	psr.append(_T("POST "));
	psr.append(post);
	psr.append(_T(" HTTP/1.1\r\nHOST: "));
	psr.append(host);
	psr.append(_T("\r\nContent-Length: "));
	psr.append(getString(cnt.length()));
	psr.append(_T("\r\nContent-Type: text/xml; charset=\"utf-8\"\r\nSOAPAction: \""));
	psr.append(m_name);
	psr.append(_T("#"));
	psr.append(name);
	psr.append(_T("\"\r\n\r\n"));
	psr.append(cnt);

	std::string response;
	std::string request = psr;
	if (!SOAP_action(addr, port, request, response)) {m_controlurl = ""; return false;}
	std::string result;
	if (!parseHTTPResponse(response, result)) return false;

	if (name == UPNPGETEXTERNALIP)
	{
		m_externalip = getProperty(result, _T("NewExternalIPAddress"));
	}
	else if (name == UPNPGETSPECIFICENTRY)
	{
		m_mappingdesc = getProperty(result, _T("NewPortMappingDescription"));
	}
	return true;
}

bool MyUPnP::getExtIp(std::string& extIp)
{
	m_externalip = "";
	if(!InvokeCommand(UPNPGETEXTERNALIP, std::string("")) || m_externalip.empty())
	{
		return false;
	}
	extIp = m_externalip;
	return true;
}

bool MyUPnP::getSpecificEntry(int eport, const std::string& type, std::string& descri)
{
	std::string args("");

	args.append(GetArgString(_T("NewRemoteHost"), _T("")));
	args.append(GetArgString(_T("NewExternalPort"), eport));
	args.append(GetArgString(_T("NewProtocol"), type));

	m_mappingdesc = "";
	if(!InvokeCommand(UPNPGETSPECIFICENTRY, args) || m_mappingdesc.empty())
	{
		return false;
	}
	descri = m_mappingdesc;
	return true;
}

bool MyUPnP::addPortmap(int eport, int iport, const std::string& iclient, const std::string& descri, const std::string& type)
{
	std::string args("");

	args.append(GetArgString(_T("NewRemoteHost"), _T("")));
	args.append(GetArgString(_T("NewExternalPort"), eport));
	args.append(GetArgString(_T("NewProtocol"), type));
	args.append(GetArgString(_T("NewInternalPort"), iport));
	args.append(GetArgString(_T("NewInternalClient"), iclient));
	args.append(GetArgString(_T("NewEnabled"), _T("1")));
	args.append(GetArgString(_T("NewPortMappingDescription"), descri));
	args.append(GetArgString(_T("NewLeaseDuration"), 0));//²»ÓÀ¾Ã

	return InvokeCommand(UPNPADDPORTMAP, args);
}

bool MyUPnP::deletePortmap(int eport, const std::string& type)
{
	std::string args("");

	args.append(GetArgString(_T("NewRemoteHost"), _T("")));
	args.append(GetArgString(_T("NewExternalPort"), eport));
	args.append(GetArgString(_T("NewProtocol"), type));

	return InvokeCommand(UPNPDELPORTMAP, args);
}

UPNPNAT_RETURN MyUPnP::GetNATExternalIp(std::string& retExternalIp)
{
	if(!IsLANIP(GetLocalIP())){
		SetLastError(_T("You aren't behind a Hardware Firewall or Router"));
		return UNAT_NOT_IN_LAN;
	}

	if (!isComplete()) {
		Search();
		if (!isComplete()) {
			SetLastError(_T("Can not found a UPnP Router"));
			return UNAT_ERROR;
		}
	}

	if (!getExtIp(retExternalIp)) {
		return UNAT_ERROR;
	}
	else {
		return UNAT_OK;
	}
}

UPNPNAT_RETURN MyUPnP::GetNATSpecificEntry(UPNPNAT_MAPPING *mapping, BOOL *lpExists)
{
	std::string	ProtoStr;

	if(!IsLANIP(GetLocalIP())){
		SetLastError(_T("You aren't behind a Hardware Firewall or Router"));
		return UNAT_NOT_IN_LAN;
	}

	if (!isComplete()) {
		Search();
		if (!isComplete()) {
			SetLastError(_T("Can not found a UPnP Router"));
			return UNAT_ERROR;
		}
	}

	if (mapping->protocol == UNAT_TCP){
		ProtoStr = _T("TCP");
	}
	else {
		ProtoStr = _T("UDP");
	}

	mapping->description = "";
	if (!getSpecificEntry(mapping->externalPort, ProtoStr, mapping->description)) {
		return UNAT_ERROR;
	}
	else {
		*lpExists = mapping->description.empty() ? FALSE : TRUE;
		return UNAT_OK;
	}
}

UPNPNAT_RETURN MyUPnP::AddNATPortMapping(UPNPNAT_MAPPING *mapping, bool tryRandom)
{
	std::string	ProtoStr, Proto;

	if(!IsLANIP(GetLocalIP())){
		SetLastError(_T("You aren't behind a Hardware Firewall or Router"));
		return UNAT_NOT_IN_LAN;
	}
	
	if (!isComplete()) {
		Search();
		if (!isComplete()) {
			SetLastError(_T("Can not found a UPnP Router"));
			return UNAT_ERROR;
		}
	}

	if (mapping->protocol == UNAT_TCP){
		Proto = _T("TCP");
		ProtoStr = _T("TCP");
	}
	else {
		Proto = _T("UDP");
		ProtoStr = _T("UDP");
	}

	if(mapping->externalPort == 0)
		mapping->externalPort = mapping->internalPort;

	WORD rndPort = mapping->externalPort;
	for (int retries = 255; retries > 0; retries--) {

		char szTemp[1024];
		_snprintf(szTemp, sizeof(szTemp), _T("(%s)[%s:%u]"), mapping->description.c_str(), ProtoStr.c_str(), mapping->externalPort);
		
		std::string Desc = std::string(szTemp);

		if (addPortmap(mapping->externalPort, mapping->internalPort, GetLocalIPStr(), Desc, Proto)) {
			return UNAT_OK;
		}

		if (!tryRandom) {
			SetLastError(_T("External NAT port in use"));
			return UNAT_EXTERNAL_PORT_IN_USE;
		}

		mapping->externalPort = 2049 + (65535 - 2049) * rand() / (RAND_MAX + 1);
	}

	SetLastError(_T("External NAT port in use: Too many retries"));
	return UNAT_EXTERNAL_PORT_IN_USE;
}

UPNPNAT_RETURN MyUPnP::RemoveNATPortMapping(UPNPNAT_MAPPING mapping)
{
	if(!IsLANIP(GetLocalIP())){
		SetLastError(_T("You aren't behind a Hardware Firewall or Router"));
		return UNAT_NOT_IN_LAN;
	}

	if (!isComplete()) {
		Search();
		if (!isComplete()) {
			SetLastError(_T("Can not found a UPnP Router"));
			return UNAT_ERROR;
		}
	}


	std::string Proto;

	if (mapping.protocol == UNAT_TCP)
		Proto = _T("TCP");
	else
		Proto = _T("UDP");

	if (deletePortmap(mapping.externalPort, Proto)) {
		return UNAT_OK;
	} else {
		SetLastError(_T("Error getting StaticPortMappingCollection"));
		return UNAT_ERROR;
	}
}

/////////////////////////////////////////////////////////////////////////////////
// Initializes m_localIP variable, for future access to GetLocalIP()
/////////////////////////////////////////////////////////////////////////////////
void MyUPnP::InitLocalIP()
{
#ifndef _DEBUG
	try
#endif
	{
		char szHost[256];
		if (gethostname(szHost, sizeof szHost) == 0){
			hostent* pHostEnt = gethostbyname(szHost);
			if (pHostEnt != NULL && pHostEnt->h_length == 4 && pHostEnt->h_addr_list[0] != NULL){
				UPNPNAT_MAPPING mapping;
				struct in_addr addr;

				memcpy(&addr, pHostEnt->h_addr_list[0], sizeof(struct in_addr));
				m_slocalIP = inet_ntoa(addr);
				m_uLocalIP = addr.S_un.S_addr;
			}
			else{
				m_slocalIP = _T("");
				m_uLocalIP = 0;
			}
		}
		else{
			m_slocalIP = _T("");
			m_uLocalIP = 0;
		}
	}
#ifndef _DEBUG
	catch(...){
		m_slocalIP = _T("");
		m_uLocalIP = 0;
	}
#endif
}

/////////////////////////////////////////////////////////////////////////////////
// Returns the Local IP
/////////////////////////////////////////////////////////////////////////////////
u_long MyUPnP::GetLocalIP()
{
	InitLocalIP();
	return m_uLocalIP;
}

/////////////////////////////////////////////////////////////////////////////////
// Returns a std::string with the local IP in format xxx.xxx.xxx.xxx
/////////////////////////////////////////////////////////////////////////////////
std::string MyUPnP::GetLocalIPStr()
{
	if(m_slocalIP.empty())
		InitLocalIP();
	
	return m_slocalIP;
}

/////////////////////////////////////////////////////////////////////////////////
// Sets the value of m_lastError (last error description)
/////////////////////////////////////////////////////////////////////////////////
void MyUPnP::SetLastError(std::string error)	{
	m_slastError = error;
};

/////////////////////////////////////////////////////////////////////////////////
// Returns the last error description in a std::string
/////////////////////////////////////////////////////////////////////////////////
std::string MyUPnP::GetLastError()
{ 
	return m_slastError; 
}

/////////////////////////////////////////////////////////////////////////////////
// Returns true if nIP is a LAN ip, false otherwise
/////////////////////////////////////////////////////////////////////////////////
bool MyUPnP::IsLANIP(u_long nIP){
	if (0 == nIP) {
		return false;
	}
// filter LAN IP's
// -------------------------------------------
// 0.*
// 10.0.0.0 - 10.255.255.255  class A
// 172.16.0.0 - 172.31.255.255  class B
// 192.168.0.0 - 192.168.255.255 class C

	unsigned char nFirst = (unsigned char)(nIP & 0x000000ff);
	unsigned char nSecond = (unsigned char)((nIP & 0x0000ff00) >> 8);

	if (nFirst==192 && nSecond==168) // check this 1st, because those LANs IPs are mostly spreaded
		return true;

	if (nFirst==172 && nSecond>=16 && nSecond<=31)
		return true;

	if (nFirst==0 || nFirst==10)
		return true;

	return false; 
}
