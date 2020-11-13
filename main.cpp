/**
*  Govee BLE scanner.
*  Full BLE scanner with govee manufacturer packet interpretation
*  to decode temperature, humidity and battery data.
*/

#include <cstdlib>
#include <csignal>
#include <iostream>
#include <cstdlib>
#include <string>
#include <map>
#include <queue>
#include <sys/ioctl.h>
#include <locale>
#include <iomanip>
#include <sstream>
#include <sys/types.h>
#include <unistd.h>

#include "bleScan.h"
#include "blePacket.h"

#define DEBUGLEVEL 1
#define SHOWALL		 0

//! Scanning flag
volatile bool isScanning = true;

//!  Operator for bdaddr_t maps
bool operator <(const bdaddr_t &a, const bdaddr_t &b)
{
	unsigned long long A = a.b[5];
	A = A << 8 | a.b[4];
	A = A << 8 | a.b[3];
	A = A << 8 | a.b[2];
	A = A << 8 | a.b[1];
	A = A << 8 | a.b[0];
	unsigned long long B = b.b[5];
	B = B << 8 | b.b[4];
	B = B << 8 | b.b[3];
	B = B << 8 | b.b[2];
	B = B << 8 | b.b[1];
	B = B << 8 | b.b[0];
	return(A < B);
}

//! Signal handler for SIGINT
void SignalHandlerSIGINT(int signal)
{
	(void) signal;
	isScanning = false;
	std::cerr << "SIGINT caught." << std::endl;
}

//! Signal handler for SIGHUP
void SignalHandlerSIGHUP(int signal)
{
	(void) signal;
	isScanning = false;
	std::cerr << "SIGHUP caught." << std::endl;
}

//! Main
int main(void)
{

	// signal handlers
	typedef void(*SignalHandlerPointer)(int);
	SignalHandlerPointer previousHandlerSIGINT = signal(SIGINT, SignalHandlerSIGINT);
	SignalHandlerPointer previousHandlerSIGHUP = signal(SIGHUP, SignalHandlerSIGHUP);

	// Data
	BLEScan gble;												//!< BLEScan object
	std::map<bdaddr_t,int> goveeMap;		//!< Map for known Govee devices
	isScanning = true;									//!< Scanning flag



	std::cout << "------ STARTED ----- " << std::endl;

	if (gble.connect())
	{
		while (isScanning)
		{
			BLEPacket bp;
			if (gble.scan(&bp))
			{
				std::string data;

				// print packet info if it's in the list of Govee devices
				if (goveeMap.find(bp.bdaddr) != goveeMap.end() || SHOWALL)
				{
					bp.printInfo(DEBUGLEVEL);
				}

				// see if we have manufacturer data
				std::map<int,BLEPacket::t_info>::iterator it = bp.infoBlocks.find(0xff);
				if (it!=bp.infoBlocks.end())
				{
					// do we have a packet starting with 0x88EC? If so, we likely have a Govee sensor
					if (it->second.data[0]==0x88 && it->second.data[1]==0xEC)
					{
						// add to govee map if it doesn't exist yet;
						if (goveeMap.find(bp.bdaddr) == goveeMap.end())
						{
							bp.printInfo(DEBUGLEVEL);
							goveeMap.insert(std::make_pair(bp.bdaddr,1));
						}
						// print data
						int dtemp = ((signed char)(it->second.data[4]) << 8) | int(it->second.data[3]);
						int dhum = int(it->second.data[6]) << 8 | int(it->second.data[5]);
						int dbat = int(it->second.data[7]);
						float temp = (float(dtemp)/100*9/5)+32;
						std::cout.setf(std::ios::fixed);
						std::cout << std::setprecision(2);
						std::cout << ANSI_BOLD << ANSI_COLOR_BLUE << "GOVEE " << ANSI_COLOR_RED << bp.addr << ANSI_COLOR_RESET;
						std::cout << " - Temp=" << float(dtemp)/100 << "(" << temp <<"F), Hum="<< float(dhum)/100;
						std::cout << "%, Bat=" << dbat << "%, RSSI= "<< int((signed char)bp.rssi) << "dBm" << std::endl;
					} // 088EC
				} // manufacturer info

				usleep(100);
			} // scan
		} // while
		gble.disconnect();
	} else {
		std::cerr << "Could not connect to BLE device." << std::endl;
	}

	signal(SIGHUP, previousHandlerSIGHUP);
	signal(SIGINT, previousHandlerSIGINT);


}
