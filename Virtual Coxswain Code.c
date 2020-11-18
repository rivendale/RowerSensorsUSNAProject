#define BINBUFSIZE 127			// input buffer for serial port
#define BOUTBUFSIZE 127       // output buffer for serial port
#use ES308_SBC.LIB

typedef char* String20;

char status;
char ModemInRange;
char ModemSendData;
char ErrorStats;
char MenuCurrNum;
char MenuSensor;
char MenuRefresh;
String20 MLine1[14];
String20 MLine2[14];
String20 MLine3[14];
String20 MLine4[14];


int EncVal1;
int EncVal2;
float IRVoltageCatch;
float IRVoltageFinish;
float IRVoltageRead;
int SPM,SPMold;
char NewStroke;

unsigned int Seconds;
unsigned int Minutes;
unsigned int Hours;

int EncMisc;
float VoltMisc;
int EncAtCatch;
int EncAtFinish;

int EncTilt;
int EncOar;
char OnDrive;  //else recovery
char AtCatch; 
char AtFinish; 
char OarOut;
float Power;
float IR;
float LastIR;
int IntrCount;

void ModemSend(char *szp)  
{
	while (*szp)
		serBputc(*szp++);					
}

String20 ModemReceive(void)
{
	int lcv;
	int pos;
	char inc;
	char InText[21];
	lcv = 0;
	pos = 0;
	InText[0] = ".";
	InText[1] = 0;
	InText[19] = 0;
	while (lcv < 30 && inc != 0 && pos < 20)
		{
		inc = serBgetc();
		if (inc != 255)
			{
			InText[pos] = (char)inc;
			pos++;
			}
		lcv++;
		}
	return InText;
}

void ModemTest(void)
{
	ModemSend("abc");
}


void WriteByte(char c)
{
	if (status == 1)
	{
		BitWrPortI(PBDR,&PBDRShadow,1,6);
		BitWrPortI(PBDR,&PBDRShadow,1,7);
		DIO_SendByte(c);
		BitWrPortI(PBDR,&PBDRShadow,0,7);
		BitWrPortI(PBDR,&PBDRShadow,0,6);
		MsDelay (2);
	}
	else if (status == 2)
	{
		BitWrPortI(PBDR,&PBDRShadow,0,6);
		BitWrPortI(PBDR,&PBDRShadow,1,7);
		DIO_SendByte(c);
		BitWrPortI(PBDR,&PBDRShadow,0,7);
		MsDelay (2);
	}
	else
	{
		printf("error: status = %d",status);
	}
	
}


void LCD_Clear ( void )
{
	status = 2;
	WriteByte(0x01);  // clear and home cursor
	MsDelay(1);	
}

void LCD_Reset(void)
{
	WriteByte(0x30);
	MsDelay(5);
	WriteByte(0x30);
	MsDelay(1);
	WriteByte(0x30);
	MsDelay(1);
}

void LCD_Init(void)
{
	status=2;
	MsDelay(15);
	LCD_Reset();
	WriteByte(0x38);   // 8-bit,1/16 dfutyy cycle, 5*7 dot matrix
	MsDelay(1);
	WriteByte(0x0C);   // display on,cursor off,blink off
	MsDelay(1);
	WriteByte(0xC0);  // increase cursor move, no display shift
	MsDelay(1);
	LCD_Clear();
}


void Display(char *szp)
{
	status = 1;
	while (*szp)
		WriteByte(*szp++);
}



void Line1 ( void )
{	status=2;			
	WriteByte(0x80);			
	MsDelay (3);					
}

void Line2 ( void )
{	status=2;			
	WriteByte(0xC0);				
	MsDelay (3);					
}

void Line3 ( void )
{	status=2;				
	WriteByte(0x94);				
	MsDelay (3);				
}

void Line4 ( void )
{	status=2;			
	WriteByte(0xD4);				
	MsDelay (3);			
}


int CheckReed(void)
{
	int B;
	B = DIO_GetByte();
	B = B & 0x80;
	if (B == 0x80)
		return 1;					// up (out of water)
	if (B == 0x00)
		return 0;               // down (in water)
}

int GetTilt(void)    // Enc #2
{
	
	EncVal2 = EncoderReadData8(2);
	if (EncVal2 > 2)
		{
		return 2;
		}
	else if (EncVal2 < -2)
		{
		return 3;
		}
	else if (EncVal2 > -2 && EncVal2 < 2)
		{
		return 1;
		}
	else
		{
		return 0;
		}
}

int GetAngleEnc(void)   // Enc #1
{
	
	EncVal1 = EncoderReadData8(1);
	if (EncVal1 < EncAtFinish + 1)
		{
		return 3;
		}
	else if (EncVal1 > EncAtCatch - 1)
		{
		return 2;
		}
	else if (EncVal1 < EncAtCatch && EncVal1 > EncAtFinish)
		{
		return 1;
		}
	else
		{
		return 0;
		}
}

float GetIR(void)    // A/D #0
{
	float IRVoltageGet;
	IRVoltageGet = Get_AD_Volts(0);
	return IRVoltageGet;
}

float GetPower(void)    // A/D #1
{
	float StrainGageVoltageGet;
	StrainGageVoltageGet = Get_AD_Volts(1);
	return StrainGageVoltageGet;
}

void UpdateSensors(void)
{
	EncTilt=GetTilt();
	EncOar=GetAngleEnc();
	OarOut= CheckReed();
	if (OnDrive)
	{
		Power = (Power +GetPower())/2.0;
	}
	IR = GetIR();
	OnDrive = 0;
	if (IR > LastIR)   // <----
	{
		if (OarOut == 0)
		{
			OnDrive = 1;    //moving up in water
		}
		if (IR < IRVoltageFinish+0.05)
			{
			if (NewStroke)
				{
					NewStroke = 0;
					SPM++;
				}
			}
	}
	if (IR < LastIR)   //  ---->
	{
		if (IR > IRVoltageFinish-0.05)  // at the finish
		{
			if (OarOut)
				NewStroke = 1;	
		}
	}
	LastIR=IR;
}

void SensorOutputMake(char MenuSensor)
{
	char textmsg1[25];
	char textmsg2[25];
	char textmsg3[25];
	char textmsg4[25];
	char* MsgIn;
	switch(MenuSensor)
		{
		case 2:
			MLine1[13]="Blade Status";
			if (OarOut)
				{    
					MLine2[13]="Oar is out of water ";
				}
			else
				{
					MLine2[13]="Oar is in the water ";
				}
		
			MLine3[13]="Time elasped: ";
			sprintf(textmsg1,"%d:%d:%d",Hours,Minutes,Seconds);
			MLine4[13]=textmsg1;
			break;
		case 3:
			MLine1[13]="Location of Oar:";
			if (EncOar == 1)
				{
				if (OnDrive)
					{
					MLine2[13]="middle of drive ";
					}
				else
					{				
					MLine2[13]="middle of recovery ";
					}
				}
			else if (EncOar == 2)
				{
				MLine2[13]="at the catch";
				}
			else if (EncOar == 3)
				{
				MLine2[13]="at the finish";
				}
			else
				{
				MLine2[13]="unknown";
				}
			
			MLine3[13]="Boat Tilt: ";
			if (EncTilt == 1)
			{
				MLine4[13]="Level";
			}
				else if (EncTilt == 2)
			{
				MLine4[13]="Off to Starboard";
			}
			else if (EncTilt == 3)
			{
				MLine4[13]="Off to Port";
			}
			else
			{
				MLine4[13]="unknown";
			}
			break;
		case 4:
			MLine1[13]="Strokes per minute:";
			sprintf(textmsg1,"%d",SPMold);
			MLine2[13]=textmsg1;
			MLine3[13]="Time for 500 meters: ";
			sprintf(textmsg2,"%f",Power);
			MLine4[13]=textmsg2;
			break;
		case 5:
			sprintf(textmsg1,"IR: %4.2f",IR);
			MLine1[13]=textmsg1;
			sprintf(textmsg2,"PWR: %4.2f",Power);
			MLine2[13]=textmsg2;
			sprintf(textmsg3,"Enc 1: %d",EncVal1);
			MLine3[13]=textmsg3;
			sprintf(textmsg4,"Enc 2: %d",EncVal2);
			MLine4[13]=textmsg4;
			break;
		case 6:
			ModemTest();
			MsgIn = ModemReceive();
			MLine1[13]= "Message from Modem: ";
			MLine2[13]= "--------------------";
			MLine3[13]=MsgIn;
			MLine4[13]="---------------------";
			break;
		}
}



nodebug root interrupt void ISR_R(void)
{
int S;
char modemstring[80];
	S = RdPortI(TACSR);
	IntrCount++;
	if (IntrCount == 70)
	{
		UpdateSensors();
		SPMold = SPM*2;
		SPM = 0;
	}
	if (IntrCount == 140)
	{
		IntrCount = 0;
		Seconds++;
		if (ModemSendData)
				{
				sprintf(modemstring,"IR Voltage: %4.2f SPM: %d Enc Oar:  %d   Enc Tilt:  %d Oar out of water: %d",IR,SPM,EncOar,EncTilt,OarOut); 
				ModemSend(modemstring);
				ModemSend("\r");
				} 
			
		if (Seconds == 60)
		{
			Seconds = 0;
			Minutes++;
			if (Minutes == 60)
			{
				Minutes = 0;
				SPMold = SPM*2;
				SPM = 0;
				Hours++;
			}
		}
	}
/*
		Get_AD_Volts(1);
		EncoderReadData8(1)
*/
}






void ModemCheck(void)
{
	char *MsgIn;
	ModemInRange=1;
	/*
	ModemTest();
	MsgIn = ModemReceive();
	*/
}

void TimeReset(void)
{
	IntrCount = 0;
	Seconds = 0;
	Minutes = 0;
	Hours = 0;
	IntrCount=0;
}

void CalFinish(void)
{
	IRVoltageFinish=Get_AD_Volts(1);
	EncoderReset(1);
	EncAtFinish=EncoderReadData8(1);
	if (EncAtFinish != 0)
	{
		ErrorStats = 1;
	}

}

void CalCatch(void)
{
	IRVoltageCatch=Get_AD_Volts(1);
	EncAtCatch=EncoderReadData8(1);
	EncoderReset(2);
}



int keyenter(int loc)
{
	int A;
	DIO_SendByte(loc);
	A = DIO_GetByte();
	A = A & 0x0F;
	MsDelay(70);
	switch(A)
	{
		case 0x0E:
			return 1;
			break;
		case 0x0D:
			return 2;
			break;
		case 0x0B:
			return 3;
			break;
		case 0x07:
			return 4;
			break;
		}
	return 0;
}



void SetVaribles(void)
{
	IntrCount = 0;
	MenuCurrNum=0;
	MenuRefresh=1;
	MenuSensor = 1;
	Power = 0.0;
	IR = 0.0;
	LastIR = 0.0;
	TimeReset();
	SPM = 0;
	SPMold = 0;
}

void SetMenu(void)
{           
	MLine1[0]="To start: have the  ";
	MLine2[0]="oar buried in the   ";
	MLine3[0]="water at the finish ";
	MLine4[0]="and press the * key ";

	MLine1[1]="Now, have the oar at";
	MLine2[1]="the catch buried and";
	MLine3[1]="press the * key     ";
	MLine4[1]="again               ";

	MLine1[2]="Virtual Coxswain    ";
	MLine2[2]="To recalibrate go to";
	MLine3[2]="the Controls menu   ";
	MLine4[2]="* - goto main menu  ";

	MLine1[3]="D - Communications  ";
	MLine2[3]="# - Controls        ";
	MLine3[3]="0 - Help            ";
	MLine4[3]="* - Show sensors    ";


	MLine1[4]="Communications Menu ";
	MLine2[4]="* - Main menu       ";
	MLine3[4]="D - Stop/Start sends";
	MLine4[4]="# - Check transfer  ";


	MLine1[5]="Wireless modem out  ";
	MLine2[5]="of range            ";
	MLine3[5]="                    ";
	MLine4[5]="            (*-Back)";


	MLine1[6]="Wireless modem in   ";
	MLine2[6]="range               ";
	MLine3[6]="                    ";
	MLine4[6]="            (*-Back)";


	MLine1[7]="Program is currently";
	MLine2[7]="Not sending data    ";
	MLine3[7]="                    ";
	MLine4[7]="            (*-Back)";


	MLine1[8]="Program is sending  ";
	MLine2[8]="data                ";
	MLine3[8]="                    ";
	MLine4[8]="            (*-Back)";


	MLine1[9]="Controls Menu       ";
	MLine2[9]="* - Main menu       ";
	MLine3[9]="D - Reset time      ";
	MLine4[9]="# - Recalibrate     ";


	MLine1[10]="Calibration Menu    ";
	MLine2[10]="D - At the finish   ";
	MLine3[10]="# - At the catch    ";
	MLine4[10]="* - Main menu       ";


	MLine1[11]="Help Menu           ";
	MLine2[11]="For the sensor menu ";
	MLine3[11]="0 - left   # - right";
	MLine4[11]="* - Main Menu       ";

	MLine1[12]="Sensor Menu (*-Back)";
	MLine2[12]="D - Power, SPM      ";
	MLine3[12]="# - Oar/Blade loc   ";
	MLine4[12]="0 - Test Mode       ";

}



void main(void)
{
	int MenuSelect;
	ES308_Init();
	LCD_Init();
	InitADC();
	SetVaribles();
	SetMenu();
	
	// Modem
	serBopen(19200);	// open serial port B at a baud rate of 19.2k
	ModemCheck();
	//

	// Timer A, A1
	// every 1 second - read sensors
	// every 60 seconds - calc avg for minute for stroke, power, tilt, enc
	// Timer A
	SetVectIntern(0x0A,ISR_R);
	WrPortI(TAT1R,NULL,255);
	WrPortI(TAT7R,NULL,255);
	WrPortI(TACR,&TACRShadow,0x82);
	BitWrPortI(TACSR,&TACSRShadow,1,7);
	RdPortI(TACSR);
	BitWrPortI(TACSR,&TACSRShadow,1,0);
	WrPortI(I0CR,&I0CRShadow,0x23);
	// End of Timer A





		

	while (1)
	{
		
		if (MenuRefresh)
		{
			LCD_Clear();
			if (MenuSensor == 1)
			{
				Line1();
				Display(MLine1[MenuCurrNum]);
				Line2();
				Display(MLine2[MenuCurrNum]);
				Line3();
				Display(MLine3[MenuCurrNum]);
				Line4();
				Display(MLine4[MenuCurrNum]);
			}
			else
			{
				SensorOutputMake(MenuSensor);
				Line1();
				Display(MLine1[13]);
				Line2();
				Display(MLine2[13]);
				Line3();
				Display(MLine3[13]);
				Line4();
				Display(MLine4[13]);
			}
			MenuRefresh=0;
		}
		MenuSelect = keyenter(0xE0);
		switch(MenuCurrNum)
		{
		case 0:
				if (MenuSelect == 4)
				{
				CalFinish;
				MenuCurrNum++;
				MenuRefresh=1;
				}
			break;
		case 1:
				if (MenuSelect == 4)
				{
				CalCatch;
				MenuCurrNum++;
				MenuRefresh=1;
				}
			break;
		case 2:
				if (MenuSelect == 4)
				{
				MenuCurrNum++;
				MenuRefresh=1;
				}
			break;
		case 3:		// Main Menu
			switch(MenuSelect)
				{
				case 1:
					MenuRefresh=1;
					MenuCurrNum=4;
					break;
				case 2:
					MenuRefresh=1;
					MenuCurrNum=9;
					break;
				case 3:
					MenuRefresh=1;
					MenuCurrNum = 11;
					break;
				case 4:
					MenuRefresh=1;
					MenuCurrNum = 12;
					break;
				}
			break;
		case 4:  // Comms Menu
			switch(MenuSelect)
				{
				case 1:
					MenuRefresh=1;
					if (ModemSendData)
						{
						ModemSendData = 0;
						MenuCurrNum = 7;
						}
					else
						{
						ModemSendData = 1;
						MenuCurrNum = 8;
						}
					break;
				case 2:
					MenuRefresh=1;
					ModemCheck();
					if (ModemInRange)
						{
						MenuCurrNum = 6;
						}
					else
						{
						MenuCurrNum = 5;
						}
					break;
				case 3:
					MenuRefresh=1;
					break;
				case 4:
					MenuCurrNum=3;
					MenuRefresh=1;
					break;
				}
			break;
		case 5:	// Wireless modem out of range
			if (MenuSelect == 4)
				{
				MenuCurrNum=3;
				MenuRefresh=1;
				}
			break;
		case 6:	// Wireless modem in range
			if (MenuSelect == 4)
				{
				MenuCurrNum=3;
				MenuRefresh=1;
				}
			break;
		case 7: // Program not currently sending data
			if (MenuSelect == 4)
				{
				MenuCurrNum=3;
				MenuRefresh=1;
				}
			break;
		case 8:	// Program is sending data
			if (MenuSelect == 4)
				{
				MenuCurrNum=3;
				MenuRefresh=1;
				}
			break;
		case 9: // Controls Menu
			switch(MenuSelect)
				{
				case 1:
					TimeReset();
					MenuRefresh=1;
					break;
				case 2:
					MenuCurrNum = 10;
					MenuRefresh=1;
					break;
				case 3:
					MenuRefresh=1;
					break;
				case 4:
					MenuCurrNum=3;
					MenuRefresh=1;
					break;
				}
			break;
		case 10:	//Calibration Menu
			switch(MenuSelect)
				{
				case 1:
					CalFinish();
					MenuRefresh=1;
					break;
				case 2:
					CalCatch();
					MenuRefresh=1;
					break;
				case 3:
					MenuRefresh=1;
					break;
				case 4:
					MenuCurrNum=3;
					MenuRefresh=1;
					break;
				}
			break;
		case 11: // Help Menu
			if (MenuSelect == 4)
				{
				MenuCurrNum=3;
				MenuRefresh=1;
				}
			break;
		case 12: // Sensor Menu
			switch(MenuSelect)
				{
				case 1:
					MenuRefresh=1;
					MenuCurrNum = 13;
					MenuSensor = 2;
					break;
				case 2:
					MenuRefresh=1;
					MenuCurrNum = 13;
					MenuSensor = 3;
					break;
				case 3:
					MenuRefresh=1;
					MenuCurrNum = 13;
					MenuSensor = 4;
					break;
				case 4:
					MenuCurrNum = 3;
					MenuSensor = 1;
					MenuRefresh = 1;
					break;
				}
			break;
		case 13:
			MenuRefresh=1; 
			switch(MenuSelect)
				{
				case 1:
					MenuCurrNum=13;
					MenuRefresh=1;
					break;
				case 2:
					MenuCurrNum=13;
					MenuRefresh=1;
					MenuSensor++;
					if (MenuSensor == 7)
						MenuSensor = 2;
					break;
				case 3:
					MenuCurrNum=13;
					MenuRefresh=1;
					MenuSensor--;
					if (MenuSensor == 1)
						MenuSensor = 6;
					break;
				case 4:
					MenuSensor = 1;
					MenuCurrNum=3;
					MenuRefresh=1;
					break;
				}
			break;
		}




	}
serBclose();
}
