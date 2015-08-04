/*************************************
Process Arduino
Data Rate : Determined by program
Frequency: 433Mhz
Modulation: LoRA
Transmit mode : continuous transmit
*************************************/

#include <SoftwareSerial.h>
#include <sx1278.h>

/*************************************
Software Serial Setup
Data Rate : 9600bps
Pinouts: RX (yellow wire) -> TXO (pin 0), TX (red wire) -> RX0, GNd (Blue) -> GND
Transmit mode : continuous transmit
*************************************/

SoftwareSerial mySerial(1, 0);    // RX, TX


unsigned char mode; //lora --1 / FSK --0
unsigned char Freq_Sel; //
unsigned char Power_Sel; //
unsigned char Lora_Rate_Sel; //
unsigned char BandWide_Sel; //
unsigned char Fsk_Rate_Sel; //


/*
Pro Micro Pintouts

SCK 	15          
MOSI	16         
MISO	14
RESET 	A3, 21
DIO0 / D5	A2, 20
NSEL / CS       A0, 18
LED / D14 	A1, 19	



*/

// Pinout for Arduino Pro Micro
// 
//
//
//

int led = 19; 					// Pro Micro P19, A1, D14
int nsel = 18;					// Pro Micro P18, A0, 	SX1278	CS
int sck = 15;
int mosi = 16;
int miso = 14;
int dio0 = 20;
int reset = 21;

// Define Modes
#define SX1278_MODE_RX_CONTINUOUS			0x00
#define SX1278_MODE_TX						0x00
#define SX1278_MODE_SLEEP					0x00
#define SX1278_MODE_STANDBY					0x00


void SPICmd8bit(unsigned char WrPara)
{
	unsigned char bitcnt;
	
	digitalWrite(nsel, LOW);
	
	digitalWrite(sck, LOW);
	
	for (bitcnt=8; bitcnt != 0; bitcnt--)
	{
		digitalWrite(sck, LOW);
	
			if (WrPara&0x80) {
				digitalWrite(mosi, HIGH);
				}
			else {
				digitalWrite(mosi, LOW);
				}
		
		digitalWrite(sck, HIGH);
		WrPara <<= 1;
	}
	
	digitalWrite(sck, LOW);
	digitalWrite(mosi, HIGH);
}	
	
unsigned char SPIRead8bit(void)
{
	unsigned char RdPara = 0;
	unsigned char bitcnt;
	
	digitalWrite(nsel, LOW);
	digitalWrite(mosi, HIGH);
	
	for ( bitcnt=8; bitcnt !=0; bitcnt--) {
		digitalWrite(sck, LOW);
		RdPara <<= 1;
		digitalWrite(sck, HIGH);
	
		if(digitalRead(miso)) {
			RdPara |= 0x01;
			}
		else {
			RdPara |= 0x00;
			}
			
	}
	digitalWrite(sck, LOW);
	return(RdPara);
}


unsigned char SPIRead(unsigned char adr)
{
	unsigned char tmp;
	SPICmd8bit(adr);
	tmp = SPIRead8bit();
	digitalWrite(nsel, HIGH);
	return(tmp);
}


void SPIWrite(unsigned char adr, unsigned char WrPara)
{
	digitalWrite(nsel, LOW);
	SPICmd8bit(adr|0x80);
	SPICmd8bit(WrPara);
	
	digitalWrite(sck, LOW);
	digitalWrite(mosi, HIGH);
	digitalWrite(nsel, HIGH);
}


void SPIBurstRead(unsigned char adr, unsigned char *ptr, unsigned char leng)
{
	unsigned char i;
		if (leng<=1) {
			return;
		}
		else {
			digitalWrite(sck, LOW);
			digitalWrite(nsel, LOW);
			SPICmd8bit(adr);
	
			for (i=0; i<leng; i++ ) {
				ptr[i] = SPIRead8bit();
			}
			
			digitalWrite(nsel, HIGH);
		}
}


void BurstWrite(unsigned char adr, unsigned char *ptr, unsigned char leng)
{
	unsigned char i;
	
	if (leng <= 1) {
		return;
	}
	else {
	digitalWrite(sck, LOW);
	digitalWrite(nsel, LOW);
	SPICmd8bit(adr|0x80);
	
		for (i=0; i<leng; i++) {
			SPICmd8bit(ptr[i]);
		}
	digitalWrite(nsel, HIGH);
	}
}

	
	// Parameter Table Definition

unsigned char sx1278FreqTbl[1][3] =
{
	{ 0x6C, 0x80, 0x00}, //434Mhz
};


unsigned char sx1278PowerTbl[4] =
{
	0xFF,
	0xFC,
	0xF9,
	0xF6,
};


unsigned char sx1278SpreadFactorTbl[7] =
{
	6,7,8,9,10,11,12
};	


unsigned char sx1278LoRaBwTbl[10] =
{
	0,1,2,3,4,5,6,7,8,9		// 7.8Khz, 10.4KHz, 15.6KHz, 20.8KHz, 31.2KHz, 41.7KHz, 62.5KHz, 125KHz, 250KHz, 500KHz
};


unsigned char sx1278Data[] = {"Mark1 Lora sx1278"};

unsigned char RxData[64];

	
void sx1278_Standby(void)
{
	SPIWrite(LR_RegOpMode, 0x09);	// Standby & Low Frequency mode
	// SPIWrite(LR_RegOpMode, 0x01);	// standby high frfequency mode
}	

void sx1278_Sleep(void)	
{	
	SPIWrite(LR_RegOpMode, 0x08); 	// Sleep & Low Frequency mode
	// SPIWrite(LR_RegOpMode, 0x00); 	// Sleep / high frequency mode
	
}	

void sx1278_EntryLoRa(void)
{
	SPIWrite(LR_RegOpMode, 0x88); 	// Low frequency mode
	// SPIWrite(LR_RegOpMode, 0x80); 	// Sigh frequency mode
}

void sx1278_LoRaClearIrq(void)
{
	SPIWrite(LR_RegIrqFlags, 0xFF);
}


unsigned char sx1278_LoRaEntryRx(void)
{
	unsigned char addr;
	mySerial.println("Enter sx76 Config");
	sx1278_Config();	// setting base parater
	mySerial.println("Write SPI");
	
	SPIWrite(REG_LR_PADAC, 0x84 );
	SPIWrite(LR_RegHopPeriod, 0xFF);
	SPIWrite(REG_LR_DIOMAPPING1, 0x01 );
	
	SPIWrite(LR_RegIrqFlagsMask, 0x3f);
	
    mySerial.println("LoRa Clear IRQ");
	sx1278_LoRaClearIrq();
	
	SPIWrite(LR_RegPayloadLength, 21);
	
	addr = SPIRead(LR_RegFifoRxBaseAddr);
	
	SPIWrite(LR_RegFifoAddrPtr, addr);
	SPIWrite(LR_RegOpMode, 0x8d);			// Set the Operating Mode to Continuos Rx Mode && Low Frequency Mode
	
    mySerial.print("read SPI LR_RegModemStat");
	while (1) 	{
			if ((SPIRead(LR_RegModemStat) & 0x04) == 0x04) {
				break;
            }    
				return 0;    
	}
}	


unsigned char sx1278_LoRaReadRSSI(void)
{
	unsigned int temp = 10;
	temp = SPIRead(LR_RegRssiValue);
	temp = temp + 127 - 137;
	return (unsigned char) temp;
}



unsigned char sx1278_LoRaRxPacket (void)
{
	unsigned char i;
	unsigned char addr;
	unsigned char packet_size;
	
        mySerial.print("processing LoRaRx Packet");
        mySerial.println(" ");
	
	if (digitalRead(dio0)) 	{
	       mySerial.println("DIO_0 shows packet recieved");
		
        	for (i = 0; i < 32; i++ ) {
				RxData[i] = 0x00;
			}
	
	addr = SPIRead(LR_RegFifoRxCurrentAddr);
	SPIWrite(LR_RegFifoAddrPtr, addr);		// RXBaseAddr --> FiFoAddrPtr
	
	if (sx1278SpreadFactorTbl[Lora_Rate_Sel] == 6 ) {
		packet_size = 21;
	} else {
		packet_size = SPIRead(LR_RegRxNbBytes);
		}
	
	SPIBurstRead(0x00, RxData, packet_size);
	
	sx1278_LoRaClearIrq();
	
	for ( i = 0; i< 17; i++ ) {
		if (RxData[i] != sx1278Data[i] ) break;
	}
	
	if ( i > 17 ) {
		return (i);
	} else return (0);
	
	}
	else return(0); // if !(digitalRead(dio0) --> this is important for recieving packets
}


unsigned char sx1278_LoRaEntryTx(void)
{
	unsigned char addr, temp;
	
	sx1278_Config();	// setting base parater
	
	SPIWrite(REG_LR_PADAC, 0x87 );
	SPIWrite(LR_RegHopPeriod, 0x00);
	SPIWrite(REG_LR_DIOMAPPING1, 0x41 );
	
	sx1278_LoRaClearIrq();
	SPIWrite(LR_RegIrqFlagsMask, 0xF7);
	SPIWrite(LR_RegPayloadLength, 21);
	
	addr = SPIRead(LR_RegFifoTxBaseAddr);
	
	SPIWrite(LR_RegFifoAddrPtr, addr);
	
	while (1) {
		temp = SPIRead(LR_RegPayloadLength);
		if (temp==21) break;
	}
	
}
	
unsigned char sx1278_LoRaTxPacket(void)
{
	unsigned char TxFlag = 0;
	unsigned char addr;
	
	BurstWrite(0x00, (unsigned char *)sx1278Data, 21);
	SPIWrite(LR_RegOpMode, 0x8b);
	
	while(1) {
		if (digitalRead(dio0)) 	{
			SPIRead(LR_RegIrqFlags);
			sx1278_LoRaClearIrq();
			sx1278_Standby();
			break;
		}
	}
}

unsigned char sx1278_ReadRSSI(void)
{
	unsigned char temp = 0xff;
	
	temp = SPIRead(0x11);
	temp >>= 1;
	temp = 127 - temp;
	return temp;
}



void sx1278_Config(void) {
	unsigned char i;
	sx1278_Sleep();	// modem must be in sleep mode
	
	for ( i = 250; i!= 0; i-- );
	
	delay(15);
	
	//lora mode
	sx1278_EntryLoRa();
	
	// SPIWrite(0x5904); // change digital regulator from 1.6V to 1.47V: see errata note
	
	BurstWrite(LR_RegFrMsb, sx1278FreqTbl[Freq_Sel],3); //set the frequency parameter
	
	// set the base parameters
	
	SPIWrite(LR_RegPaConfig, sx1278PowerTbl[Power_Sel]); // set the output power parameter
	
	SPIWrite(LR_RegOcp, 0x0B); 
	SPIWrite(LR_RegLna, 0x23);
	
	if(sx1278SpreadFactorTbl[Lora_Rate_Sel]==6)
	{	
		unsigned char tmp;
	
		SPIWrite(LR_RegModemConfig1, ((sx1278LoRaBwTbl[BandWide_Sel] << 4) +  (CR<<1 ) + 0x01 ));
		// Implicit Enable CRC Enable (0x02) & Error Coding rate 4/5 (0x01), 4/6 (0x02), 4/7 (0x03), 4/8 (0x04)
	
		SPIWrite(LR_RegModemConfig2, ((sx1278SpreadFactorTbl[Lora_Rate_Sel] << 4 ) + (CRC<<2 ) + 0x03 ));
	
		tmp = SPIRead(0x31);
		tmp &= 0xF8;
		tmp |= 0x05;
		SPIWrite(0x31,tmp);
		SPIWrite(0x37, 0x0C);
	}
	
	else {
		SPIWrite(LR_RegModemConfig1, ((sx1278LoRaBwTbl[BandWide_Sel] << 4)+(CR<<1 )+0x00);
		SPIWrite(LR_RegModemConfig2, ((sx1278SpreadFactorTbl[Lora_Rate_Sel]<<4)+(CRC<<2)+0x03));
	}
	
	SPIWrite(LR_RegSymbTimeoutLsb, 0xFF);
	SPIWrite(LR_RegPreambleMsb, 0x00);
	SPIWrite(LR_RegPreambleLsb, 12);
	SPIWrite(REG_LR_DIOMAPPING2, 0x01);
	sx1278_Standby();
}	
	
	
void setup() {
    pinMode(led, OUTPUT);
    pinMode(nsel, OUTPUT);
    pinMode(sck, OUTPUT);
    pinMode(mosi, OUTPUT);
    pinMode(miso, OUTPUT);
    pinMode(reset, OUTPUT);
    
    Serial.begin(9600);
    mySerial.begin(9600);
    mySerial.println("Software Serial Port Connected");
    
}    


void loop() {
// this is the main code to run repeatedly
	mode 	= 0x01; 	//lora mode
	Freq_Sel 	= 0x00; 	//433Mhz
	Power_Sel 	= 0x00; 	//
	Lora_Rate_Sel 	= 0x06;
	BandWide_Sel 	= 0x07;
	Fsk_Rate_Sel 	= 0x00;
	
        Serial.println("sx1276 Config \n");
		mySerial.println("sx1276 Config \n");
        sx1278_Config();
        Serial.println("sx1276 LoRa Entry Recieve \n");
        mySerial.println("sx1276 LoRa Entry Recieve \n");
		sx1278_LoRaEntryRx();

        mySerial.println("sx1276 LoRa Entry Recieve Complete \n");	
        mySerial.println("RX Data buffer contains");
        for (int i=0; i<64; i++) {
          mySerial.print(RxData[i]);
        }
        mySerial.println(" ");


	digitalWrite(led, HIGH); 	// turn the LED on
	delay(500); 	// wait for 500ms
	digitalWrite(led, LOW); 	// turn the LED off
	delay(500);	// wait for 500ms
	
        int loopCnt = 0;
	while(1)
	{
	// Master
        /*
	      	digitalWrite(led, HIGH);
                delay(200);
                Serial.print(loopCnt);
                Serial.print(": Check Lora Entry Tx \n");
                mySerial.print(loopCnt);
                mySerial.print(": Check Lora Entry Tx \n");
	sx1278_LoRaEntryTx();
                Serial.print(loopCnt);
                Serial.print(": Check Lora TX packet \n");
                mySerial.print(loopCnt);
                mySerial.print(": Check Lora TX packet \n");
	sx1278_LoRaTxPacket();
	digitalWrite(led, LOW);
	sx1278_LoRaEntryRx();
	delay(200);
                loopCnt++;
          */
	/*
          if (sx1278_LoRaRxPacket())
	{
	mySerial.println("Packet Recieved");
                        digitalWrite(led, HIGH);
	delay(500);
	digitalWrite(led, LOW);
	delay(500);
	}
	*/

	// Slave
	mySerial.println("Waiting for RX packet  ");
        char fromSPI = SPIRead(LR_RegFifoRxCurrentAddr);
        mySerial.print("Reg FIFO Rx Current Address  ");
        mySerial.print(fromSPI, HEX);
        mySerial.println(" ");
        mySerial.println(" ");
        delay(1000);
        if (dio0 == HIGH)
          mySerial.println("DIO-0 high");
        else
          mySerial.println("DIO-0 low");
//          dio0 = !dio0;
        
        // check the Modem Status Indicators
        
        // frmo thr ModemStatus bits in RegModemStat
        char lrModemStat = SPIRead(LR_RegModemStat);
        // Signal Detected bit 0
        mySerial.println();
        mySerial.print("RegModem (LoRa) Stat 0x");
        mySerial.print(lrModemStat, HEX);
        mySerial.println();
               
        // Signal synchoronized bit 1
        
        // head info valid bit 3
        
        // show the OpMode
        
       char showOpMode = SPIRead(LR_RegOpMode);

       if (showOpMode && 0x80 == 0 )
           mySerial.println("OPMode is FSK");
       else
           mySerial.println("OPMode is LoRa");
        
       showOpMode = SPIRead(LR_RegOpMode) && 0xF8;
       mySerial.print("OpMode Regsiters: ");
       mySerial.print(showOpMode, HEX);
       mySerial.println(" ");
       
       switch(showOpMode) { 
       case 0:
          mySerial.println("OpMode is 0 SLEEP");
          break;
      
       case 1:
          mySerial.println("OpMode is 1 STBY");
          break;

       case 2:       
          mySerial.println("OpMode is 2 FSTX");      
          break;

       case 3:
          mySerial.println("OpMode is 3 TX");      
          break;

       case 4:
          mySerial.println("OpMode is 4 FSRX");         
          break;

       case 5:      
          mySerial.println("OpMode is 5 RXCONTINUOUS");      
          break;

       case 6:
          mySerial.println("OpMode is 6 RXSINGLE");         
          break;

       case 7:       
          mySerial.println("OpMode is 7 CAN");      
          break;
          
       }
       
       char modemStat = SPIRead(LR_RegModemStat);
       modemStat = modemStat && B00011111;
       mySerial.print("Modem Register Status :");
       mySerial.print(modemStat, HEX);
       mySerial.println(" ");
     
     /*  
       if ((modemStat && B00001000) == B00001000 )
         mySerial.println("Modem Status x18 = MODEM CLEAR");
       
       if ((modemStat && B00000100) == B00000100 )
         mySerial.println("Modem Status x18 = HEADER INVALID");

       if ((modemStat && B00000010) == B00000010 )
         mySerial.println("Modem Status x18 = NO SIGNAL SYNC");

       if ((modemStat && B00000001) == B00000001 )
         mySerial.println("Modem Status x18 = NO SIGNAL DETECTED");
     */  
  /*     
       if (showOpMode == 0) 
       {
         mySerial.println("confirming that Opmode is SLEEP");
         mySerial.println(SPIRead(LR_RegOpMode), BIN); 
         mySerial.println("forcing Opmode to STANDBY");
         char newOpMode = (SPIRead(0x01) || 0x01 );
         SPIWrite(0x01, newOpMode);        
        
      
         
       }
    */   
       char RSSIVal = SPIRead(0x1B);
       mySerial.print("Current RSSI : ");
       mySerial.print(RSSIVal, HEX);
       mySerial.print(" dBm");
       mySerial.println(" ");
       
       
	if(sx1278_LoRaRxPacket())
	{
	mySerial.println("Packet Recieved");
  	digitalWrite(led, HIGH);
	delay(500);
	digitalWrite(led, LOW);
	delay(500);
	mySerial.println("RX Data buffer contains");
                for (int i=0; i<64; i++)
                {
                  mySerial.print(RxData[i]);
                }
                mySerial.println(" ");
                sx1278_LoRaEntryRx();
	
	/*
                digitalWrite(led, HIGH);
	
	sx1278_LoRaEntryTx();
	sx1278_LoRaTxPacket();
	digitalWrite(led, LOW);
	sx1278_LoRaEntryRx();
	*/

	
              }	
        
}
}



