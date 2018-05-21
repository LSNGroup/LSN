#ifndef _WINDOWS
	#include <sys/ioctl.h>
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <unistd.h>
	#include <fcntl.h>
	#include <netdb.h>
	#include <arpa/inet.h>
    #include <net/if.h>
	//#include <net/if_dl.h>
    
	#define _T(s)		(s)
	#define Sleep(x)	usleep(x*1000)
#else
	#error ERROR: UPnP not for Windows!
#endif

#include "platform.h"
#include "udp.h"
#include "UPnP.h"

#ifdef ANDROID_NDK
#include <android/log.h>
#endif

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
	std::string tmp = RTrim(str);
	return LTrim(tmp);
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
	char buffer[1024*8];

	int length = request.length();
	if (length >= sizeof(buffer))
	{
#ifdef ANDROID_NDK ////Debug
		__android_log_print(ANDROID_LOG_INFO, "UPnP", "SOAP_action request buffer too small!!!");
#endif
		return false;
	}
	strcpy(buffer, request.c_str());

	dword ip = inet_addr(addr.c_str());

	struct sockaddr_in sockaddr;
	memset(&sockaddr, 0, sizeof(sockaddr));
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_port = htons(port);
	sockaddr.sin_addr.s_addr = ip;

	SOCKET s = socket(AF_INET, SOCK_STREAM, 0);
	int ret = connect(s, (struct sockaddr *)&sockaddr, sizeof(sockaddr));
	
#ifdef UNIX
  // Set non-blocking I/O
  // UNIX does not support SO_RCVTIMEO
  int opts = fcntl(s, F_GETFL);
  if (-1 == fcntl(s, F_SETFL, opts | O_NONBLOCK))
     return false;
#elif _WINDOWS
  DWORD ot = 100; //milliseconds
  if (0 != setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (char *)&ot, sizeof(DWORD)))
     return false;
#else
  // Set receiving time-out value
  timeval tv;
  tv.tv_sec = 0;
  tv.tv_usec = 100000;
  if (0 != setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(timeval)))
     return false;
#endif
	
	int n = send(s, buffer, length, 0);
	Sleep(500);
	int rlen = recv(s, buffer, sizeof(buffer), 0);
	closesocket(s);
	if (rlen == SOCKET_ERROR) return false;
	if (!rlen) return false;
	if (rlen >= sizeof(buffer))
	{
#ifdef ANDROID_NDK ////Debug
		__android_log_print(ANDROID_LOG_INFO, "UPnP", "SOAP_action recv(s) buffer too small!!!");
#endif
	}

	buffer[rlen] = '\0';
	response = std::string(buffer);

#ifdef ANDROID_NDK ////Debug
	__android_log_print(ANDROID_LOG_INFO, "UPnP", "%s", response.c_str());
#endif

	return true;
}

int SSDP_sendRequest(SOCKET s, dword ip, word port, const std::string& request)
{
	char buffer[1024*8];

	int length = request.length();
	if (length >= sizeof(buffer))
	{
#ifdef ANDROID_NDK ////Debug
		__android_log_print(ANDROID_LOG_INFO, "UPnP", "SSDP_sendRequest request buffer too small!!!");
#endif
		return -1;
	}
	strcpy(buffer, request.c_str());

	struct sockaddr_in sockaddr;
	memset(&sockaddr, 0, sizeof(sockaddr));
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_port = htons(port);
	sockaddr.sin_addr.s_addr = ip;

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

std::string getProperty(const std::string& all, const std::string& name)
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

#ifdef UNIX
	  // Set non-blocking I/O
	  // UNIX does not support SO_RCVTIMEO
	  int opts = fcntl(s, F_GETFL);
	  if (-1 == fcntl(s, F_SETFL, opts | O_NONBLOCK))
	     return false;
#elif _WINDOWS
	  DWORD ot = 5; //milliseconds
	  if (0 != setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (char *)&ot, sizeof(DWORD)))
	     return false;
#else
	  // Set receiving time-out value
	  timeval tv;
	  tv.tv_sec = 0;
	  tv.tv_usec = 5000;
	  if (0 != setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(timeval)))
	     return false;
#endif

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

		char buffer[1024*8];
		rlen = recv(s, buffer, sizeof(buffer), 0);
		if (rlen <= 0) continue;
		if (rlen >= sizeof(buffer))
		{
#ifdef ANDROID_NDK ////Debug
			__android_log_print(ANDROID_LOG_INFO, "UPnP", "InternalSearch recv(s) buffer too small!!!");
#endif
		}
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
		port = atoi(s.c_str());
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

#ifdef ANDROID_NDK ////Debug
	__android_log_print(ANDROID_LOG_INFO, "UPnP", "InvokeCommand(%s) OK!", name.c_str());
#endif
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

UPNPNAT_RETURN MyUPnP::GetNATExternalIp(char retExternalIp[16])
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

	std::string tmpStr("");
	if (!getExtIp(tmpStr)) {
		return UNAT_ERROR;
	}
	else {
		strncpy(retExternalIp, tmpStr.c_str(), 16);
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

	std::string tmpStr("");
	if (!getSpecificEntry(mapping->externalPort, ProtoStr, tmpStr)) {
		return UNAT_ERROR;
	}
	else {
		//mapping->description = tmpStr;
		*lpExists = tmpStr.empty() ? FALSE : TRUE;
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
		snprintf(szTemp, sizeof(szTemp), _T("(%s)[%s:%u]"), mapping->description.c_str(), ProtoStr.c_str(), mapping->externalPort);
		
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
int MyUPnP::InitLocalIP()
{
	m_slocalIP = _T("");
	m_uLocalIP = 0;

#define min(a,b)    ((a) < (b) ? (a) : (b))
#define max(a,b)    ((a) > (b) ? (a) : (b))
#define BUFFERSIZE  4096

#ifndef ANDROID_NDK
	int                 len;
#endif
	char                buffer[BUFFERSIZE], *ptr, lastname[IFNAMSIZ], *cptr;
	struct ifconf       ifc;
	struct ifreq        *ifr, ifrcopy;
	struct sockaddr_in  *sin;
	int sockfd;
	int nextAddr = 0;


	sockfd = socket(AF_INET, SOCK_DGRAM, 0);    
	if (sockfd < 0)    
	{    
		//perror("socket failed");    
		return -1;    
	}    


	ifc.ifc_len = BUFFERSIZE;
	ifc.ifc_buf = buffer;    
	if (ioctl(sockfd, SIOCGIFCONF, &ifc) < 0)    
	{    
		//perror("ioctl error");    
		close(sockfd); 
		return -1;    
	}    

	lastname[0] = '\0';    

	for (ptr = buffer; ptr < buffer + ifc.ifc_len; )    
	{    
		ifr = (struct ifreq *)ptr;    
#ifndef ANDROID_NDK
		len = max(sizeof(struct sockaddr), ifr->ifr_addr.sa_len);    
		ptr += sizeof(ifr->ifr_name) + len;  // for next one in buffer     
#else
		ptr = (char *)(ifr + 1);
#endif
		if (ifr->ifr_addr.sa_family != AF_INET)    
		{    
			continue;   // ignore if not desired address family     
		}    

		if ((cptr = (char *)strchr(ifr->ifr_name, ':')) != NULL)    
		{    
			*cptr = '\0';      // replace colon will null     
		}    

		if (strncmp("lo", ifr->ifr_name, 2) == 0)    
		{    
			continue;   /* Skip loopback interface */   
		}    
		if (strncmp("ppp", ifr->ifr_name, 3) == 0)    
		{    
			continue;   /* Skip ppp interface */   
		}    
		if (strstr(ifr->ifr_name, "wlan") == NULL && strstr(ifr->ifr_name, "wifi") == NULL && strstr(ifr->ifr_name, "eth") == NULL)    
		{    
			continue;   /* Skip xxx interface */   
		}

		if (strncmp(lastname, ifr->ifr_name, IFNAMSIZ) == 0)    
		{    
			continue;   /* already processed this interface */   
		}    

		memcpy(lastname, ifr->ifr_name, IFNAMSIZ);    

		ifrcopy = *ifr;    
		ioctl(sockfd, SIOCGIFFLAGS, &ifrcopy);    
		if ((ifrcopy.ifr_flags & IFF_UP) == 0)    
		{    
			continue;   // ignore if interface not up     
		}    

		sin = (struct sockaddr_in *)&ifr->ifr_addr;    
		if (sin->sin_addr.s_addr != inet_addr("127.0.0.1")) {
			m_uLocalIP = sin->sin_addr.s_addr;
			m_slocalIP = inet_ntoa(sin->sin_addr);
			nextAddr++;
		}
	}    
	
	close(sockfd);
	return 0;

#undef min
#undef max
#undef BUFFERSIZE
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