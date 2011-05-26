/* +---------------------------------------------------------------------------+
   |          The Mobile Robot Programming Toolkit (MRPT) C++ library          |
   |                                                                           |
   |                   http://mrpt.sourceforge.net/                            |
   |                                                                           |
   |   Copyright (C) 2005-2011  University of Malaga                           |
   |                                                                           |
   |    This software was written by the Machine Perception and Intelligent    |
   |      Robotics Lab, University of Malaga (Spain).                          |
   |    Contact: Jose-Luis Blanco  <jlblanco@ctima.uma.es>                     |
   |                                                                           |
   |  This file is part of the MRPT project.                                   |
   |                                                                           |
   |     MRPT is free software: you can redistribute it and/or modify          |
   |     it under the terms of the GNU General Public License as published by  |
   |     the Free Software Foundation, either version 3 of the License, or     |
   |     (at your option) any later version.                                   |
   |                                                                           |
   |   MRPT is distributed in the hope that it will be useful,                 |
   |     but WITHOUT ANY WARRANTY; without even the implied warranty of        |
   |     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         |
   |     GNU General Public License for more details.                          |
   |                                                                           |
   |     You should have received a copy of the GNU General Public License     |
   |     along with MRPT.  If not, see <http://www.gnu.org/licenses/>.         |
   |                                                                           |
   +---------------------------------------------------------------------------+ */


#include <mrpt/base.h>  // Precompiled headers


#include <mrpt/utils/CWirelessPower.h>


#include <mrpt/config.h>

#ifdef MRPT_OS_LINUX
#include <iostream>
#include <sstream>
using namespace mrpt::utils;

/*---------------------------------------------------------------
					setNet
   Set the SSID and GUID of the target network
 ---------------------------------------------------------------*/

void CWirelessPower::setNet(std::string ssid_, std::string guid_)
{
	ssid = ssid_;
	guid = guid_;   // in the case of Linux, the "GUID" is the interface name (wlanX)
}


/*---------------------------------------------------------------
					ListInterfaces
   Gets a list of the interfaces
 ---------------------------------------------------------------*/

std::vector<std::string>	CWirelessPower::ListInterfaces()
{
    // in linux, the command line is used to get all the relevant information

	std::vector<std::string> output;				// output vector of strings

	FILE *cmdoutput;                                // file handler for the executed command line
	char ifaceread[256],*netname;                   // strings used to read the output of the command line and get the name of each network

    // Run the command line: get the info frim /proc/net/wireless and cut out the names of the interfaces

	//commandl << "cat /proc/net/wireless|grep \"wlan\"|cut -d\" \" -f2|cut -d\":\" -f1";
	cmdoutput = popen("cat /proc/net/wireless|grep \"wlan\"|cut -d\" \" -f2|cut -d\":\" -f1","r");
	fgets(ifaceread,3,cmdoutput);       // read output

	// iterate thrugh list and get each interface as a string
    netname = strtok(ifaceread,"\n");
	while(netname){
        output.push_back(std::string(netname));
        netname = strtok(NULL,"\n");
	}

	return output;
}


/*---------------------------------------------------------------
					ListNetworks
   Gets a list of the networks available for the interface
 ---------------------------------------------------------------*/

std::vector<std::string>	CWirelessPower::ListNetworks()
{

	std::vector<std::string> output;							// output vector of strings

	std::stringstream commandl;             // command to be executed

	FILE *cmdoutput;
	char listread[1024];
	char *netname;

    // Run command: get a list of networks and cut out their names. Note: this must be done as a superuser, so the command is executed with sudo. Usually it should ask for the password only the first time.
    // To avoid the inconvenience of having to write the password, it would be useful to configure sudo to allow the user to run this command. See "man sudoers"
	commandl << "sudo iwlist " << "wlan0" << " scan|grep ESSID|cut -d\"\\\"\" -f2";
	cmdoutput = popen(commandl.str().c_str(),"r");
	fgets(listread,3,cmdoutput);

	netname = strtok(listread,"\n");
	while(netname){
        output.push_back(std::string(netname));
        netname = strtok(NULL,"\n");
	}


	return output;

}



/*---------------------------------------------------------------
					GetPower
   Gets the power of the network
 ---------------------------------------------------------------*/
int		CWirelessPower::GetPower()
{
    FILE *cmdoutput;
	char *powerReadL;
    std::stringstream commandl;
    // Run command: get the power of the networks and additional info
	commandl << "sudo iwlist " << "wlan0" << " scan";
	cmdoutput = popen(commandl.str().c_str(),"r");

	std::vector<std::string> powerReadV;
    size_t readBytes;

  powerReadL = (char *) malloc (256);
	std::stringstream ssidLine;

	ssidLine << "ESSID:\"" << ssid << "\"";
	int i = 0;
    getline(&powerReadL,&readBytes,cmdoutput);
    while(!strstr(powerReadL, ssidLine.str().c_str())){
        powerReadV.push_back(std::string(powerReadL));
        getline(&powerReadL,&readBytes,cmdoutput);
       // mrpt::system::pause();
	}

    std::vector<std::string>::iterator ssiter= powerReadV.end() - 2;

char powerLine[256];

	// now we have a string per output line of iwlist. We must find the line containing the desired ESSID and read the power from two lines before

strcpy(powerLine,(*ssiter).c_str());

    char level[10];
    //*level = '0';
	// If the ESSID was found and the line containing the power read, separate the information now
	//if (sIter != powerReadV.end()){
	 //   std::cout << "PASA 6" << std::endl;
        // meaning that the ESSID was found
        // Example pf the value of poerLine: Quality=57/100  Signal level=57/100
        char *fraction;
         //fraction = (char *) malloc (256);
        strtok(powerLine,"=");
        strtok(NULL,"=");
        fraction = strtok(NULL,"=");
        strcpy(level,strtok(fraction,"/"));


    //std::cout << "Power level read: " << level << std::endl;
    free(powerReadL);

    return atoi(level);
/*
	// Calculate the RSSI
	if (wlan->wlanSignalQuality == 0)
		iRSSI = -100;
	else if (wlan->wlanSignalQuality == 100)
		iRSSI = -50;
	else
		iRSSI = -100 + (wlan->wlanSignalQuality/2);

	return iRSSI;
*/

}


#endif //ifdef MRPT_OS_LINUX

#ifdef MRPT_OS_WINDOWS
#include <windows.h>
#include <wlanapi.h>
#include <objbase.h>
#include <wtypes.h>
#pragma comment(lib, "Wlanapi.lib")

using namespace mrpt::utils;

/*---------------------------------------------------------------
					ConnectWlanServerW
   Get a connection to the WLAN server
 ---------------------------------------------------------------*/

		/** Gets a connection to the server
		 * \exception std::exception In case there is a failure (happens when WiFi is not started)
		 */

void*	ConnectWlanServerW()
{
	DWORD dwMaxClient = 2;        
    DWORD dwCurVersion = 0;
	DWORD dwResult = 0;			// Result of the API call
	HANDLE hClient;
	// open connection to server 
    dwResult = WlanOpenHandle(dwMaxClient, NULL, &dwCurVersion, &hClient);
    if (dwResult != ERROR_SUCCESS) {
		// if an error ocurred
		std::stringstream excmsg;
		excmsg << "WlanOpenHandle failed with error: " << dwResult << std::endl;
		
        // You can use FormatMessage here to find out why the function failed
		THROW_EXCEPTION(excmsg.str());
    }
	return (void*)hClient;
}


/*---------------------------------------------------------------
					ListInterfacesW
   Gets a list of the interfaces (Windows)
 ---------------------------------------------------------------*/

		/** Gets a list of the interfaces available in the system (in Windows format)
		 * \exception std::exception In case there is a failure
		 * \return std::vector returns handles to the available interfaces
		 */

std::vector<PWLAN_INTERFACE_INFO>	ListInterfacesW(HANDLE hClient)
{
	// Get a list of the available interfaces

	std::vector<PWLAN_INTERFACE_INFO> outputVector;			// start the output vector
	PWLAN_INTERFACE_INFO_LIST pIfList = NULL;				// list of WLAN interfaces
    PWLAN_INTERFACE_INFO pIfInfo = NULL;					// information element for one interface
	DWORD dwResult = 0;

	int i;	

	
	// Call the interface enumeration function of the API
	dwResult = WlanEnumInterfaces(hClient, NULL, &pIfList);
	
	// check result
    if (dwResult != ERROR_SUCCESS) {
		// In case of error, raise an exception
		std::stringstream excmsg;
		excmsg << "WlanEnumInterfaces failed with error: " << dwResult << std::endl;
		
		THROW_EXCEPTION(excmsg.str());
        // You can use FormatMessage here to find out why the function failed
    } else {
		// iterate throught interfaces to add them to the output vector
		for (i = 0; i < (int) pIfList->dwNumberOfItems; i++) {
			
            pIfInfo = (WLAN_INTERFACE_INFO *) &pIfList->InterfaceInfo[i];
			outputVector.push_back(pIfInfo);
		}
	}
	return outputVector;
	
}




/*---------------------------------------------------------------
					setNet
   Set the SSID and GUID of the target network
 ---------------------------------------------------------------*/

void CWirelessPower::setNet(std::string ssid_, std::string guid_)
{
	ssid = ssid_;
	guid = guid_;
	
	hClient = ConnectWlanServerW();
}

/*---------------------------------------------------------------
					GUID2Str
   Gets the GUID of a network based on its handler in Windows
 ---------------------------------------------------------------*/
		/** Transforms a GUID structure (in Windows format) to a string
		 * \exception std::exception In case there is a failure
		 * \return std::string returns a string containing the GUID
		 */

std::string GUID2Str(GUID ifaceGuid)
{
	// Variables
	int iRet;
	errno_t wctostr;
	size_t sizeGUID;

	WCHAR GuidString[39] = {0};
	char GuidChar[100];


	std::string outputString;

	// Call the API function that gets the name of the GUID as a WCHAR[]
    iRet = StringFromGUID2(ifaceGuid, (LPOLESTR) &GuidString, sizeof(GuidString)/sizeof(*GuidString)); 
            // For c rather than C++ source code, the above line needs to be
            // iRet = StringFromGUID2(&pIfInfo->InterfaceGuid, (LPOLESTR) &GuidString,
            //     sizeof(GuidString)/sizeof(*GuidString));


	// translate from a WCHAR to string if no error happened
	if (iRet == 0){
		THROW_EXCEPTION("StringFromGUID2 failed\n");
	}	else {
		wctostr = wcstombs_s(&sizeGUID, GuidChar, 100, GuidString, 100 );
		if ( (wctostr == EINVAL) || (wctostr == ERANGE) ){
			THROW_EXCEPTION("wcstombs_s failed\n");
		} else {
			outputString = std::string(GuidChar);
		}
	}

	return outputString;
}

/*---------------------------------------------------------------
					ListInterfaces
   Gets a list of the interfaces
 ---------------------------------------------------------------*/

std::vector<std::string>	CWirelessPower::ListInterfaces()
{
	// In windows, this function is a wrapper to ListInterfacesW

	std::vector<PWLAN_INTERFACE_INFO> ifaces;					// vector containing the interface entries (Windows format)
	std::vector<PWLAN_INTERFACE_INFO>::iterator ifacesIter;		// iterator to run through the previous list
	std::vector<std::string> output;							// output vector of strings

	// get the list
	ifaces = ListInterfacesW(hClient);

	// iterate thrugh list and get each GUID as a string
	for (ifacesIter = ifaces.begin(); ifacesIter != ifaces.end() ; ifacesIter++){
		output.push_back(GUID2Str((*ifacesIter)->InterfaceGuid));
	}

	return output;
}


/*---------------------------------------------------------------
					GetInterfaceW
   Gets a handler for the interface (Windows)
 ---------------------------------------------------------------*/

		/** Gets a handle to the interface that has been set by setNet() (in Windows format)
		 * \exception std::exception In case there is a failure
		 * \return PWLAN_INTERFACE_INFO returns a handle to the interface
		 */

PWLAN_INTERFACE_INFO GetInterfaceW(std::string guid, HANDLE hClient)
{
	// Get interface given the GUID as a string (by the guid property of the object)


	std::vector<PWLAN_INTERFACE_INFO> ifaceList;						// interface list
	std::vector<PWLAN_INTERFACE_INFO>::iterator ifaceIter;				// iterator
	PWLAN_INTERFACE_INFO output = NULL;									// interface info element

	// get a list of all the interfaces
	ifaceList = ListInterfacesW(hClient);

	// search for the interface that has the given GUID
	for(ifaceIter = ifaceList.begin(); ifaceIter != ifaceList.end(); ifaceIter++){
		if (GUID2Str((*ifaceIter)->InterfaceGuid) == guid){
			output = *ifaceIter;
			break;
		}
	}

	return output;
}


/*---------------------------------------------------------------
					ListNetworksW
   Gets a list of the networks available for the interface (in Windows)
 ---------------------------------------------------------------*/

		/** Gets a list of the networks available for an interface (in Windows format)
		 * \exception std::exception In case there is a failure
		 * \return std::vector returns handles to the available networks of a given interface
		 * \param iface handle to the WiFi interface
		 */
std::vector<PWLAN_AVAILABLE_NETWORK>	ListNetworksW(PWLAN_INTERFACE_INFO iface, HANDLE hClient)
{
	// Start variables
	DWORD dwResult = 0;
    PWLAN_AVAILABLE_NETWORK_LIST pBssList = NULL;	// list of available networks
    PWLAN_AVAILABLE_NETWORK pBssEntry = NULL;		// information element for one interface
	GUID ifaceGuid = iface->InterfaceGuid;			// Get GUID of the interface
	int j;
	std::vector<PWLAN_AVAILABLE_NETWORK> outputVector;	// output vector
	WCHAR GuidString[39] = {0};
	// Force a scan (to obtain new data)
	PWLAN_RAW_DATA pIeData;
	WlanScan((HANDLE)hClient, &ifaceGuid, NULL, pIeData, NULL);


	// Call the Windows API and get a list of the networks available through the interface
	dwResult = WlanGetAvailableNetworkList((HANDLE)hClient, &ifaceGuid, 0, NULL, &pBssList);

	// Check the result of the call
	if (dwResult != ERROR_SUCCESS) {
		// In case an error ocurred
		std::stringstream excmsg;
		excmsg << "WlanGetAvailableNetworkList failed with error: " << dwResult << std::endl;
		THROW_EXCEPTION(excmsg.str());
	} else {
		// for each network, get its info and save it
		for (j = 0; j < pBssList->dwNumberOfItems; j++) {
			pBssEntry = (WLAN_AVAILABLE_NETWORK *) & pBssList->Network[j];	// get entry for network
			outputVector.push_back(pBssEntry);	// save entry
		}
	}
	return outputVector;
}


/*---------------------------------------------------------------
					ListNetworks
   Gets a list of the networks available for the interface
 ---------------------------------------------------------------*/

std::vector<std::string>	CWirelessPower::ListNetworks()
{
	// Wrapper function to ListNetworksW in Windows
	PWLAN_INTERFACE_INFO iface;			// Information element for an interface
	std::vector<std::string> output;	// Output vector

	iface = GetInterfaceW(guid,(HANDLE)hClient);			// Get the interface handler

	// Get the list of networks
	std::vector<PWLAN_AVAILABLE_NETWORK> pBssList = ListNetworksW(iface,(HANDLE)hClient);

	// Iterate through the list and save the names as strings
	std::vector<PWLAN_AVAILABLE_NETWORK>::iterator netIter;
	for(netIter = pBssList.begin(); netIter != pBssList.end() ; netIter++){
		output.push_back( std::string((char*)((*netIter)->dot11Ssid.ucSSID)));
	}

	return output;

}


/*---------------------------------------------------------------
					GetNetworkW
   Gets a handler to a wireless network in Windows
 ---------------------------------------------------------------*/
/** Gets a handle to the network that has been set by setNet() (in Windows format)
		 * \exception std::exception In case there is a failure
		 * \return PWLAN_AVAILABLE_NETWORK returns a handle to the network
		 */
PWLAN_AVAILABLE_NETWORK GetNetworkW(HANDLE hClient, std::string ssid, std::string guid)
{
	// Variables
	PWLAN_INTERFACE_INFO iface;			// interface handler
	PWLAN_AVAILABLE_NETWORK output;		// output network handler

	// Get a handler to the interface
	iface = GetInterfaceW(guid, hClient);

	// Get the list of networks
	std::vector<PWLAN_AVAILABLE_NETWORK> pBssList = ListNetworksW(iface,hClient);


	// Iterate through the list and find the network that has the matching SSID
	std::vector<PWLAN_AVAILABLE_NETWORK>::iterator netIter;
	for(netIter = pBssList.begin(); netIter != pBssList.end() ; netIter++){
		if (std::string((char*)((*netIter)->dot11Ssid.ucSSID)) == ssid ){
			output = *netIter;
			break;
		}
	}

	return output;
}



/*---------------------------------------------------------------
					GetPower
   Gets the power of the network
 ---------------------------------------------------------------*/
int		CWirelessPower::GetPower()
{
	//int iRSSI = 0;					// Received signal strength indication
	PWLAN_AVAILABLE_NETWORK wlan;	// handler to the network

	// Get a handler to the network
	wlan = GetNetworkW((HANDLE)hClient,ssid,guid);
/*
	// Calculate the RSSI
	if (wlan->wlanSignalQuality == 0)
		iRSSI = -100;
	else if (wlan->wlanSignalQuality == 100)
		iRSSI = -50;
	else
		iRSSI = -100 + (wlan->wlanSignalQuality/2);

	return iRSSI;
*/

	return wlan->wlanSignalQuality;
}



#endif //  end of Windows code