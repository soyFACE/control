// tam0509 Edits 05082009 Changed Ozone settings to target instead of tracking and hours of 1000 - 1800
// tam0509 Modified from SOYFACE050307.c

//tam0520 Modified for no high O3 output shutdown.



#use "BLxS2xx.lib"
#use SUNRISE.lib            //Astronomical Calculations Library
#define NETWORK  				// Comment Out to not load Network Drivers


//#define FAT
//#define INITIALIZE_PARAMS  //  Run the first time to config settings - Comment out after that
#ifdef NETWORK
   //#define IPDOWNLOAD
	#define WIRELESS    //SELECT ONE OF THESE TWO OPTIONS BASED UPON BOARD MODEL
   //#define WIRED
#endif /* NETWORK */
#define EXTDA

#define	STDIO_DEBUG_SERIAL	SADR
#define	STDIO_DEBUG_BAUD	57600
#define	STDIO_DEBUG_ADDCR
#define Polled  0				// Set 0 to dump data per second/minute, 1 to wait for command
#define OZONE_MULT 1    	// Leave set to 1, now set through host computer
#define ADBRD 1  				// Slot # for Analog Input Board
#define DABRD 2      		// Slot # for Analog Output Board
#define RELAY 4   			// Slot # for Relay Output Board
#define DIGIO 6    			// Slot # for Dig IO Board
#define INP_Dew 4				// A/D Channel for Dewpt input
#define INP_WSpd 5   		// A/D Channel for Wind Speed Input
#define INP_WDir 6  			// A/D Channel for Wind Direction Input
#define INP_WDiff 7 		// A/D CHannel for Wind Ground (Differential Measure)
#define GRAB_TIME 4 			// Frequency for Data Transmit to Serial/IP (seconds)
#define OZONE_MAX 200 		// Maximum value Ozone can set to
#define delay_time 30		// Time Delay for Ozone Restart after low wind speed shut down  //Changed to 30 from 120 for testing
#define N_CHANNELS 1			// Number of Channels to Operate (Layers)
#define STARTTIME_OZONE 9
#define STOPTIME_OZONE 18
#define STARTTIME_CO2 8
#define STOPTIME_CO2 18

#define MATCHFLAG RN_MATCH_PRDID	//set flag to search for product ID
#define MATCHPID  RN1300			//match DAC board product ID

#memmap xmem
   //  *** MOST USER OPTIONS ABOVE THIS LINE  - Network Configure listed Below ***
   //  *** Also change calc_day routine to include correct lon, lat, and time zone for correct sunrise/set

#ifdef NETWORK
	#define TCPCONFIG 1
   #define _PRIMARY_STATIC_IP    "192.168.1."
	#define _PRIMARY_NETMASK      "255.255.255.0"
	#define MY_NAMESERVER         "192.168.1.1"
	#define MY_GATEWAY            "192.168.1.1"

#ifdef WIRELESS
//	#define WIFI_USE_WPA								// Bring in WPA_PSK support
//	#define WIFI_AES_ENABLED                  // Enable AES specific code
//	#define IFC_WIFI_ENCRYPTION   IFPARAM_WIFI_ENCR_CCMP  // Define encryption cypher suite
//	#define IFC_WIFI_SSID         "fieldtest"
   //NOTE for a new access point, the paraphrase must be entered teh first time, and then copy out the hex version to speed up boot time.
   //#define IFC_WIFI_WPA_PSK_PASSPHRASE "8nKz1fW45qEgt"
//	#define   IFC_WIFI_WPA_PSK_HEXSTR  "C2D0D23F369904FC238C2C2EE95BA4F8570C0C6857B706ABC35A184F2C556959"

//	#define WIFI_VERBOSE_PASSPHRASE

//	#define IFCONFIG_WIFI0 \
//	            IFS_IPADDR,aton(_PRIMARY_STATIC_IP), \
//	            IFS_NETMASK,aton(_PRIMARY_NETMASK), \
//	            IFS_UP

#define IFC_WIFI_SSID "soy1"
#define IFC_WIFI_ENCRYPTION IFPARAM_WIFI_ENCR_WEP
#define IFC_WIFI_WEP_KEYNUM 0
#define IFC_WIFI_WEP_KEY0_HEXSTR "101a08f9786683025d96eb04d3"


#endif /* WIRELESS */


   #define sethostid
 	#define UDP_SOCKETS 6	// allow enough for downloader and DHCP
	#define MAX_UDP_SOCKET_BUFFERS 6
	#define LOCAL_PORT   425
	#define REMOTE_IP       "255.255.255.255" /*broadcast*/
	#define  DEST       "192.168.1.155"
	#define  PORT     1001   //1001 for primary ozone, 1002 for secondary ozone
	#define  ETH_MTU      900
	#define  BUFF_SIZE    (ETH_MTU-40)  //must be smaller than (ETH_MTU - (IP Datagram + TCP Segment))
	#define  TCP_BUF_SIZE ((ETH_MTU-40)*4) // sets up (ETH_MTU-40)*2 bytes for Tx & Rx buffers
	#define MAX_TCP_SOCKET_BUFFERS 6
	#define ETH_MAXBUFS  12


#use "dcrtcp.lib"
#ifdef IPDOWNLOAD
	#define UDPDL_LOADER "Z:/Z-World/UDPDL/Loaders/pdl-generic-D.bin"
	#use "udpdownl.lib"
#endif /* IPDOWNLOAD */

	#define INCOMING_IP     0        //accept all connections
	#define INCOMING_PORT   0        //accept all ports
	#define TIME_OUT        15    // if in any state for more than a 15 seconds re-initialize

#endif /* NETWORK */

#memmap xmem
int i;
#ifdef FAT
#define FAT_USE_FORWARDSLASH
#define FAT_BLOCK
#use "fat16.lib"
#use "dcrtcp.lib"

FATfile my_file;     // When files are accessed, we need a FATfile structure.
int rc, uid, handle;
   long prealloc;
   				// Used if the file needs to be created.
	fat_part *first_part;
					// Use the first mounted FAT partition.

char message[250];  //string for serial data
int filemode;   // mode for writing
char datafile[15];
#web datafile
char ipbuf[16];
#web ipbuf
char ipaddrstr;
#web ipaddrstr
// This is a buffer for reading/writing the file.
char buf[128];
#endif /* FAT */
unsigned long ip;



#define DELAYTIME 50 //350



//---------------------------------------------------------
// Macro's
//---------------------------------------------------------
#define MAXDISPLAYROWS	4
#define LEDOFF				0
#define TOGGLE				1
#define INCREMENT			2
#define OPERATE			3

#define ASCII				0
#define NUMBER				1

#ifdef NETWORK
	tcp_Socket sock_A, sock_B, sock_C;
	int bytes_A, bytes_B, bytes_C;
	int state_A, state_B, state_C;
	int sent_A, sent_B, sent_C;
	int rcvd_A, rcvd_B, rcvd_C;
	long statetime_A, statetime_B, statetime_C;
	static char buff_A[BUFF_SIZE], buff_B[BUFF_SIZE], buff_C[BUFF_SIZE];
	word my_port_A, my_port_B, my_port_C;
   char str_A[50];
   char str_B[50];
   char str_C[50];
 	char* strscan;

   char DEST_IP[15];
#endif /* NETWORK */

long settings[13];
   void* save_data[1];
	unsigned int save_lens[1];

//----------------------------------------------------------
// Main_Menu options
//----------------------------------------------------------
const char *main_menu[] =
{		" <<<<Main Menu>>>>",
		"1)Return to Program",
		"2)Turn Ozone ON",
		"3)Turn Ozone OFF",
		"4)Calibrate Unit",
		"5)Set Date & Time",
		"6)Config Settings",
		"7) *** RESET *** ",
      "8) Set IP Address",
		"9) Set IP Ozone Addr",
      "10)Set Ozone Port",
		NULL
};
const char *treatments[] =
{		"CO2:",
		"O3 :",
      "OFF:",
		NULL
};
const char *anemometer[ ]=
{		"RMYoung12005",
		"Sonic",
      NULL
};

const char *boolean[ ]=
{		"False",
		"True",
      NULL
};
const char *display[ ]=
{		"Single",
		"Double",
      NULL
};


const char *token;
const char *delim = " ,:/";


//RabbitNet Variables for D/A card
	int device0, device1, portnum;
   rn_devstruct *devaddr;
	int status;
 	char tmpbuf[24];
   char recbyte;
   char sendbyte;
   int done, command;
   rn_search newdev;
   DacCal DacCalTable1;
   float voltout;
   int channel, chSelected, selectChannel;
   int key;


//----------------------------------------------------------
// Structures, arrays, variables
//----------------------------------------------------------



typedef struct  {
	int data;
	char *ptr;
} fieldupdate;

struct tm CurTime;     //structure for Time/Date
struct tm rtc;			  //Time and Date struct

char szTime[40];
char szString[20];
const char Days[] = {"SunMonTueWedThuFriSat"};
const char Months[] = {"JanFebMarAprMayJunJulAugSepOctNovDec"};


int UNIT_ID;
int nighttime_local;
int ledCntrl;
int beeperTick, timerTick ;
int max_menu_options;
int max_cmds_options;
unsigned long ulTime;
char *keybuffer;






#define EINBUFSIZE 127
#define EOUTBUFSIZE 127
#define FINBUFSIZE 127
#define FOUTBUFSIZE 127
#define MAX_SENTENCE 100


#define SETRAW 16				//offset to get raw analog data
#define INS 1
#define OUTS 0
#define BANK_A INS		//bank A inputs
#define BANK_B OUTS		//bank B outputs
#define TURNON 1    //REVERSED FOR BL4S200 051117 // JAM 2018-05-25 Set back to 1 to work with sourcing high current digital outputs
#define TURNOFF 0   //REVERSED FOR BL4S200 051117 // JAM 2018-05-25 Set back to 0 to work with sourcing high current digital outputs


#define DIM_S           10              //
#define DIM_L           45              //

#define PURGE_TIME	120

typedef unsigned char uchar;
typedef unsigned int  uint;
//typedef unsigned long ulong;

static unsigned long store;				// physical memory address to write to



//---------------------------------------------------------------------------
// DEFINIZIONE DELLE VARIABILI
//---------------------------------------------------------------------------
int aovReturn;
uchar wI;
uchar byIChA[N_CHANNELS+1];
uchar byIChB[N_CHANNELS+1];
uint outd;

uint scan;
uint RECORD;                            //
uint nCont01[N_CHANNELS+1];             //
uint nCont02[N_CHANNELS+1];             //
uint nCont03[N_CHANNELS+1];             //
uint nCont04[N_CHANNELS+1];             //
uint nContao[N_CHANNELS+1];             //
uint LAYER[N_CHANNELS+1];
uint LAYERRELAY[N_CHANNELS+1];
uint sec_count;
uint nSel;                              //
uint chn;
uint byICh;
uint nI;
uint cnt45;
uint cnt10;
uint bySect;
uint opSect[N_CHANNELS+1];
uint MOBILE;
uint MobDir[9];
uchar T1;                                //
uchar T2;                                //
uchar T3;                                //
uchar T4;                                //
uchar T5;                                //
uchar T6;                                //
uint V1;                                //
uint V2;                                //
uint V3;                                //
uint V4;                                //
uint V5;                                //
int sector;

char menudisp;
float fConc[N_CHANNELS+1];
float fDew;
float fTemp;
float fRh;
float fFpro0[N_CHANNELS+1] [DIM_L+1];     //
float fFdif0[N_CHANNELS+1] [DIM_L+1];     //
float fFwin[DIM_L+1];                   //
float fFspro[N_CHANNELS+1];             //
float fFsdif[N_CHANNELS+1];             //
float fFswin[N_CHANNELS+1];             //
float fF10win[DIM_L+1];                 //
float fF10[N_CHANNELS+1];               //
float fVp[N_CHANNELS+1];                //
float fV0[N_CHANNELS+1];                //
float fint0[N_CHANNELS+1];              //
float fVs[N_CHANNELS+1];					//
float fIrgaA;                           //
float fIrgaB;                           //
float fMedIrgaB;                        //
float fFwind;                           //
float fFlow;                            //
float MobSum;                           //
float MobCorr;                          //
float Direz;                            //
float fResA;                            //
float fResB;                            //
float Sumw;                             //
float Sumd;                             //
float f45;
float f10;
float s45;
float s10;

uint giorno;
uint mese;
uint ggiul;
float dayl;
float sunr;
float suns;
float ora;  //Current Decimal Time

float fwind;
float fval[N_CHANNELS+1];
float fmpro[N_CHANNELS+1];
float fmdif[N_CHANNELS+1];
float fmwin[N_CHANNELS+1];
float fm10w[N_CHANNELS+1];
float feint[N_CHANNELS+1];
float fepro[N_CHANNELS+1];
float fedif[N_CHANNELS+1];
float fcor[N_CHANNELS+1];
float fpps[N_CHANNELS+1];
float TARG[N_CHANNELS+1];
float nighttimeCO2;
float AD_GAIN[N_CHANNELS+1];
float AD_OFFSET[N_CHANNELS+1];
float DA_MULT[N_CHANNELS+1];
float Wind_Mult;
float Wind_Offset;

float FAINT[N_CHANNELS+1];
float FAPRO[N_CHANNELS+1];
float FADIF[N_CHANNELS+1];
float FCW[N_CHANNELS+1];
float ENDCONO;
float fRet;
float V_OUT_MIN;
float V_OUT_MAX;
                                                                   ;

float MB;
float fTxlm;
float fMedo;
float fVento;

float medo[N_CHANNELS+1];
float vento[N_CHANNELS+1];
float txlm[N_CHANNELS+1];

uint memPointer;

char ozonator_loc;
char ozonator_rem;
char CO2_loc;
char CO2_rem;
char ozone_connect;
char network_connect;
char flow;
char purge;
char ozone_power;

char delay_restart;  // Ozone Restart Timer
unsigned long delay_restart_time; // Ozone Restart Time






char inkey;
char nighttime_remote;
int purge_off;
char DA_Channel[8];
char locBuf[50];
char SENTENCE_E[MAX_SENTENCE];
char SENTENCE_F[MAX_SENTENCE];
char SENTENCE_IP[MAX_SENTENCE];
int input_char;
int string_pos_e;
int string_pos_f;
int string_pos_ip;
int backlight;
int timeOn;
const int timeDelay = 10;
int DISPLAY; //Set Display Options

int StatusCode;

// Sunrise Calculations
const double pi = 3.14159265358979;
double degs;
double rads;
double L,g,daylen;
double twam,altmax,noont,settm,riset,twpm;
const double AirRefr = 34.0/60.0; // athmospheric refraction degrees
// Sunrise Calculations



#ifdef NETWORK
udp_Socket sock;    //Added 121405
tcp_Socket socket;
   char ipbuff[20];
	char ipbuffd[10];
#endif /* NETWORK */
char message[250];
char s[9];


void calc_day(void);
void  fControl(void);
void readString_E(void);
void readString_F(void);
void readSerialE(void);
void readSerialF(void);
void calibrate(int sector);
void cont_loop(void);
void sector_select(void);
void wind_avg(void);
void minute_average(void);
void second_data(void);
void set_output(void);
void menu(void);
void keypress(void);
void SetDateTime(void);
void SetConfig(void);
void DispMenu(void);
void Reset_Ozone_Vars(void);
void readString_IP(void);
#ifdef FAT
void writefile(char *message, char *datafile, int writemode);
#endif /* FAT */

#ifdef NETWORK
int receive_packet_udp(void)
{
	auto int i;
	static char buf[128];

	#GLOBAL_INIT
	{
		memset(buf, 0, sizeof(buf));
	}

	/* receive the packet */
	if (-1 == udp_recv(&sock, buf, sizeof(buf))) {
		/* no packet read. return */
		return 0;
	}


	 for(i=0;i<sizeof(buf);i++)
	 	{
	 	input_char = buf[i];
	 	if(input_char == '\r')
			{
			printf("Received Return Mulitcast \n");
			SENTENCE_IP[string_pos_ip++] = '\0';
			readString_IP();
			}
		else if(input_char > 0)
			{
			SENTENCE_IP[string_pos_ip] = input_char;
			string_pos_ip++;
			//printf("%d \t %c \n",input_char,input_char);
			}
		}




   memset(buf, 0, sizeof(buf));

	return 1;
}

int receive_packet(tcp_Socket* sock, char *buff, int* bytes, int* rcvd, char* str)
{
	/* receive the packet */
	*bytes = sock_fastread(sock,buff,BUFF_SIZE);

	switch(*bytes)
	{
		case -1:
			return 4; // there was an error go to state 4 (NO_WAIT_CLOSE)
		case  0:
			return 2; // connection is okay, but no data received
		default:
         	(*rcvd)++;
         //*strcat(buff, *input);


         buff[*bytes] = '\0';
         strcat(str,buff);
         strscan = strrchr(str, '\r');
         if(strscan !='\0')
         {
         //printf("%s, %d \n", str, strlen(str));
         strcpy(SENTENCE_IP, str);
         readString_IP();
         str[0] = '\0';
          }



         return 3;	//now go to state 3 (SEND)
	}
}

int send_packet(tcp_Socket* sock, char* buff, int* bytes, int* sent)
{
   return 2;
   /* send the packet
	*bytes = sock_fastwrite(sock,buff,*bytes);
	switch(*bytes)
	{
		case -1:
			return 4; // there was an error go to state 4 (NO_WAIT_CLOSE)
		default:
			(*sent)++;
			return 2;	//now go to state 2 (RECEIVE)
	}                 */
}

MyHandle(tcp_Socket* sock, char* buff, int* bytes, int* state, word my_port,
			long* statetime, int *sent, int *rcvd, char* str)
{
	tcp_tick(sock);
	switch(*state)
	{
		case 0:/*INITIALIZATION*/										// listen for incoming connection
			tcp_listen(sock,my_port,INCOMING_IP,INCOMING_PORT,NULL,0);
			(*statetime) = SEC_TIMER+TIME_OUT;						// reset the statetime
			*sent = *rcvd = 0;											// reset num of packets send and rcvd
			(*state)++;														// init complete move onto next state
         printf("%02d:%02d:%02d, \t Port %d Initialized\n",rtc.tm_hour,rtc.tm_min,rtc.tm_sec, my_port);
         break;

		case 1:/*LISTEN*/
			if(sock_established(sock))									// check for a connection
				(*state)++;													//   we have connection so move on
			else if ((long)(SEC_TIMER-(*statetime)) > 0)			// if 1 sec and no sock
				*state = 4;													//	  abort and re-init

        break;
		case 2:/*RECEIVE*/
			*state = receive_packet(sock, buff, bytes, rcvd, str);	// see function for details
			if ((long)(SEC_TIMER-(*statetime)) > 0)					// if 1 sec and still waiting
				*state = 4;													//	  abort and re-init
			break;
		case 3:/*SEND*/
			(*statetime) = SEC_TIMER+TIME_OUT;						// reset the timer
			*state = send_packet(sock, buff, bytes, sent);		// see function for details

			break;
		case 4:/*NO WAIT_CLOSE*/
			sock_abort(sock);
         str[0] = '\0';												// close the socket
			*state = 0;														// go back to the INIT state
   		printf("%02d:%02d:%02d, \t Port %d Closed\n",rtc.tm_hour,rtc.tm_min,rtc.tm_sec, my_port);
   }
	tcp_tick(sock);
}
MyHandle_A(tcp_Socket* sock, char* buff, int* bytes, int* state, word my_port,
			long* statetime, int *sent, int *rcvd, char* str)
{
	longword  destIP;
	tcp_tick(sock);
	switch(*state)
	{
		case 0:/*INITIALIZATION*/										// listen for incoming connection
           if( 0L == (destIP = resolve(DEST_IP)) ) {
		printf( "ERROR: Cannot resolve \"%s\" into an IP address\n", DEST_IP );
		exit(2);
   	   }
	      tcp_open(sock,0,destIP,settings[12],NULL);

          printf("%02d:%02d:%02d, \t Port %d Initialized\n",rtc.tm_hour,rtc.tm_min,rtc.tm_sec, settings[12]);
        //while(!sock_established(&socket) && sock_bytesready(&socket)==-1) {
	      //   tcp_tick(NULL);
         //   if ((long)(SEC_TIMER-(*statetime)) > 0){			// if 1 sec and no sock
			 //	*state = 4;													//	  abort and re-init
          //  break;
          //  }
	      //}

         (*statetime) = SEC_TIMER+TIME_OUT;						// reset the statetime
			*sent = *rcvd = 0;											// reset num of packets send and rcvd
			(*state)++;														// init complete move onto next state
         break;
		case 1:/*LISTEN*/
			if(sock_established(sock)){									// check for a connection
				(*state)++;
            //ledOut(4,1);
            ozone_connect = 1;
            }													//   we have connection so move on
			else if ((long)(SEC_TIMER-(*statetime)) > 0)			// if 1 sec and no sock
				*state = 4;													//	  abort and re-init

        break;
		case 2:/*RECEIVE*/
			*state = receive_packet(sock, buff, bytes, rcvd, str);	// see function for details
			if ((long)(SEC_TIMER-(*statetime)) > 0)					// if 1 sec and still waiting
				*state = 4;													//	  abort and re-init
			break;
		case 3:/*SEND*/
			(*statetime) = SEC_TIMER+TIME_OUT;						// reset the timer
			*state = send_packet(sock, buff, bytes, sent);		// see function for details

			break;
		case 4:/*NO WAIT_CLOSE*/
			sock_abort(sock);
         str[0] = '\0';												// close the socket
			*state = 0;
         //ledOut(4,0);													// go back to the INIT state
         ozone_connect = 0;
         printf("%02d:%02d:%02d, \t Port %d Closed\n",rtc.tm_hour,rtc.tm_min,rtc.tm_sec, PORT);

   }
	tcp_tick(sock);
}

PrintIt(int x, int y, int bytes, int state, word port, long statetime, int rcvd, int sent)
{
	printf ("\x1B=%c%c bytes_A: %05d  state_A: %03d",x,y, bytes, state);
	printf (" my_port_A: %05d  statetime_A: %09ld",port, statetime);
	printf ("\x1B=%c%c Rcvd: %06d  Sent: %06d", x+2, y+1, rcvd, sent);

}

#endif /* NETWORK */
#ifdef FAT
void WriteLogFile()
{

sprintf(message,"Controller Restarted %02d/%02d/%04d %02d:%02d:%02d \r\n" , rtc.tm_mon,rtc.tm_mday,(1900 + rtc.tm_year),rtc.tm_hour,rtc.tm_min,rtc.tm_sec);
sprintf(datafile,"LOGFILE.TXT");
writefile(message, datafile,1);
}
#endif /* FAT */
main()
{
  //removed auto int device0, temp
 	auto char tmpbuf[24];
   auto char recbyte;
   auto int done, command;
   auto rn_search newdev;

   auto float voltout;
   auto int channel, chSelected, selectChannel;
   auto int key;
   auto char buf[128];


   int inputnum;
   int outputnum;

#ifdef NETWORK
   longword  destIP;
#endif /* NETWORK */

   save_data[0] = &settings;
	save_lens[0] = sizeof(settings);
   #ifdef INITIALIZE_PARAMS
   settings[0] = 7;     //Ring ID
   settings[1] = 0;     //NightTime CO2
   settings[2] = 0;      //Layer 0 type
   settings[3] = 600;    //Layer 0 Setpoint
	settings[4] = 0;      //Layer 0 D/A Channel
	settings[5] = 2;      //Layer 1 type
   settings[6] = 0;    //Layer 1 Setpoint
   settings[7] = 1;      //Layer 1 D/A Channel
   settings[8] = 0;      //Anemometer Type - 0=RMYoung 1= Sonic
   settings[9] = 1;      //Display Type
   settings[10] = 223;   //Unit ID IP Address 200
	settings[11] = 152;   //Ozone Controller IP Address 225
   settings[12] = 1001;   //Ozone Controller PORT (1001 or 1002)


   save_data[0] = &settings;
	save_lens[0] = sizeof(settings);
   writeUserBlockArray(0, save_data, save_lens, 1);
#endif /* INITIALIZE_PARAMS */

     //Read UserBlock Variables 3/4/04
  	readUserBlockArray(save_data, save_lens, 1, 0);

   printf("settings.[0] = %ld\n", settings[0]);
	printf("settings.[1] = %ld\n", settings[1]);
   printf("settings.[2] = %ld\n", settings[2]);
   printf("settings.[3] = %ld\n", settings[3]);
	printf("settings.[4] = %ld\n", settings[4]);
   printf("settings.[5] = %ld\n", settings[5]);
   printf("settings.[6] = %ld\n", settings[6]);
   printf("settings.[7] = %ld\n", settings[7]);
   printf("settings.[8] = %ld\n", settings[8]);
   printf("settings.[9] = %ld\n", settings[9]);
	printf("settings.[10] = %ld\n", settings[10]);
	printf("settings.[11] = %ld\n", settings[11]);
   printf("settings.[11] = %ld\n", settings[12]);

	//store = xalloc(65536);		// physical memory address (SRAM)


  DEST_IP[0] = '\0';
  strcat(DEST_IP,"192.168.1." );
  ltoa(settings[11],ipbuff);
  strcat(DEST_IP,ipbuff );

  ltoa(settings[10],ipbuffd);
   strcat(_PRIMARY_STATIC_IP,ipbuffd );
#ifdef WIRELESS
   ifconfig(IFCONFIG_WIFI0, IFS_DOWN,
   			IFS_IPADDR, aton(_PRIMARY_STATIC_IP),
            IFS_NETMASK,aton(_PRIMARY_NETMASK),
            IFS_ROUTER_SET, aton(MY_GATEWAY),
            IFS_UP,
    			IFS_END);          //WIFI0
#endif /* WIRELESS */
#ifdef WIRED
   ifconfig(IFCONFIG_ETH0, IFS_DOWN,
   			IFS_IPADDR, aton(_PRIMARY_STATIC_IP),
            IFS_NETMASK,aton(_PRIMARY_NETMASK),
            IFS_ROUTER_SET, aton(MY_GATEWAY),
            IFS_UP,
    			IFS_END);          //WIFI0
#endif /* WIRED */

    		printf("IP Address Entered = %s\r\n",_PRIMARY_STATIC_IP);
         printf("IP Ozone   Entered = %s\r\n", DEST_IP);

         //ltoa(settings[11],ipbuff);
  	 		//strcat(ipbuffd, DEST );
         //strcat(ipbuffd, ipbuff);
   		//printf("IP Ozone Controller Entered = %s\r\n", ipbuff);




#ifdef NETWORK
   printf( "initializing sock..\n" );
   brdInit();
   rn_init(RN_PORTS, 1);      //initialize controller RN ports
   sock_init();
      //sock_init_or_exit(1);
	// Wait for the interface to come up
	//while (ifpending(IF_DEFAULT) == IF_COMING_UP) {
	//	tcp_tick(NULL);
	//}

   ip_print_ifs();
   if(!udp_open(&sock, LOCAL_PORT, resolve(REMOTE_IP), 0, NULL)) {
		printf("udp_open failed!\n");
		exit(0);
      }

#endif /* NETWORK */
    //WriteLogFile();


#ifdef IPDOWNLOAD
   UDPDL_Init("Ring");  //Initialize Network Download
#endif

anaOutConfig(0,0);

#ifdef EXTDA
   //search for device match
   //search on ports using physical node address
 	portnum=0000;
	if ((device0 = rn_device(portnum)) == NOCONNECT)
	{
   	printf("\n\n*** No device found on port %d\n\n", 0);
   }
   else
   {
		// device0 is actually an address, cast it as such.
   	devaddr = (rn_devstruct *)device0;
   	printf("\n\n*** Device found on port 0\n");
      printf("- Product ID 0x%04x\n", devaddr->productid);
      printf("- Serial number 0x%02x%02x%02x%02x\n",
      	    devaddr->signature[0], devaddr->signature[1],
             devaddr->signature[2], devaddr->signature[3]);
   }

   portnum=0100;
	if ((device1 = rn_device(portnum)) == NOCONNECT)
	{
   	printf("\n\n*** No device found on port 1\n\n");
   }
   else
   {
		// device1 is actually an address, cast it as such.
   	devaddr = (rn_devstruct *)device1;
   	printf("\n\n*** Device found on port 1\n");
      printf("- Product ID 0x%04x\n", devaddr->productid);
      printf("- Serial number 0x%02x%02x%02x%02x\n",
      	    devaddr->signature[0], devaddr->signature[1],
             devaddr->signature[2], devaddr->signature[3]);
   }


   if(device0 == NOCONNECT && device1 == NOCONNECT)
   {
		printf("\nNo board connections!\n");
   	//exit(-ETIMEDOUT);   Commented out 7/17/17
   }
   rn_write(device0, 0, &sendbyte, 1);
   rn_anaOutConfig(device0, 2, 0, 0);
   for(channel=0; channel < 8; channel++)
      {
      	rn_anaOutRdCalib(device0, channel, &DacCalTable1, 0);
         rn_anaOutVolts(device0, channel,  0, &DacCalTable1, 0);
      }
#endif /* EXTDA */



      #ifdef FAT
   rc = fat_AutoMount(FDDF_USE_DEFAULT);
 first_part = NULL;
	for (i = 0; i < num_fat_devices * FAT_MAX_PARTITIONS; ++i) {
		if ((first_part = fat_part_mounted[i]) != NULL) {
			// found a mounted partition, so use it
			break;
		}
	}
	// Check if a mounted partition was found
	if (first_part == NULL) {
		// No mounted partition found, ensure rc is set to a FAT error code.
		rc = (rc < 0) ? rc : -ENOPART;
	} else {
		// It is possible that a non-fatal error was encountered and reported,
		// even though fat_AutoMount() succeeded in mounting at least one
		// FAT partition.
		printf("fat_AutoMount() succeeded with return code %d.\n", rc);
		// We found a partition to work with, so ignore other error (if any).
		rc = 0;
	}
   // FAT return codes always follow the convention that a negative value
   // indicates an error.
	if (rc < 0) {
   	if (rc == -EUNFORMAT)
      	printf("Device not Formatted, Please run Fmt_Device.c\n");
      else
	   	printf("fat_AutoMount() failed with return code %d.\n", rc);
      exit(1);
   }
#endif /* FAT */


	StatusCode = 0;
  	ozonator_loc = 1;
  	ozonator_rem = 0;
   CO2_loc = 1;
   CO2_rem = 1;
	flow =0;     //Must be 1 for ozonator to come on.
   ozone_connect=0;
   purge = 0;
   ozone_power = 0;
   nighttime_remote = 1;



	serEopen(19200);
	serFopen(115200);
	serMode(0);
	memPointer = 0;

   //Transmit current settings to display
    sprintf(message,"Z,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld, \r",settings[0],settings[1],settings[2],settings[3],settings[4],settings[5],settings[6],settings[7],settings[8],settings[9],settings[10],settings[11],settings[12]);
         serEputs(message);
   		while (serEwrFree() != EOUTBUFSIZE) ;

   //Transmit current datetime to display
       tm_rd(&rtc);
       sprintf(message,"T,%02d/%02d/%04d,%02d:%02d:00,\r",rtc.tm_mon,rtc.tm_mday,(1900 + rtc.tm_year),rtc.tm_hour,rtc.tm_min,rtc.tm_sec);
			serEputs(message);
   		while (serEwrFree() != EOUTBUFSIZE) ;

	     //----------------------------------------------------------------------
     // Initialize the variables
     //----------------------------------------------------------------------




   nSel=0;
   chn=0;
   V_OUT_MAX=10; //Maximum output voltage at pressure regulator  *** MOVED TO WIND DIR CHANGE
	V_OUT_MIN=0;  //Minimum output voltage at pressure regulator

      // Configure all outputs to be general digital outputs that are high
	for(channel = 0; channel < 16; ++channel)
	{
   	// Set output to be general digital output
		setDigOut(channel, 1);
		digOut(channel,1);   //REVERSED FOR BL4S200 051117
   }
   digOutConfig_H(0xFF);

   for(channel = 0; channel < 8; ++channel)
	{
   	// Set output to be general digital output
		digOut_H(channel,TURNOFF);   //REVERSED FOR BL4S200 051117
   }


   for(i = 0;i < N_CHANNELS+1;i++)
   	{
   	byIChA[i]=1;
		byIChB[i]=1;
		feint[i]=0;
		fepro[i]=0;
		fedif[i]=0;
      opSect[i]=1;
      medo[i] = 0;
		vento[i] = 0;
		txlm[i] = 0;
		nContao[i] = 0;

		}
	s45=0;
	f45=0;
	cnt45=0;
	s10=0;
	f10=0;
	cnt10=0;
	outd = 0;
   MOBILE=60;    //60;    //Running average for directional control
	MB=10;        //Running average for wind speed
	ENDCONO=60;	//60
	RECORD=0;

	string_pos_e = 0;
	string_pos_f = 0;
   string_pos_ip = 0;
	SENTENCE_E[0] = '\0';
	SENTENCE_F[0] = '\0';
   SENTENCE_IP[0] = '\0';
	sec_count = 0;
	fDew = 0;
   for(wI=1;wI<=8;wI++) MobDir[wI]=0;

   delay_restart = FALSE;



   UNIT_ID = (int)settings[0];
   nighttime_local = (int)settings[1];
   LAYER[0] = (int)settings[2];
   TARG[0] = (long)settings[3];
    //tam0509  if (LAYER[0] == 2) {TARG[0] = 0;}

   DA_Channel[0] = (int)settings[4];
   LAYER[1] = (int)settings[5];
   TARG[1] = (long)settings[6];
   //tam0509  if (LAYER[1] == 2) {TARG[1] = 0;}
   DA_Channel[1] = (int)settings[7];
   if (!settings[8])
   {
   Wind_Mult = 12.5;
   Wind_Offset = 0;
   }
   else
   {
   Wind_Mult = 10;
   Wind_Offset = 0;  //Changed 6/9/04 - Corn Rings from 2.2 to 0.0

   }
   DISPLAY = (int)settings[9];

   for (i=0;i<N_CHANNELS+1 ;i++)
   {

   if (LAYER[i] == 2)
   {
   FAINT[i]=0;   //
      FAPRO[i]=0;     //
      FADIF[i]=0;      //
      FCW[i]=0;          //
      AD_GAIN[i] = 0;
      AD_OFFSET[i] = 0;
      DA_MULT[i] = 0;
   }
   else if (LAYER[i] == 1)
   {  //Layertype 1 - Ozone Settings
      FAINT[i]=-0.00008;   //Initial=0.00002  6/24  -0.00016,-0.0008  6/25 -0.00006  8/9 -0.00008
      FAPRO[i]=-0.0016;     //Initial=0.0008 6/24  -0.0032,-0.0016
      FADIF[i]=-0.016;      //Initial=0.0080
      FCW[i]=0.5;          //Initial=0.2
   	AD_GAIN[i] = 50;
      AD_OFFSET[i] = 0;
      DA_MULT[i] = .5;
   }
   else if (LAYER[i] == 0)
   {   //Layertype 0 - CO2 Settings
   	FAINT[i]=-0.000015;   //
      FAPRO[i]=-0.0008;     //
      FADIF[i]=-0.016;      //
      FCW[i]=0.15;          //
      AD_GAIN[i] = 400;
      AD_OFFSET[i] = 0;
      DA_MULT[i] = 1;
   }

   }
   LAYERRELAY[0] = 6;  //Define which relay board controls each layer
   LAYERRELAY[1] = 5;

   nighttimeCO2 = 360;




   fFwind=((float)anaInVolts(INP_WSpd,2) - (float)anaInVolts(INP_WDiff,2))*Wind_Mult + Wind_Offset;


	if(fFwind<0)
   	  {
	   fFwind=0.0;
   	  }



      for(nI=0; nI<N_CHANNELS+1;nI++)    // Inizializza  contatori
   	{

		if(LAYER[nI] == 0)
			{
			fV0[nI]=V_OUT_MIN+(float)(fFwind*FCW[nI]);
			}
		else if(LAYER[nI] == 1)
			{
   		fV0[nI]=0;
			}
      else if(LAYER[nI] == 2)//OFF
			{
   		fV0[nI]=0;
			}

      fFspro[nI]=0;
      fFsdif[nI]=0;
      fFswin[nI]=0;
      fint0[nI]=0;
	   nCont01[nI]=1;
	   nCont02[nI]=1;
	   nCont03[nI]=1;
	   nCont04[nI]=1;
	   nContao[nI]=0;
	   fVp[nI]=fV0[nI];
  	   if (fVp[nI]>V_OUT_MAX)
         {
	      fVp[nI]=V_OUT_MAX;
         }
		for(wI=1; wI<=DIM_L;wI++)    // Inizializza  contatori

			{
			fFpro0[nI] [wI]=0;
			fFdif0[nI] [wI]=0;
			fFwin[wI]=0;
			fF10win[wI]=0;
         }
   	 }
	menudisp=0;
	timeOn = (int)SEC_TIMER;
   calc_day(); //Initial Sunrise Calculation


cont_loop();

}

void cont_loop()
{

	int memo_second;
   int sec_now;


   int Ip;
   float sba1;
   float sba2;

 #ifdef NETWORK
	my_port_A = 888;
	my_port_B = 999;
   my_port_C = 777;
	state_A = state_B = state_C = 0;
	sent_A = sent_B = sent_C = 0;
	rcvd_A = rcvd_B = rcvd_C = 0;
   str_A[0]= str_B[0]=str_C[0]= '\0';
#endif


//----------------------------------------------------------------------
//************************   MAIN LOOP    ******************************
//----------------------------------------------------------------------


	while(1)
	{
   tcp_tick(sock);
   	costate
	{
      waitfor(DelayMs(10));




	}


	costate
	{
		readSerialE();
		readSerialF();
#ifdef NETWORK
      MyHandle(&sock_A, buff_A, &bytes_A, &state_A, my_port_A, &statetime_A, &sent_A, &rcvd_A, str_A);
	   //PrintIt(0x20, 0x20, bytes_A, state_A, my_port_A, statetime_A, rcvd_A, sent_A);
     	MyHandle_A(&sock_B, buff_B, &bytes_B, &state_B, my_port_B, &statetime_B, &sent_B, &rcvd_B, str_B);
	   //PrintIt(0x20, 0x24, bytes_B, state_B, my_port_B, statetime_B, rcvd_B, sent_B);
      receive_packet_udp();
#endif
#ifdef IPDOWNLOAD
      if (UDPDL_Tick()) // this should be called at least twice a second.
		{
			printf("Download request pending!\n");
			// if you need to shut things down before the download, do it here.
		}
#endif

	}
	costate
	{

		if(timeOn + timeDelay < (int)SEC_TIMER)
		{
		backlight = 0;
		//glBackLight(backlight);
		//printf("%d \n",backlight);

		};

		if (memo_second != (int)SEC_TIMER)
			{

         //TOBEFIXED anaOutVolts(2,3);  // Provide power to leaf wetness sensor
			//if(ozonator_loc || (ozonator_loc && ozonator_rem)) {ledOut(0,1);} else {ledOut(0,0);}
 		  	//tam0509  if ((ora>sunr && ora<suns)  && ozonator_rem ) {ledOut(1,1);} else {ledOut(1,0);}
        	//if ((ora>10 && ora<18)  && ozonator_rem ) {ledOut(1,1);} else {ledOut(1,0);}

         //ledOut(2,(flow|purge));
			//ledOut(3,1); //Flash watch light
         memo_second = (int)SEC_TIMER;

			wind_avg();
			sector_select();
         fControl();
         set_output();
         second_data();
         if (nContao[0] == ENDCONO)
         	{
         	fDew = (float)anaInVolts(INP_Dew,2); // Read Dew Point
         	minute_average();  //calculate minute averages

				}
			//if(ozonator_loc && !ozonator_rem) ledOut(0,0);
         //ledOut(3,0);

         //anaOutVolts(ChanAddr(DABRD, 7), 0);
        	}
      hitwd();
	}
   }


}

void sector_select(void)
{

        int i;
        fFlow=360./5.*(float)anaInVolts(INP_WDir,2);    //read wind direction
        printf("Wind: %.2f %.2f %.2f\n", fFlow, (float)anaInVolts(INP_WDir,2),(float)anaInVolts(INP_WDir,2));
        //fFlow = 88;       //TESTING ONLY

        Direz=fFlow;
        bySect=(fFlow-22.5)/45+2;
        if(bySect>8) bySect=1;
        MobDir[bySect]++;
        nSel++;

        if (purge)
				{
					if(purge_off <= (int)SEC_TIMER)
					{
						digOut(1, TURNOFF);
						digOut(2, TURNOFF);
						purge = 0;
					}
				}


for(chn=0;chn < N_CHANNELS;chn++)
{

if(LAYER[chn]==0)  // CO2 CONTROL LAYER
	{
	if (((ora >= STARTTIME_CO2 && ora < STOPTIME_CO2) || (nighttime_remote && nighttime_local)) && CO2_loc && CO2_rem)   // Esegue le operazioni solo di giorno 18.08.2000
	{
        if (nSel>=MOBILE && s45>0.5)
        {
           MobSum=-1000;
           for(wI=1;wI<=8;wI++)
           {
              if (MobDir[wI]>MobSum)
              {
                 MobSum=MobDir[wI];
                 //MobDir[wI]=0;
                 bySect=wI;
              }                                 //close if
           }                                    //close for
           if(bySect>8 | bySect<1) bySect=1;

           //if(bySect != byIChB[chn]) digOut(byIChB[chn]-1,TURNOFF);
           //printf("%d /n",((chn*8)+byIChB[chn]-1));
           if(bySect != byIChB[chn]) digOut_H(((chn)+byIChB[chn]-1),TURNOFF);
           digOut_H(((chn) + bySect-1), TURNON);   //Corrected

           printf("bySect: %01d\n", bySect);
           byIChB[chn]=bySect;
           opSect[chn]=bySect;
           //nSel=0;

        }                                       //close if

         if  (nSel>=MOBILE && s45<0.5)
         {
           for(wI=1;wI<=8;wI++)
           {
            digOut_H(((chn) + wI)-1,TURNOFF);
	    		//MobDir[wI]=0;
           }
           byIChA[chn]++;
           if(byIChA[chn]>8) byIChA[chn]=1;
           digOut_H(((chn)+byIChA[chn])-1, TURNON);

           opSect[chn]=byIChA[chn];
           byIChB[chn]=byIChA[chn];
           //nSel=0;
           if (fVp[chn] > 1)
				{
         	fVp[chn]=1;
         	}
         }
	}                                        //close if
	else
	{

			for(wI=1;wI<=8;wI++)   //Close all sectors
         	{
         	digOut_H(((chn) +wI)-1, TURNOFF);
       		}
	}
	}		//END LAYER SELECT == 0



if(LAYER[chn]==1)  // OZONE CONTROL LAYER
{

//tam0509 if ((ora>sunr && ora<suns)  && ozonator_rem )
if ((ora >= STARTTIME_OZONE && ora < STOPTIME_OZONE)  && ozonator_rem )
   {
   digOut(0, TURNON);  //CHANGE2003
   ozone_power=1;
	}
   else
   {
   digOut(0, TURNOFF);
   ozone_power=0;
	}

if (delay_restart)   //  Check for timeout if in ozone delay restart
	{
   if (SEC_TIMER > delay_restart_time) delay_restart = FALSE;
   //printf("%ld %ld \n" ,delay_restart_time, SEC_TIMER);

   }


//tam0509 if ((ora>sunr && ora<suns) && ozonator_loc && ozonator_rem && !purge && !delay_restart)   // Esegue le operazioni solo di giorno 18.08.2000
if ((ora >= STARTTIME_OZONE && ora < STOPTIME_OZONE) && ozonator_loc && ozonator_rem && !purge && !delay_restart)   // Esegue le operazioni solo di giorno 18.08.2000
	{
        if (nSel>=MOBILE && s45>0.5)
        {

        digOut(1, TURNON); //Turn on AC flow
           MobSum=-1000;
           for(wI=1;wI<=8;wI++)
           {
           	//printf("%f\n",MobSum);
              if (MobDir[wI]>MobSum)
              {
                 MobSum=MobDir[wI];
                 //MobDir[wI]=((chn*8)+;
                 bySect=wI;
              }                                 //close if
           }                                    //close for
           if(bySect>8) bySect=1;
           if(bySect != byIChB[chn]) digOut_H(((chn)+byIChB[chn]-1), TURNOFF);
           digOut_H(((chn)+bySect-1), TURNON);
           byIChB[chn]=bySect;
           opSect[chn]=bySect;
           //nSel=0;
           flow =1;
          if (ozonator_rem) digOut(0, TURNON);  //CHANGE2003
				else digOut(0, TURNOFF);

        }                                       //close if

         if  (nSel>=MOBILE && s45<0.5)
         {
         delay_restart = TRUE;
         delay_restart_time = SEC_TIMER + delay_time;

				if (ozonator_rem) digOut(0, TURNON);  //CHANGE2003
				else digOut(0, TURNOFF);

				if(flow)
				{
					purge = 1;
					flow = 0;
					purge_off = (int)SEC_TIMER + PURGE_TIME;
				}

       	}




	}                                        //close if
	else
	{
		digOut(0, TURNOFF);  //Turn off Ozone Generator
      if  (s45<0.5)
      	 {
          delay_restart = TRUE;
          delay_restart_time = SEC_TIMER + delay_time;
          }

      if(flow)
			{
			purge = 1;
			flow = 0;
			purge_off = (int)SEC_TIMER + PURGE_TIME;
			}



	}

}		//END LAYER SELECT == 1

if(LAYER[chn]== 2 )  // LAYER OFF
{
	for(wI=1;wI<=8;wI++)   //Close all sectors
         	{
         	digOut_H(wI-1,TURNOFF);
       		}
}



}		//END CHN LOOP
if (nSel>=MOBILE)
	{
	 nSel = 0;
	 for(wI=1;wI<=8;wI++) MobDir[wI]=0;
	}
}		//END SUB SECTOR SELECT

/************************************************************/
/*                    average wind speed                    */
/************************************************************/


void wind_avg(void)
{
   //fFwind=(((((float)anaInVolts(ChanAddr(ADBRD, INP_WSpd)))-
   //  	((float)anaInVolts(ChanAddr(ADBRD, INP_WDiff)))))* Wind_Mult)-Wind_Offset; //WSpd Diff Input
   fFwind=((float)anaInVolts(INP_WSpd,2) - (float)anaInVolts(INP_WDiff,2))*Wind_Mult + Wind_Offset; //WSpd Diff Input
   //fFwind = 3;
   //fFwind = 5; //TEST
	if(fFwind<0)
 	  {
     fFwind=0.0;
  	  }
   f45=fFwind/45.;
	f10=fFwind/10.;

	if (cnt45<45)
           {
		    s45=s45+f45;
	    cnt45++;
           }
   else
           {
	    s45=s45-s45/45.+f45;
           }
	if (cnt10<10)
           {
	    s10=s10+f10;
	    cnt10++;
           }
	else
           {
	    s10=s10-s10/10.+f10;
           }
	fwind=fFwind;

}








/************************************************************/
/*                    fControl                              */
/************************************************************/
void fControl(void)
{
	for(chn=0;chn < N_CHANNELS+1;chn++)
	{

	//   read analog input *Differential Input Change
	fResA=0;
	for (wI=1;wI<=100;wI++)
   	{
      fIrgaA=(float)anaInVolts(chn*2,2);
      fIrgaB =(float)anaInVolts(chn*2+1,2);
      fResA=fResA+(((fIrgaA-fIrgaB)* AD_GAIN[chn])+ AD_OFFSET[chn]);
      //printf("%.2f %.2f %.2f %.2f \n",fIrgaA, fIrgaB, fIrgaA-fIrgaB, fResA);
      }
      //printf("break \n");
      fpps[chn]=fResA/100;
      //printf("%.2f \n",fpps[chn]);
      printf("fIrgaA: %.2f, fIrgaB: %.2f \n",fIrgaA,fIrgaB);


	nContao[chn]++;
   if(LAYER[chn]==0)  // CO2 CONTROL LAYER
   {
   if ((ora <= STARTTIME_CO2 || ora > STOPTIME_CO2) && (nighttime_remote && nighttime_local))
   	{
      TARG[chn]=  nighttimeCO2;
      }
   else
   	{
      if (chn == 0)
      {
      TARG[chn] = settings[3];
      }
      if (chn == 1)
      {
      TARG[chn] = settings[6];
      }

      }
   }
      fval[chn] = fpps[chn] - TARG[chn];

	if(LAYER[chn]==0)  // CO2 CONTROL LAYER
	{
   	//if (ora<sunr | ora>suns | !nighttime_remote | !nighttime_local | !CO2_rem | !CO2_loc)   // If nighttime_remote
      if(((ora <= STARTTIME_CO2 || ora > STOPTIME_CO2) && !(nighttime_remote && nighttime_local))|| !CO2_rem || !CO2_loc )
      {
	  	fVs[chn]=0;
      }
      else // Daytime
      {
         if (nCont01[chn] <= DIM_L) //Delay for first 45 seconds
	{

      fFpro0[chn][nCont01[chn]] = fval[chn];
	   if (nCont01[chn] == 1)
	 	{
		   fFdif0[chn][nCont01[chn]] = fFpro0[chn][nCont01[chn] - 0];
		}
	   if (nCont01[chn] > 1)
		{
	   	fFdif0[chn][nCont01[chn]] = fFpro0[chn][nCont01[chn]] - fFpro0[chn][nCont01[chn] - 1];
		}

	   fFspro[chn] = fFspro[chn] + fFpro0[chn][nCont01[chn]];
	   fFsdif[chn] = fFsdif[chn] + fFdif0[chn][nCont01[chn]];

	   if ((fVp[chn] > V_OUT_MIN) && (fVp[chn] < V_OUT_MAX))
		{
	   	fint0[chn] = fint0[chn] + FAINT[chn] * fval[chn];
		}
	   if ((fVp[chn] == V_OUT_MIN) && (fval < 0))    //tried remove ==
		{
		   fint0[chn] = fint0[chn] + FAINT[chn] * fval[chn];
		}
	   if ((fVp[chn] == V_OUT_MAX) && (fval > 0))
		{
			fint0[chn] = fint0[chn] + FAINT[chn] * fval[chn];
	   }

	   nCont01[chn]++;
	   fVs[chn]=fVp[chn];
	}
else
{
	   	fFspro[chn] = fFspro[chn] - fFpro0[chn][ nCont02[chn]];
	   	fFsdif[chn] = fFsdif[chn] - fFdif0[chn][ nCont02[chn]];
	   	fFpro0[chn][ nCont02[chn]] = fval[chn];

		   if (nCont02[chn] == 1)
		  	{
         	fFdif0[chn][nCont02[chn]] = fFpro0[chn][nCont02[chn]] - fFpro0[chn][ (DIM_L-1)];
		   }

		   if (nCont02[chn] > 1)
		   {
         	fFdif0[chn][nCont02[chn]] = fFpro0[chn][nCont02[chn]] - fFpro0[chn][ nCont02[chn] - 1];
			}

         fFspro[chn] = fFspro[chn] + fFpro0[chn][nCont02[chn]];
         fFsdif[chn] = fFsdif[chn] + fFdif0[chn][nCont02[chn]];
		   nCont02[chn]++;
		   nCont04[chn]++;

		   if (nCont02[chn] > DIM_L)
			{
				nCont02[chn] = 1;
			}

		   if (nCont04[chn] > MB)
			{
				nCont04[chn] = 1;
			}

		   fmpro[chn] = fFspro[chn] / DIM_L;
		   fmdif[chn] = fFsdif[chn] / DIM_L;
		   fmwin[chn] = s45;
		   fm10w[chn] = s10;

		   if ((fVp[chn] > V_OUT_MIN) && (fVp[chn] <V_OUT_MAX))
			{
				fint0[chn] = (float)(fint0[chn] + FAINT[chn] * fval[chn]);
			}

		   if ((fVp[chn] == V_OUT_MIN) && (fval < 0))
			{
				fint0[chn] = (float)(fint0[chn] + FAINT[chn] * fval[chn]);
			}

		   feint[chn] = fint0[chn];
		   fepro[chn] = (float)(FAPRO[chn] * fmpro[chn]);
		   fedif[chn] = (float)(FADIF[chn] * fmdif[chn]);
		   fcor[chn] = feint[chn] + fepro[chn] + fedif[chn];

		   if (fmwin[chn] == 0)
			{
				fVp[chn] = fV0[chn] + fcor[chn];
			}
		   else
			{
			   fVp[chn] = (float)(fV0[chn] + fcor[chn] + ((FCW[chn] * fmwin[chn] * fm10w[chn]) / fmwin[chn]));
			}

		   if (fVp[chn] < V_OUT_MIN)
			{
				fVp[chn] = V_OUT_MIN;
			}
         if (s45<0.5)
         {
            if (fVp[chn] > 1)
			{
				fVp[chn] =1;
         }
         }

		   if (fVp[chn] > V_OUT_MAX)
			{
				fVp[chn] = V_OUT_MAX;
            //CO2_rem = 0;

			}
		fVs[chn]=fVp[chn];
}
		}
	}  //End Layertype=0

	if(LAYER[chn]==1)  // O3 CONTROL LAYER
	{
   	//tam0509 if (ora<sunr || ora>suns || !ozonator_loc || !ozonator_rem || !flow  )   // If nighttime or ozone off
      if (ora <=  STARTTIME_OZONE || ora > STOPTIME_OZONE || !ozonator_loc || !ozonator_rem || !flow  )   // If nighttime or ozone off
	  	{
	  	fVs[chn]=0;
      }
      else // Daytime and ozone on
      {
         if ((nCont01[chn] <= DIM_L)&&flow) //Delay for first 45 seconds
			{
      fFpro0[chn][nCont01[chn]] = fval[chn];
	   if (nCont01[chn] == 1)
	 	{
		   fFdif0[chn][nCont01[chn]] = fFpro0[chn][nCont01[chn] - 0];
		}
	   if (nCont01[chn] > 1)
		{
	   	fFdif0[chn][nCont01[chn]] = fFpro0[chn][nCont01[chn]] - fFpro0[chn][nCont01[chn] - 1];
		}

	   fFspro[chn] = fFspro[chn] + fFpro0[chn][nCont01[chn]];
	   fFsdif[chn] = fFsdif[chn] + fFdif0[chn][nCont01[chn]];

	   if ((fVp[chn] > V_OUT_MIN) && (fVp[chn] < V_OUT_MAX))
		{
	   	fint0[chn] = fint0[chn] + FAINT[chn] * fval[chn];
		}
	   if ((fVp[chn] == V_OUT_MIN) && (fval < 0))
		{
		   fint0[chn] = fint0[chn] + FAINT[chn] * fval[chn];
		}
	   if ((fVp[chn] == V_OUT_MAX) && (fval > 0))
		{
			fint0[chn] = fint0[chn] + FAINT[chn] * fval[chn];
	   }

	   nCont01[chn]++;
	   fVs[chn]=fVp[chn];
	}
else
{
	   	fFspro[chn] = fFspro[chn] - fFpro0[chn][ nCont02[chn]];
	   	fFsdif[chn] = fFsdif[chn] - fFdif0[chn][ nCont02[chn]];
	   	fFpro0[chn][ nCont02[chn]] = fval[chn];

		   if (nCont02[chn] == 1)
		  	{
         	fFdif0[chn][nCont02[chn]] = fFpro0[chn][nCont02[chn]] - fFpro0[chn][ (DIM_L-1)];
		   }

		   if (nCont02[chn] > 1)
		   {
         	fFdif0[chn][nCont02[chn]] = fFpro0[chn][nCont02[chn]] - fFpro0[chn][ nCont02[chn] - 1];
			}

         fFspro[chn] = fFspro[chn] + fFpro0[chn][nCont02[chn]];
         fFsdif[chn] = fFsdif[chn] + fFdif0[chn][nCont02[chn]];
		   nCont02[chn]++;
		   nCont04[chn]++;

		   if (nCont02[chn] > DIM_L)
			{
				nCont02[chn] = 1;
			}

		   if (nCont04[chn] > MB)
			{
				nCont04[chn] = 1;
			}

		   fmpro[chn] = fFspro[chn] / DIM_L;
		   fmdif[chn] = fFsdif[chn] / DIM_L;
		   fmwin[chn] = s45;
		   fm10w[chn] = s10;

		   if ((fVp[chn] > V_OUT_MIN) && (fVp[chn] <V_OUT_MAX))
			{
				fint0[chn] = (float)(fint0[chn] + FAINT[chn] * fval[chn]);
			}

		   if ((fVp[chn] == V_OUT_MIN) && (fval < 0))
			{
				fint0[chn] = (float)(fint0[chn] + FAINT[chn] * fval[chn]);
			}

		   feint[chn] = fint0[chn];
		   fepro[chn] = (float)(FAPRO[chn] * fmpro[chn]);
		   fedif[chn] = (float)(FADIF[chn] * fmdif[chn]);
		   fcor[chn] = feint[chn] + fepro[chn] + fedif[chn];

		   if (fmwin[chn] == 0)
			{
				fVp[chn] = fV0[chn] + fcor[chn];
			}
		   else
			{
			   fVp[chn] = (float)(fV0[chn] + fcor[chn] + ((FCW[chn] * fmwin[chn] * fm10w[chn]) / fmwin[chn]));
			}

		   if (fVp[chn] < V_OUT_MIN)
			{
				fVp[chn] = V_OUT_MIN;
			}

		   if (fVp[chn] > V_OUT_MAX)
			{
				fVp[chn] = V_OUT_MAX;
            // tam0520  ozonator_loc = 0;
	    // tam0520 fVp[chn]=0;        //Changed 05/05

			}
		fVs[chn]=fVp[chn];
		}
	}  //End Layertype=1

	}  //Chiude il loop della notte 18.08.2000
if(LAYER[chn]==2)  // LAYER OFF
   {
   fVp[chn] = 0;
   fVs[chn]=fVp[chn];
   }



		txlm[chn] = txlm[chn] + fVs[chn];
		medo[chn] = medo[chn] + fpps[chn];
		vento[chn] = fwind + vento[chn];
}


return;
}


void set_output(void)
{
for(chn=0;chn < N_CHANNELS+1;chn++)
	{
	if(LAYER[chn]==0)  // CO2 CONTROL LAYER
		{
		if ((ora <= STARTTIME_CO2 || ora > STOPTIME_CO2) && !(nighttime_remote && nighttime_local))   // Esegue le operazioni solo di giorno 18.08.2000
			{
      	//anaOutVolts( chn, 0);
         #ifdef EXTDA
         rn_anaOutVolts(device0, DA_Channel[chn],  0, &DacCalTable1, 0);
         //digOut(ChanAddr(RELAY, 2),TURNOFF);  //CO2 shutoff valve
         #endif
         }
      else
      	{
         //anaOutVolts(chn, (fVs[chn]*DA_MULT[chn]));
         //printf("%5.2f \n",(fVs[chn]*DA_MULT[chn]));
         //rn_anaOutVolts(device0, chn,  (fVs[chn]*DA_MULT[chn]), &DacCalTable1, 0);
         #ifdef EXTDA
         rn_anaOutVolts(device0, DA_Channel[chn],  (fVs[chn]*DA_MULT[chn]), &DacCalTable1, 0);
         #endif
         // rn_anaOutVolts(device0, 0,  5, &DacCalTable1, 0);

         //digOut(ChanAddr(RELAY, 2),TURNON);  //CO2 shutoff valve
         }
   	}
   if(LAYER[chn]==1)  // O3 CONTROL LAYER
		{
		//tam0509 if (ora<sunr || ora>suns)   // Esegue le operazioni solo di giorno 18.08.2000
      if (ora <= STARTTIME_OZONE || ora > STOPTIME_OZONE)   // Esegue le operazioni solo di giorno 18.08.2000
			{
      	//anaOutVolts( DA_Channel[chn], 0);
          #ifdef EXTDA
          rn_anaOutVolts(device0, DA_Channel[chn],  0, &DacCalTable1, 0);
         #endif
         }
      else
      	{
         //anaOutVolts(DA_Channel[chn], (fVs[chn]*DA_MULT[chn]));
         #ifdef EXTDA
         rn_anaOutVolts(device0, DA_Channel[chn],  (fVs[chn]*DA_MULT[chn]), &DacCalTable1, 0);
         #endif

         }
   	}
	}

   anaOutVolts(0,5); // set excitation voltage for wind direction from main board
   #ifdef EXTDA
   rn_anaOutVolts(device0, 7,  5, &DacCalTable1, 0); // Set excitation voltage for wind direction
   //rn_anaOutStrobe(device0, 0);
   #endif



/*        if ((ora<sunr | ora>suns) && !nighttime) //It's time to sleep at night
             {
             for(nI=0; nI<=N_CHANNELS;nI++)    //Zero analog output
             	{
	     	  		anaOutVolts(ChanAddr(DABRD, nI), 0);

             	}
             }
      	else
      	      //It's time to work during the day
             {
             for(nI=0; nI<=N_CHANNELS;nI++)
             	{
             	anaOutVolts(ChanAddr(DABRD, DA_Channel[nI]), (fVs[nI]*DA_MULT[nI]));

             	}
             }
*/
}

void second_data(void)
{
char mess[20];
auto int retval1;
	tm_rd(&rtc);
	sec_count++;
   //SEND DATA TO DISPLAY
      #ifdef WIRELESS
      sprintf(message,"U,%d,%d,%d,%d,%d,%d \r", ozonator_loc, ozonator_rem, flow|purge, ozone_power, ozone_connect,linkpending(IF_WIFI0));
      #endif
      #ifdef WIRED
      sprintf(message,"U,%d,%d,%d,%d,%d,%d \r", ozonator_loc, ozonator_rem, flow|purge, ozone_power, ozone_connect,linkpending(IF_ETH0));
      #endif

      serEputs(message);
   	while (serEwrFree() != EOUTBUFSIZE) ;

      //printf("S,%d,%02d/%02d/%04d,%02d:%02d:%02d,%.2f,%.0f,%.0f,%.2f,%.0f,%.0f,%.1f,%.0f,%d \n",UNIT_ID,rtc.tm_mon,rtc.tm_mday,(rtc.tm_year+1900),rtc.tm_hour,rtc.tm_min,rtc.tm_sec,fVs[0],fpps[0],TARG[0],fVs[1],fpps[1],TARG[1],fFwind,Direz,opSect[0]);

      sprintf(message,"S,%d,%02d/%02d/%04d,%02d:%02d:%02d,%.2f,%.0f,%.0f,%.2f,%.0f,%.0f,%.1f,%.0f,%d \r",UNIT_ID,rtc.tm_mon,rtc.tm_mday,(rtc.tm_year+1900),rtc.tm_hour,rtc.tm_min,rtc.tm_sec,fVs[0],fpps[0],TARG[0],fVs[1],fpps[1],TARG[1],fFwind,Direz,opSect[0]);

         serEputs(message);
   		while (serEwrFree() != EOUTBUFSIZE) ;



	if(sec_count >= GRAB_TIME)
	{
   ip_print_ifs();
   //network_connect = linkpending(IF_WIFI0);
   //printf ("%d Network status \n",linkpending(IF_WIFI0));

#ifdef NETWORK
      sprintf(mess, "H,%d,%.2f\r",(flow|purge),fVs[1]);  //H Primary I Secondary ring controller.
   	sock_write(&sock_B,mess,strlen(mess));
#endif


	if(!Polled) //Send data to serial port
	{
  		 serFputs(message);
   		while (serFwrFree() != FOUTBUFSIZE) ;
		//serEputs(message);
   	//	while (serEwrFree() != EOUTBUFSIZE) ;

	}
#ifdef NETWORK
	if(state_A >> 1)
		{
      sprintf(message,"S,%d,%02d:%02d:%02d,%.2f,%.0f,%.0f,%.2f,%.0f,%.0f,%.1f,%.0f,%d \r",UNIT_ID,rtc.tm_hour,rtc.tm_min,rtc.tm_sec,fVs[0],fpps[0],TARG[0],fVs[1],fpps[1],TARG[1],fFwind,Direz,opSect[0]);

		//for(i=0;i<20;i++){
		retval1 = sock_fastwrite(&sock_A,message,strlen(message));
		}
#endif
#ifdef FAT
   sprintf(message,"S,%d,%02d/%02d/%04d,%02d:%02d:%02d,%.2f,%.0f,%.0f,%.2f,%.0f,%.0f,%.1f,%.0f,%d \r\n",UNIT_ID,rtc.tm_mon,rtc.tm_mday,(rtc.tm_year+1900),rtc.tm_hour,rtc.tm_min,rtc.tm_sec,fVs[0],fpps[0],TARG[0],fVs[1],fpps[1],TARG[1],fFwind,Direz,opSect[0]);
      //sprintf(datafile,"SECOND.csv",rtc.tm_mon,rtc.tm_mday,(1900 + rtc.tm_year));
      sprintf(datafile,"S%02d%02d%02d.csv",rtc.tm_mon,rtc.tm_mday,(1900 + rtc.tm_year - 2000));
      writefile(message, datafile,1);
#endif
   sec_count = 0;
	}

	if(!menudisp)
	{
  //	glPrintf(0,0, &fi6x8, "%02d/%02d/%04d  %02d:%02d:%02d", rtc.tm_mon,rtc.tm_mday,(1900 + rtc.tm_year),rtc.tm_hour,rtc.tm_min,rtc.tm_sec);
  //	glPrintf(0,8, &fi6x8, "Wind:%4.1f/%3.0f Sect:%d",fFwind,Direz,opSect[0]);
	//glPrintf(0,16, &fi6x8, "1)%s%4.0f Out:%5.2f",treatments[LAYER[0]],fpps[0], fVs[0]);
	//glPrintf(0,24, &fi6x8, "1)TARG:%4.0f ",TARG[0]);
	//glPrintf(0,16, &fi6x8, "%s%4.0f/%4.0f V:%4.2f",treatments[LAYER[0]],fpps[0],TARG[0], fVs[0]);
	//glPrintf(0,24, &fi6x8, "%s%4.0f/%4.0f V:%4.2f",treatments[LAYER[1]],fpps[1],TARG[1], fVs[1]);

	}

}






//*************************************************************************************************************************************************************************************************
//******    Minute averages
//*************************************************************************************************************************************************************************************************
void minute_average(void)
{
auto int retval1;
calc_day();
for(chn=0;chn < N_CHANNELS+1;chn++)
	{
		txlm[chn] = txlm[chn] / nContao[chn];
		vento[chn] = vento[chn] / nContao[chn];
		medo[chn] = medo[chn] / nContao[chn];
	}
		//fDew = fDew/nContao[0];

		//Find1 (int)StatusCode = ozonator_loc && ozonator_rem && flow;
		//printf("%d,%d,%d \n",ozonator_loc, ozonator_rem, (int)StatusCode);
	if(!Polled)  // Write avg to serial port
		{
			sprintf(message,"M,%d,%02d/%02d/%04d,%02d:%02d:%02d,%.2f,%.0f,%.0f,%.2f,%.0f,%.0f,%.1f,%.0f,%d,%d,%.2f \r",UNIT_ID,rtc.tm_mon,rtc.tm_mday,(1900 + rtc.tm_year),rtc.tm_hour,rtc.tm_min,rtc.tm_sec,txlm[0],medo[0],TARG[0],txlm[1],medo[1],TARG[1],vento[0],fFlow,opSect[0],StatusCode,fDew);
			serEputs(message);
   			while (serEwrFree() != EOUTBUFSIZE) ;
#ifdef FAT
         sprintf(message,"M,%d,%02d/%02d/%04d,%02d:%02d:%02d,%.2f,%.0f,%.0f,%.2f,%.0f,%.0f,%.1f,%.0f,%d,%d,%.2f \r\n",UNIT_ID,rtc.tm_mon,rtc.tm_mday,(1900 + rtc.tm_year),rtc.tm_hour,rtc.tm_min,rtc.tm_sec,txlm[0],medo[0],TARG[0],txlm[1],medo[1],TARG[1],vento[0],fFlow,opSect[0],StatusCode,fDew);
          sprintf(datafile,"M%02d%02d%02d.csv",rtc.tm_mon,rtc.tm_mday,(1900 + rtc.tm_year - 2000));
         //sprintf(datafile,"DAILY.csv",rtc.tm_mon,rtc.tm_mday,(1900 + rtc.tm_year));
		writefile(message, datafile,1);
#endif
      #ifdef NETWORK
	if(state_A >> 1)
		{
		//for(i=0;i<20;i++){
		retval1 = sock_fastwrite(&sock_A,message,strlen(message));
		}
#endif

		}

for(chn=0;chn < N_CHANNELS+1;chn++)
	{
   if(LAYER[chn]==1)
   	{
      if (fVp[chn] > V_OUT_MAX)
			{
				fVp[chn] = V_OUT_MAX;
            //ozonator_loc = 0;  //Changed 05/05  Moving to Minute Average
            //fVp[chn]=0;        //Changed 05/05

			}
		}
	}




      //fMedo=medo[chn]; // CO2 avg

      //fResA=fVp[chn]; // Output Volts



        	if(RECORD > 2975) RECORD=0;
        	//write_mem(RECORD);
        	RECORD = RECORD + 1;

for(chn=0;chn < N_CHANNELS+1;chn++)
	{

		medo[chn] = 0;
		vento[chn] = 0;
		txlm[chn] = 0;
		nContao[chn] = 0;
	}
		fDew = 0;


}


void calc_day(void)
{
double d, m, y;
tm_rd(&rtc);
y = 2005;

CalcSun(rtc.tm_year, rtc.tm_mon, rtc.tm_mday, 40.04,-88.13,-6);
//printf("%d,%d,%d,%d \n",d,m,riset,settm);
sunr = (float)riset;
suns = (float)settm;
ora=rtc.tm_hour+(float)(rtc.tm_min/60.);
//printf("%f %f %f \n",sunr, suns, ora);

}

void Reset_Ozone_Vars(void)
{

      for(nI=0; nI<N_CHANNELS+1;nI++)    // Inizializza  contatori
   	{
			if(LAYER[nI] == 1)
			{
   		fV0[nI]=0;
         //byIChA[nI]=0;
			//byIChB[nI]=0;
      	feint[nI]=0;
	      fepro[nI]=0;
	      fedif[nI]=0;
         fFspro[nI]=0;
	      fFsdif[nI]=0;
	      fFswin[nI]=0;
	      fint0[nI]=0;
	      nCont01[nI]=1;
	      nCont02[nI]=1;
	      nCont03[nI]=1;
	      nCont04[nI]=1;
	      nContao[nI]=0;
    		txlm[nI] = 0;
			vento[nI] = 0;
			medo[nI] = 0;

	      fVp[nI]=fV0[nI];
	      if (fVp[nI]>V_OUT_MAX)
	         {
	         fVp[nI]=V_OUT_MAX;
	         }
	      for(wI=1; wI<=DIM_L;wI++)    // Inizializza  contatori

	         {
	         fFpro0[nI] [wI]=0;
	         fFdif0[nI] [wI]=0;
	         fFwin[wI]=0;
	         fF10win[wI]=0;
	         }
  		 	 }
       }

}

/************************************************************/
/*                    Calibrate                             */
/************************************************************/
void calibrate(int sector)
{
float temp;
int memo_sec1;
int memo_sec2;
int m;


   anaOutVolts(DA_Channel[0], 5);
   anaOutVolts(DA_Channel[1], 3);
   digOut(1,TURNON); //Turn on AC flow
	//anaOutVolts(ChanAddr(DABRD, 0), 5);  //Set Control Valve at 50%
	//anaOutVolts(ChanAddr(DABRD, 1), 5);  //Set Control Valve at 50%
top:
	for(m=0;m<8;m++)			//Turn all valves off
		{
   	digOut_H(m,TURNOFF);

		}
   memo_sec2 = (int)SEC_TIMER+5;

	while(1)
		{



			digOut_H(sector, TURNON);
           	while(1)
				{

				costate
				{
				readSerialE();
   			if (memo_sec1 != (int)SEC_TIMER)
					{
					//hitwd();
	            temp=((float)anaInVolts(0,2) * 400)  ;
	            //glPrintf(0,0, &fi6x8, "   <<CALIBRATE>>");
					//	glPrintf(0,8, &fi6x8, "Sector : %d",sector);
					//	glPrintf(0,16, &fi6x8, "1)CO2:%4.0f Out:5.00",temp);
						//glPrintf(0,24, &fi6x8, "2)CO2:%4.0f Out:%5.2f",fpps[1], fVs[1]);

					memo_sec1 = (int)SEC_TIMER;
					}
				}
			}

		}



}

void readSerialE(void)
{
	input_char = serEgetc();
	//printf("%c \n",input_char);

	if(input_char == '\r')
		{
		SENTENCE_E[string_pos_e++] = '\0';

		readString_E();
		}
	else if(input_char > 100)
		{
		}

	else if(input_char > 0)
		{

		SENTENCE_E[string_pos_e] = input_char;
		string_pos_e++;
		//printf("%d \t %c \n",input_char,input_char);
		}
}


void readSerialF(void)
{
	input_char = serFgetc();
	//printf("%c \n",input_char);

	if(input_char == '\r')
		{
		SENTENCE_F[string_pos_f++] = '\0';

		readString_F();
		}
	else if(input_char > 100)
		{
		}

	else if(input_char > 0)
		{

		SENTENCE_F[string_pos_f] = input_char;
		string_pos_f++;
		//printf("%d \t %c \n",input_char,input_char);
		}
}


void readString_E(void) //CONNECTION TO OP6800 DISPLAY
{
		int i;
		int location;
		printf("%s \n",SENTENCE_E);
		token = strtok(SENTENCE_E,delim);

		switch(*token) // Read Command
		{
      	case 65: //A Set CO2 local
         	string_pos_e = 0;
            token = strtok(NULL,delim);
               if (atoi(token) == UNIT_ID)
               {
               token = strtok(NULL,delim);
					CO2_loc = atoi(token);
               }


         break;
      	case 66: //B Set CO2 remote
         	string_pos_e = 0;
				token = strtok(NULL,delim);
				CO2_rem = atoi(token);

         break;


			/*case 67: //C Set CO2 Nightime
				string_pos_e = 0;
				token = strtok(NULL,delim);
				nighttime_remote = atoi(token);
            token = strtok(NULL,delim);
            nighttimeCO2 = atoi(token);
			break;
         */
         case 67: //Calibrate
				string_pos_e = 0;
				token = strtok(NULL,delim);
				//printf("%d \n",atoi(token));
				calibrate(atoi(token));
			break;

			case 68: //D Download
				printf("Download \n");
				string_pos_e = 0;
				//download(RECORD);
			break;

            case 69: //E Calibrate
				exit(-1);

				//download(RECORD);
			break;



         case 78: //N,1 Set Ozone Local
         	string_pos_e = 0;
				//token = strtok(NULL,delim);
              // if (atoi(token) == UNIT_ID)
               //{
                string_pos_e = 0;
			  		token = strtok(NULL,delim);
					ozonator_loc =  atoi(token);
               //}

         	break;



	case 79: // O,1Set Ozone
				token = strtok(NULL,delim);
            token = strtok(NULL,delim);

            ozonator_rem = atoi(token);	//set ozone
				//tam0509 if ((ora>sunr && ora<suns)  && ozonator_rem ) digOut(ChanAddr(RELAY, 0),TURNON);
            if ((ora >= STARTTIME_OZONE && ora < STOPTIME_OZONE)  && ozonator_rem ) digOut(0,TURNON);
            else digOut(0,TURNOFF);

				token = strtok(NULL,delim);
				for(i=0;i < N_CHANNELS+1;i++)
					{

					if(LAYER[i] == 1)
						{
						//nCont01[i]=1;
	  					//nCont02[i]=1;
	   				//nCont03[i]=1;
	   				//nCont04[i]=1;
	   				//nContao[i]=0;
						//medo[i] = 0;
						//vento[i] = 0;
						//txlm[i] = 0;
						//nContao[i] = 0;
						//tam0509 TARG[i] = (atof(token)* OZONE_MULT);
                  //tam0509 if(TARG[i] > OZONE_MAX) TARG[i] = OZONE_MAX;
						}
					fDew = 0;
 					}
				string_pos_e = 0;
			break;
         // *** Added 03/27/04
         case 82:   //Reset Controller (R)
            string_pos_e = 0;
            token = strtok(NULL,delim);
               if (atoi(token) == UNIT_ID)
               {
               #ifdef FAT
               rc = fat_Close(&my_file);

	                  if (rc < 0) {
	                     printf("fat_Close() failed with return code %d\n", rc);

	                  }
               fat_UnmountDevice(first_part->dev);
               #endif
               exit(0);
               }
         break;

  			case 83: //Transmit second data (S)
				string_pos_e = 0;
				token = strtok(NULL,delim);
               if (atoi(token) == UNIT_ID)
               {
                  sprintf(message,"S,%d,%02d:%02d:%02d,%.2f,%.0f,%.0f,%.2f,%.0f,%.0f,%.1f,%.0f,%d \r",UNIT_ID,rtc.tm_hour,rtc.tm_min,rtc.tm_sec,fVs[0],fpps[0],TARG[0],fVs[1],fpps[1],TARG[1],fFwind,Direz,opSect[0]);
						serEputs(message);
   					while (serEwrFree() != EOUTBUFSIZE) ;
               }

			break;


			case 84: //Set Time
				string_pos_e = 0;
				token = strtok(NULL,delim);
				rtc.tm_mon = atoi(token);	//set month
				token = strtok(NULL,delim);
				rtc.tm_mday = atoi(token); //set day
				token = strtok(NULL,delim);
				rtc.tm_year = (atoi(token)-1900);	//set year
				token = strtok(NULL,delim);
				rtc.tm_hour = atoi(token); //set hour
				token = strtok(NULL,delim);
				rtc.tm_min = atoi(token); //set minute
				token = strtok(NULL,delim);
				rtc.tm_sec = atoi(token); //set second

				tm_wr(&rtc);
				SEC_TIMER = mktime(&rtc);
				sprintf(message,"T,%02d/%02d/%04d,%02d:%02d:%02d \r",rtc.tm_mon,rtc.tm_mday,(rtc.tm_year+1900),rtc.tm_hour,rtc.tm_min,rtc.tm_sec);
						serEputs(message);
   					while (serEwrFree() != EOUTBUFSIZE) ;
             calc_day();
			break;

         case 86: //V Reset Ozone Variables
         	string_pos_e = 0;
         	Reset_Ozone_Vars();
         break;

         case 87: //W Hard Reset Ozone Rings
            string_pos_e= 0;
				token = strtok(NULL,delim);
               if (atoi(token) == UNIT_ID)
               {
                	Reset_Ozone_Vars();
                  printf("Reset Ozone Variables this ring only\n");
               }


         break;

         case 90:  // "Z" RECEIVE INITIAL PARAMETERS
         	string_pos_e = 0;
            for (i=0;i<13 ;i++)
            {
            	   token = strtok(NULL,delim);
                  settings[i] = atol(token);
	               printf("settings.[x] = %ld\n", settings[i]);

	               }
             	save_data[0] = &settings;
  					save_lens[0] = sizeof(settings);
  					 writeUserBlockArray(0, save_data, save_lens, 1);

              exit(0);  //RESET AFTER CHANGINC PARAMETERS
            break;




			default:


		}
		string_pos_e = 0;
}

void readString_F(void)
{
		int location;
		printf("%s \n",SENTENCE_F);
		token = strtok(SENTENCE_F,delim);

		switch(*token) // Read Command
		{

			case 67: //Calibrate
				string_pos_f = 0;
				token = strtok(NULL,delim);
				//printf("%d \n",atoi(token));
				calibrate(atoi(token));
			break;

			case 68: //Download
				printf("Download \n");
				string_pos_f = 0;
				//download(RECORD);
			break;

			case 82:
				string_pos_f = 0;

				cont_loop();
			break;

			case 84: //Set Time
				string_pos_f = 0;
				token = strtok(NULL,delim);
				rtc.tm_mon = atoi(token);	//set month
				token = strtok(NULL,delim);
				rtc.tm_mday = atoi(token); //set day
				token = strtok(NULL,delim);
				rtc.tm_year = (atoi(token)-1900);	//set year
				token = strtok(NULL,delim);
				rtc.tm_hour = atoi(token); //set hour
				token = strtok(NULL,delim);
				rtc.tm_min = atoi(token); //set minute
				token = strtok(NULL,delim);
				rtc.tm_sec = atoi(token); //set second

				tm_wr(&rtc);
				SEC_TIMER = mktime(&rtc);
            calc_day();
			break;

  			case 83: //Transmit second data (S)
				string_pos_f = 0;
				token = strtok(NULL,delim);
               if (atoi(token) == UNIT_ID)
               {
                  sprintf(message,"S,%d,%02d:%02d:%02d,%.2f,%.0f,%.0f,%.2f,%.0f,%.0f,%.1f,%.0f,%d \r",UNIT_ID,rtc.tm_hour,rtc.tm_min,rtc.tm_sec,fVs[0],fpps[0],TARG[0],fVs[1],fpps[1],TARG[1],fFwind,Direz,opSect[0]);
						serFputs(message);
   					while (serFwrFree() != FOUTBUFSIZE) ;
               }

			break;

			default:


		}
		string_pos_f = 0;
}


void readString_IP(void)
{
		int i;
		int location;
      auto int retval1;
		printf("%s \n",SENTENCE_IP);
		token = strtok(SENTENCE_IP,delim);

		switch(*token) // Read Command
		{

      	case 6:  //Acknowledge Data Received on other end
           string_pos_ip = 0;
           printf("Data Acknowledged \n");
         break;

      	case 65: //A Set CO2 local
         	printf("Data Acknowledged 65 \n");
            string_pos_ip = 0;
            token = strtok(NULL,delim);//IGNORE UNIT ID
            token = strtok(NULL,delim);
               if (atoi(token) == UNIT_ID)
               {
               token = strtok(NULL,delim);
					CO2_loc = atoi(token);
               }


         break;

      	case 66: //B Set CO2 remote
         printf("Data Acknowledged 66 \n");
         	string_pos_ip = 0;
            token = strtok(NULL,delim);//IGNORE UNIT ID
				//token = strtok(NULL,delim);
				CO2_rem = atoi(token);

         break;


			case 67: //C Set CO2 Nightime
         printf("Data Acknowledged 67 \n");
				string_pos_ip = 0;
            token = strtok(NULL,delim); //IGNORE UNIT ID
				token = strtok(NULL,delim);
				nighttime_remote = atoi(token);
            token = strtok(NULL,delim);
            nighttimeCO2 = atoi(token);
			break;

			case 68: //D Download
         	printf("Data Acknowledged 68\n");
				printf("Download \n");
				string_pos_ip = 0;
				//download(RECORD);
			break;



         case 78: //N Set Ozone Remote
         printf("Data Acknowledged 78\n");
         	string_pos_ip = 0;
				token = strtok(NULL,delim);//IGNORE UNIT ID
            token = strtok(NULL,delim);
				ozonator_loc =  atoi(token);
         break;



			case 79: //Set Ozone
         printf("Data Acknowledged 79\n");
           //	token = strtok(NULL,delim);//IGNORE UNIT ID
				token = strtok(NULL,delim);
				ozonator_rem = atoi(token);	//set ozone
				//tam0509 if ((ora>sunr && ora<suns)  && ozonator_rem ) digOut(ChanAddr(RELAY, 0),TURNON);
            if ((ora >= STARTTIME_OZONE && ora < STOPTIME_OZONE)  && ozonator_rem ) digOut(0,TURNON);
            else digOut( 0,TURNOFF);

				token = strtok(NULL,delim);
				for(i=0;i < N_CHANNELS+1;i++)
					{

					if(LAYER[i] == 1)
						{
						//nCont01[i]=1;
	  					//nCont02[i]=1;
	   				//nCont03[i]=1;
	   				//nCont04[i]=1;
	   				//nContao[i]=0;
						//medo[i] = 0;
						//vento[i] = 0;
						//txlm[i] = 0;
						//nContao[i] = 0;
						//tam0509 TARG[i] = (atof(token)* OZONE_MULT);
                  //tam0509 if(TARG[i] > OZONE_MAX) TARG[i] = OZONE_MAX;
						}
					fDew = 0;
 					}
				string_pos_ip = 0;
			break;
         // *** Added 03/27/04
         case 82:   //Reset Controller (R)
          printf("Data Acknowledged 82\n");
            string_pos_ip = 0;
            token = strtok(NULL,delim);
            #ifdef FAT

                fat_UnmountDevice(first_part->dev);
                //rc = fat_AutoMount(FDDF_USE_DEFAULT);
            #endif
               exit(0);
          break;

  			case 83: //Transmit second data (S)
         	printf("Data Acknowledged 83\n");
				string_pos_ip = 0;
				token = strtok(NULL,delim);
               if (atoi(token) == UNIT_ID)
               {
                  sprintf(message,"S,%d,%02d:%02d:%02d,%.2f,%.0f,%.0f,%.2f,%.0f,%.0f,%.1f,%.0f,%d \r",UNIT_ID,rtc.tm_hour,rtc.tm_min,rtc.tm_sec,fVs[0],fpps[0],TARG[0],fVs[1],fpps[1],TARG[1],fFwind,Direz,opSect[0]);
						serEputs(message);
   					while (serEwrFree() != EOUTBUFSIZE) ;
               }

			break;


			case 84: //Set Time
         	printf("Data Acknowledged 84\n");
				string_pos_ip = 0;
				token = strtok(NULL,delim);
				rtc.tm_mon = atoi(token);	//set month
				token = strtok(NULL,delim);
				rtc.tm_mday = atoi(token); //set day
				token = strtok(NULL,delim);
				rtc.tm_year = (atoi(token)-1900);	//set year
				token = strtok(NULL,delim);
				rtc.tm_hour = atoi(token); //set hour
				token = strtok(NULL,delim);
				rtc.tm_min = atoi(token); //set minute
				token = strtok(NULL,delim);
				rtc.tm_sec = atoi(token); //set second

				tm_wr(&rtc);
				SEC_TIMER = mktime(&rtc);
				sprintf(message,"T,%02d/%02d/%04d,%02d:%02d:%02d \r",rtc.tm_mon,rtc.tm_mday,(rtc.tm_year+1900),rtc.tm_hour,rtc.tm_min,rtc.tm_sec);
						serFputs(message);
   					while (serFwrFree() != FOUTBUFSIZE) ;
             calc_day();
			break;

         case 86: //V Reset Ozone Variables
         	printf("Data Acknowledged 86\n");
         	string_pos_ip = 0;
         	Reset_Ozone_Vars();
         break;

         case 87: //W Hard Reset Ozone Rings
         printf("Data Acknowledged 87 \n");
         	for(nI=0; nI<N_CHANNELS+1;nI++)    // Inizializza  contatori
   				{
					if(LAYER[nI] == 1)
						{
                  exit(0);
                  }
               }

         break;




			default:


		}
		string_pos_ip = 0;
}
#ifdef FAT
 void writefile(char *message, char *datafile, int writemode)
{


	prealloc = 0;
   // Open (and maybe create) it...

   rc = fat_Open(
                 first_part,	// First partition pointer from fat_AutoMount()
                 datafile,//"HELLO.TXT",	// Name of file.  Always an absolute path name.
                 FAT_FILE,		// Type of object, i.e. a file.
                 FAT_CREATE,	// Create the file if it does not exist.
                 &my_file,		// Fill in this structure with file details
                 &prealloc		// Number of bytes to allocate.
                );

	if (rc < 0) {
   	printf("fat_Open() failed with return code %d\n", rc);
      exit(1);
   }

  // Write to it...
if (writemode == 1)
{
   rc = fat_Seek( &my_file,-1,SEEK_END);
}
   rc = fat_Write(
                  &my_file,				// File, as set by fat_Open()
                  message,	// Some data to write.
                  strlen(message) +1							// Number of characters to write.
                 );

   if (rc < 0) {
   	printf("fat_Write() failed with return code %d\n", rc);

      // In real applications which don't exit(), we would probably want to
      // close the file and continue with something else.
      exit(1);
   }
   // Done writing; close it.
   rc = fat_Close(&my_file);

   // Many programmers do not check the return code from "close".  This is a
   // bad idea, since an error return code can indicate that data was lost!
   // Your application should be concerned about this...
   if (rc < 0) {
   	printf("fat_Close() failed with return code %d\n", rc);
      // In this case, we soldier on to see if the file can be read.
   }

   printf("Data written OK. \n");
}
#endif

