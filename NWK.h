/*
 * NWK.h
 *
 *  Created on: 12 Jun 2012
 *      Author: Richard
 */

#ifndef NWK_H_
#define NWK_H_

/*
 * NWK.c
 *
 *  Created on: 12 Jun 2012
 *      Author: Richard
 */


#define  PACKET_LEN         (0x08)			// PACKET_LEN <= 61

//struct packetStructTag;
typedef struct packetStructTag
{
	unsigned char buffer[8];
	unsigned char length;
}packetStruct;

extern volatile int joinCallback;
extern volatile int ackCallback;
extern volatile int beaconCallback;
extern volatile int pingCallback;
extern volatile int searchCallback;



void NwkBuildPacket(void);
void NwkRegistration(void);
void NwkJoin(void);
void NwkBeacon(int attempt);
void NwkPing(void);
void NwkSearch(void);
void NwkListenOn(void);
void NwkListenOff(void);
void NwkListen(void);
void NwkAck(int ackJack, unsigned char destAddr);
int NwkProcessPacket(void);
void NwkPacket(unsigned char *packet, int packetType, unsigned char destAddr);
void NwkIdFlashWrite(unsigned char networkID, unsigned char myID);
int LocalJoin(void);
int CheckPacketHeader(unsigned char thisNwkID, unsigned char thisDestAddr);

int GetPacketType(void);

#endif /* NWK_H_ */
