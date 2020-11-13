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

volatile bool isScanning = true;


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
	isScanning = true;

	std::cout << "------ STARTED ----- " << std::endl;

	BLEScan gble;
	if (gble.connect())
	{
		while (isScanning)
		{
			BLEPacket bp;
			if (gble.scan(&bp))
			{
				std::string data;

				// see if we have manufacturer data
				std::map<int,BLEPacket::t_info>::iterator it = bp.infoBlocks.find(0xff);
				if (it!=bp.infoBlocks.end())
				{
					// do we have a packet starting with 0x88EC? If so, we likely have a Govee sensor
					if (it->second.data[0]==0x88 && it->second.data[1]==0xEC)
					{
						bp.printInfo(3);
						int dtemp = ((signed char)(it->second.data[4]) << 8) | int(it->second.data[3]);
						int dhum = int(it->second.data[6]) << 8 | int(it->second.data[5]);
						int dbat = int(it->second.data[7]);
						float temp = (float(dtemp)/100*9/5)+32;
						std::cout.setf(std::ios::fixed);
						std::cout << std::setprecision(2);
						std::cout << "GOVEE: " << bp.addr << ",temp=" << float(dtemp)/100 << "(" << temp <<"F), hum="<< float(dhum)/100;
						std::cout << ", bat=" << dbat << ",RSSI= "<< int((signed char)bp.rssi) << "dBm" << std::endl;
					} // 088EC
				} // sc>0
				usleep(1000);
			} // scan
		} // while
		gble.disconnect();
	} else {
		std::cerr << "Could not connect to BLE device." << std::endl;
	}

	signal(SIGHUP, previousHandlerSIGHUP);
	signal(SIGINT, previousHandlerSIGINT);


}
