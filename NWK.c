/*
 * NWK.c
 *
 *  Created on: 12 Jun 2012
 *      Author: Richard
 */

#include "NWK.h"
#include "RF.h"
#include "common.h"
#include "Timer.h"


#define JOIN 1
#define BEACON 2
#define ACK 3
#define JACK 4
#define PING 5
#define SEARCH 6

#define BROADCAST 0xFF

unsigned char deviceList[10];	// max of 10 devices
unsigned int numDevices = 0;	// number of devices on the network

volatile unsigned char rxBuffer[PACKET_LEN+2];
unsigned char rxBufferLength = 0;


/* Set up the initial nwkID and base address different depending on device */
#ifdef __AP__
	unsigned char nwkID = 0x01;
	unsigned char baseAddr = 0xFF;
#else
	unsigned char baseAddr = 0xFF;	// Initiate as 0xFF
	unsigned char nwkID;		// Initiate as 0xFF
#endif

unsigned char myAddr = 0xFF;
unsigned int  idFlashWrite = 0;

int packetType;
unsigned int seqNum = 0;

volatile int joinCallback = 0;
volatile int ackCallback = 0;
volatile int jackCallback = 0;
volatile int beaconCallback = 0;
volatile int pingCallback = 0;
volatile int searchCallback = 0;

extern unsigned volatile int rxCallback;

void NwkRegistration(){

	/* Look for network id in flash memory */

	/* If network id exists */

	if (idFlashWrite){

		/* Then use this and set deviceID */
		__no_operation();
		//deviceID =
	}

	/* Else run join procedure */
	else{

		/* Send join request */
		NwkJoin();
		/* delay cycles */
	}

}

void NwkJoin(){

	/* Construct join packet */
	unsigned char packet[8];
	NwkPacket(packet, JOIN, BROADCAST);

	while(!jackCallback){

		Transmit( (unsigned char*)packet, sizeof packet);
		/* Expect ACK */
		Sleep(3);

		StartTimer(500); // Wait a period of time for the ack to come back
		NwkListen();	// Listen for it...	}
	}

	jackCallback = 0;
}

void NwkBeacon(int attempt){

	/* Green LED lights and turns off when ACK received.
	 * If lit continously, still waiting for ack.
	 */

	P1OUT |= 0x01;	// Turn on green led

	unsigned char packet[8];	// creat packet array

	/* Build usual beacon packet */
	NwkPacket(packet, BEACON, baseAddr);

	while(!ackCallback){

		/* Back off code in case of collision on network */

		//if(attempt){
		//	StartTimer(10*nwkID*attempt); // Start timer if this isn't the first attempt to offset collisions
		//	Sleep(3);	// Sleep, could wake from packet or timer
		//}

		/* Transmit Beacon packet */
		Transmit( (unsigned char*)packet, sizeof packet);

		/* Expect ACK */
		Sleep(3);

		StartTimer(100); // Wait a period of time for the ack to come back
		NwkListen();	// Listen for it...
	}

	/* Check ACK matches Beacon */

	ackCallback = 0;

	P1OUT &= ~0x01; // Turn off green led
}

void NwkPing(){

	/* Ping the base station */

}

void NwkAck(int ackType, unsigned char destAddr){

	/* Red LED flashes on ACK complete */
	P3OUT |= BIT6;

	unsigned char packet[8];
	NwkPacket(packet, ackType, destAddr);
	Transmit( (unsigned char*)packet, sizeof packet);
	Sleep(3);
	P3OUT &= ~BIT6;
}

void NwkSearch(){

	/* Search for base stations */
}

void NwkListenOn(){

	ReceiveOn();
}

void NwkListenOff(){

	ReceiveOff();
}

void NwkListen(){

	NwkListenOn();

	Sleep(3);

	/* Program will resume here after sleeping */

	if(rxCallback) NwkProcessPacket();

	NwkListenOff();

}
int NwkProcessPacket(){

	/* Strip away header info */

	int pType = GetPacketType(); // returns packet length and copies packet from buffer

	unsigned char thisNwkID = rxBuffer[0];			// NwkID
	unsigned char thisPacketLength = rxBuffer[1];	// Packet Length
	unsigned char thisDestAddr = rxBuffer[2];		// Destination Address (us!)
	unsigned char thisSeqNum = rxBuffer[4];			// Seq Number
	signed char thisSrcAddr = rxBuffer[5];		// Source Address (where it came from)


	/* If this is the AP config, we need to set in the deviceList */
//#ifdef __AP__
//	deviceList[thisSrcAddr] = thisSeqNum;
//
//	// Update the device list on the AP to carry the latest SeqNum
//
//#else
//	if(seqNum == (thisSeqNum-1)) seqNum = thisSeqNum;
//
//	else return 0; // packets not valid !
//
//#endif

	/* Only accept packet if the NwkID is us, and the destination address is us */

	switch(pType){

	/* Only the AP can process certain packets */
	#ifdef __AP__

		case JOIN:
			joinCallback++;

			/* Do local join procedure */
			thisSrcAddr = LocalJoin();
			if(thisSrcAddr >=0) NwkAck(JACK, (unsigned) thisSrcAddr); // does field checking inside LocalJoin()
			break;

		case SEARCH:
			searchCallback++;
			break;

		case BEACON:

			/* Check the packet was for this node */
			if(CheckPacketHeader(thisNwkID, thisDestAddr)){
				beaconCallback++;
				NwkAck(ACK, thisSrcAddr);
			}
			break;

	#endif

		case ACK:

			/* Check the packet was for this node */
			if(CheckPacketHeader(thisNwkID, thisDestAddr)){
				ackCallback++;
			}

			break;

		case JACK:

			/* A JACK can only be received by a node
			 * process it here...
			 */

			/* Make sure we're not trying to process a packet we didn't request */
			if(!idFlashWrite){
				NwkIdFlashWrite(nwkID, myAddr); // * This method needs to be finished to be safe.
												// Write to flash first incase of crash

				nwkID = thisNwkID; // Set the NwkID permanently
				myAddr = thisDestAddr; // Set my address permantly
				jackCallback++;
				idFlashWrite = 1;
			}

			break;

		case PING:
			pingCallback++;
			break;

		default:
			break;

	}

	return 1;
}

//void NwkPacket(packetStruct* packet, int packetType){
void NwkPacket(unsigned char *packet, int packetType, unsigned char destAddr){

	packet[0] = nwkID;
	packet[1] = sizeof packet;
	packet[5] = myAddr;
	packet[6] = 0xAA;
	packet[7] = 0xBB;

	switch(packetType){

		case JOIN:
			packet[0] = 0xFF;	// NwkID
			packet[2] = 0xFF;	// destAddr
			packet[3] = JOIN;
			packet[4] = seqNum;
			break;
		case BEACON:
			packet[2] = baseAddr;
			packet[4] = seqNum;
			packet[3] = BEACON;
			packet[6] = 0x0;	// payload 2
			packet[7] = 0xFF;	// payload 3
			break;
		case ACK:
			packet[3] = ACK;

		#ifdef __AP__
			packet[4] = deviceList[destAddr]++;
		#else
			packet[4] = seqNum++;
		#endif

			break;
		case JACK:
			packet[0] = nwkID;	// NwkID
			packet[2] = destAddr;	// destAddr
			packet[3] = JACK;
			break;
		case PING:
			packet[3] = PING;
			break;
		case SEARCH:
			packet[3] = SEARCH;
			break;
	}

	/* Structure of packet here */


//	packet[0] = NwkID
//	packet[1] = sizeof packet;	// baseAddr
//	packet[2] = destAddr//
//	packet[3] = packetType;	// Type
//	packet[4] = seqNum;	// Payload (SeqNum)
//  packet[5] = myAddr;



}

void NwkIdFlashWrite(unsigned char networkID, unsigned char myID){
	/* Write the nwk id to flash storage */
}

void NwkBuildPacket(){

}

int GetPacketType(){

	GetRxBuffer(rxBuffer); // read the RxBuffer

	rxCallback--; // Get the most recent
	return (int) rxBuffer[3]; // return the packet type - (cast as int for switch statement)

}

int LocalJoin(){

	if( (rxBuffer[0] == 0xFF) && (rxBuffer[5] == 0xFF) ){
		// Only proceed if these fields = 0xFF

		/* Need to expect ACK from JACK */

		if(sizeof deviceList <= 10){
			numDevices++;
			deviceList[numDevices] = 1;

			/* Device has joined this network,
			 * let's send it back it's address and our nwkId
			 */

			return numDevices;
		}
	}

	else return -1;

}

int CheckPacketHeader(unsigned char thisNwkID, unsigned char thisDestAddr){
	if(thisNwkID == nwkID && thisDestAddr == myAddr) return 1;
	else return 0;
}
