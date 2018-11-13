/*
6/15/06  -  Added Ser(C&F)rdFlush to statements to clear input buffer before receiving more data for O3
6/15/06  -  Changed Timeouts from 200 to 400 for serial response time...
*/

#
use SUNRISE.lib
//#define INITIALIZE_PARAMS

# define UNIT_ID 0 // Unit Number used for COM Identification
# define Polled 0

# define TCPCONFIG 0# define _PRIMARY_STATIC_IP "192.168.1."#
define _PRIMARY_NETMASK "255.255.255.0"#
define MY_NAMESERVER "192.168.1.1"#
define MY_GATEWAY "192.168.1.1"

#
define USE_ETHERNET 1# define IFCONFIG_ETH0\
IFS_IPADDR, aton(_PRIMARY_STATIC_IP), \
  IFS_NETMASK, aton(_PRIMARY_NETMASK), \
  IFS_UP

# define UDP_SOCKETS 5 // allow enough for downloader and DHCP
# define MAX_UDP_SOCKET_BUFFERS 5# define LOCAL_PORT 420# define REMOTE_IP "255.255.255.255" /*broadcast*/ #
define ETH_MTU 900# define BUFF_SIZE(ETH_MTU - 40) //must be smaller than (ETH_MTU - (IP Datagram + TCP Segment))
# define TCP_BUF_SIZE((ETH_MTU - 40) * 4) // sets up (ETH_MTU-40)*2 bytes for Tx & Rx buffers
# define MAX_TCP_SOCKET_BUFFERS 6

# define ETH_MAXBUFS 3

# memmap xmem

# define UDPDL_LOADER "Z:/Z-World/UDPDL/Loaders/pdl-generic-D.bin"

#
use "dcrtcp.lib"#
use "udpdownl.lib"

#
define INCOMING_IP 0 //except all connections
# define INCOMING_PORT 0 //except all ports
# define TIME_OUT 10 // if in any state for more than a 2 seconds re-initialize

# define ADC_RTC 0 // thermocouple input
# define DAC_RTC 0 // output for thermocouple resistor network

# define N_CHANNELS 1# define DIM_L 10# define DIM_H 60

# define DELAYTIME 50 //350
# define PURGE_TIME 120

//---------------------------------------------------------
// Macro's
//---------------------------------------------------------
# define MAXDISPLAYROWS 4# define LEDOFF 0# define TOGGLE 1# define INCREMENT 2# define OPERATE 3

# define ASCII 0# define NUMBER 1
int hour;
int minute;
int gen_alarm_count; // Count for communications failure
float dayl;
float sunr;
float suns;
float ora; //Current Decimal Time
double pi = 3.14159265358979;
double degs;
double rads;
double L, g, daylen;
double twam, altmax, noont, settm, riset, twpm;
double AirRefr = 34.0 / 60.0; // athmospheric refraction degrees

int Ring_Connect[2];
int flow[2];
int purge[2];
int ozone_output[2];
int delay_restart[2]; // Ozone Restart Timer
unsigned long delay_restart_time[2]; // Ozone Restart Time
float ozone_ANA_out[2];
unsigned long purge_off[2];

shared long lVal, ipVal, rVal, nVal;

char ipbuff[20];
char ipbuffd[10];
tcp_Socket sock[3];
int bytes[3];
int state[3];
int sent[3];
int rcvd[3];
long statetime[3];
static char buff[3][BUFF_SIZE];
word my_port0, my_port[3];
char str[3][50], str_1[3][50];
char str_UDP[50], str_UDP1[50];

char * strscan;

//----------------------------------------------------------
// Main_Menu options
//----------------------------------------------------------
// Can insert/delete menu options. The highlight bar is setup
// to start with the first MENU option and stop at the last
// menu option in the MENU.
//
// When adding/deleting menu options you must match up the
// case statements to the menu option number.
//
long settings[6];
void * save_data[1];
unsigned int save_lens[1];

const char * treatments[] = {
  "CO2:",
  "O3 :",
  NULL
};

const char * label[] = {
  "ANA IN ",
  "Temp C ",
  "PSI    ",
  "DIG IN ",
  NULL

};

const char * token;
const char * delim = " , : / \n \r";

//----------------------------------------------------------
// Structures, arrays, variables
//----------------------------------------------------------
//fontInfo fi6x8, fi8x10, fi12x16;
//windowFrame textWindow;

typedef struct {
  int data;
  char * ptr;
}
fieldupdate;

struct tm CurTime;

char szTime[40];
char szString[20];
const char Days[] = {
  "SunMonTueWedThuFriSat"
};
const char Months[] = {
  "JanFebMarAprMayJunJulAugSepOctNovDec"
};

int ledCntrl;
int beeperTick, timerTick;
int max_menu_options;
int max_cmds_options;
unsigned long ulTime;
char * keybuffer;
unsigned long sendTime;

#
define CINBUFSIZE 127# define COUTBUFSIZE 127# define FINBUFSIZE 127# define FOUTBUFSIZE 127# define MAX_SENTENCE 100

# define TURNON 1# define TURNOFF 0

typedef unsigned char uchar;
typedef unsigned int uint;
//pedef unsigned long ulong;

static unsigned long store; // physical memory address to write to
long digOutBitsReceived, digInBitsToSend, mask;

//---------------------------------------------------------------------------
// DEFINIZIONE DELLE VARIABILI
//---------------------------------------------------------------------------
uchar wI;
uchar byIChA[N_CHANNELS + 1];
uchar byIChB[N_CHANNELS + 1];
uint outd;
int disp_offset;
uint chan_type[18];
float chan_value[18];

uint scan;
uint RECORD; //

uchar T1; //
uchar T2; //
uchar T3; //
uchar T4; //
uchar T5; //
uchar T6; //
uint V1; //
uint V2; //
uint V3; //
uint V4; //
uint V5; //
uint i;
char menudisp;

float fInp[10];
float dInp[10];
int count;
int ozone_power;

uint day;
uint month;
uint doy;

uint memPointer;

char inkey;

char locBuf[50];
char SENTENCE_F[MAX_SENTENCE];
char SENTENCE_C[MAX_SENTENCE];
char SENTENCE_IP[MAX_SENTENCE];
char alarmc[20];
char alarmf[20];
char o3c[20];
char o3f[20];
int input_char;
int string_pos_f;
int string_pos_c;
int string_pos_ip;

int backlight;
int timeOn;
const int timeDelay = 2;

int StatusCode;

struct tm rtc; //Time and Date struct
/* vars for telnet recieve daemon */

char message[250];
udp_Socket sockudp; //Added 113005
void calc_day(void);
void readString_C(void);
void readString_IP(char * str, word my_port0);
void cont_loop(void);
float readtemp(int channel);
void print_data(tcp_Socket * sock, int * state);
void read_o3(void);
void read_alarm(void);
void update(void);
void read_input(void);
void calc_day(void);

int receive_packet_udp(void) {
  auto int i;
  static char buf[128];

  #
  GLOBAL_INIT {
    memset(buf, 0, sizeof(buf));
  }

  /* receive the packet */
  if (-1 == udp_recv( & sockudp, buf, sizeof(buf))) {
    /* no packet read. return */
    return 0;
  }

  for (i = 0; i < sizeof(buf); i++) {
    input_char = buf[i];
    if (input_char == '\r') {
      //printf("Received Return \n");
      SENTENCE_IP[string_pos_ip++] = '\0';
      readString_IP(SENTENCE_IP);
    } else if (input_char > 0) {
      SENTENCE_IP[string_pos_ip] = input_char;
      string_pos_ip++;
      //printf("%d \t %c \n",input_char,input_char);
    }
  }

  memset(buf, 0, sizeof(buf));

  return 1;
}

int receive_packet(tcp_Socket * sock, char * buff, int * bytes, int * rcvd, char * str, word my_port) {
  /* receive the packet */
  * bytes = sock_fastread(sock, buff, BUFF_SIZE);

  switch ( * bytes) {
  case -1:
    return 4; // there was an error go to state 4 (NO_WAIT_CLOSE)
  case 0:
    return 2; // connection is okay, but no data received
  default:
    ( * rcvd) ++;
    //*strcat(buff, *input);

    buff[ * bytes] = '\0';
    strcat(str, buff);
    strscan = strrchr(str, '\r');
    if (strscan != '\0') {
      //printf("%s,%d, %d \n", str, strlen(str), my_port);

      readString_IP(str, my_port);
      str[0] = '\0';
    }

    return 3; //now go to state 3 (SEND)
  }
}

int send_packet(tcp_Socket * sock, char * buff, int * bytes, int * sent) {
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

MyHandle(tcp_Socket * sock, char * buff, int * bytes, int * state, word my_port,
  long * statetime, int * sent, int * rcvd, char * str) {
  int i;
  tcp_tick(sock);
  switch ( * state) {
  case 0:
    /*INITIALIZATION*/ // listen for incoming connection
    tcp_listen(sock, my_port, INCOMING_IP, INCOMING_PORT, NULL, 0);
    ( * statetime) = SEC_TIMER + TIME_OUT; // reset the statetime
    * sent = * rcvd = 0; // reset num of packets send and rcvd
    ( * state) ++; // init complete move onto next state
    //printf("Port Initialized\n");
    break;
  case 1:
    /*LISTEN*/
    if (sock_established(sock)) // check for a connection
      ( * state) ++; //   we have connection so move on
    else if ((long)(SEC_TIMER - ( * statetime)) > 0) // if 1 sec and no sock
      *
      state = 4; //	  abort and re-init

    break;
  case 2:
    /*RECEIVE*/
    if (my_port > 1000) {
      if (my_port == 1001) {
        i = 0;
      }
      if (my_port == 1002) {
        i = 1;
      }

      Ring_Connect[i] = TRUE;
    }

    * state = receive_packet(sock, buff, bytes, rcvd, str, my_port); // see function for details
    if ((long)(SEC_TIMER - ( * statetime)) > 0) // if 1 sec and still waiting
      *
      state = 4; //	  abort and re-init
    break;
  case 3:
    /*SEND*/
    ( * statetime) = SEC_TIMER + TIME_OUT; // reset the timer
    * state = send_packet(sock, buff, bytes, sent); // see function for details

    break;
  case 4:
    /*NO WAIT_CLOSE*/
    if (my_port > 1000) {
      if (my_port == 1001) {
        i = 0;
      }
      if (my_port == 1002) {
        i = 1;
      }

      ozone_output[i] = FALSE;
      if (flow[i]) {
        purge[i] = 1;
        flow[i] = 0;
        purge_off[i] = (int) SEC_TIMER + PURGE_TIME;
      }
    }

    sock_abort(sock);
    str[0] = '\0'; // close the socket
    * state = 0; // go back to the INIT state
    //printf("Port Closed\n");
  }
  tcp_tick(sock);
}

PrintIt(int x, int y, int bytes, int state, word port, long statetime, int rcvd, int sent, char * str) {
  printf("\x1B=%c%c bytes_A: %05d  state_A: %03d", x, y, bytes, state);
  printf(" my_port_A: %05d  statetime_A: %09ld", port, statetime);
  printf("\x1B=%c%c Rcvd: %06d  Sent: %06d, Len: %d,%s", x + 2, y + 1, rcvd, sent, strlen(str), str);

}

main() {

  int memo_second;
  int sec_now;
  int Ip;
  int i;

  int inputnum;
  int outputnum;
  char tempstr[20];
  //store = xalloc(65536);		// physical memory address (SRAM)
  my_port[0] = 1000;
  my_port[1] = 1001;
  my_port[2] = 1002;
  state[0] = state[1] = state[2] = 0;
  sent[0] = sent[1] = sent[2] = 0;
  rcvd[0] = rcvd[1] = rcvd[2] = 0;
  str[0][0] = str[1][0] = str[2][0] = '\0';
  flow[0] = 0;
  flow[1] = 0;
  purge[0] = 0;
  purge[1] = 0;

  gen_alarm_count = 0;

  brdInit();
  sock_init();
  UDPDL_Init("OzoneShed1");
  save_data[0] = & settings;
  save_lens[0] = sizeof(settings);
  readUserBlockArray(save_data, save_lens, 1, 0);
  ltoa(settings[0], ipbuffd);
  strcat(_PRIMARY_STATIC_IP, ipbuffd);
  ifconfig(IF_ETH0, IFS_DOWN,
    IFS_IPADDR, aton(_PRIMARY_STATIC_IP),
    IFS_NETMASK, aton(_PRIMARY_NETMASK),
    IFS_ROUTER_SET, aton(MY_GATEWAY),
    IFS_UP,
    IFS_END);

  printf("IP Address Entered = %s\r\n", _PRIMARY_STATIC_IP);

  if (!udp_open( & sockudp, LOCAL_PORT, resolve(REMOTE_IP), 0, NULL)) {
    //printf("udp_open failed!\n");
    exit(0);
  }
  //printf( inet_ntoa( tempstr, my_ip_addr ) );

  anaOutConfig(0, 0);
  anaOutPwr(1); // Initialize BL2600
  digOutConfig(0xFFFF); // Configure Outputs
  digHoutConfig(0x0F); // Configure HC Outputs
  digHTriStateConfig(0);

  anaInConfig(0, 1);
  anaInConfig(1, 1);
  anaInConfig(2, 1);
  anaInConfig(3, 1);

  save_data[0] = & settings;
  save_lens[0] = sizeof(settings);

  StatusCode = 0;
  serFopen(9600);
  serCopen(9600);
  serMode(0);
  memPointer = 0;
  count = 0;
  //----------------------------------------------------------------------
  // Initialize the variables
  //----------------------------------------------------------------------

  string_pos_f = 0;
  string_pos_c = 0;
  string_pos_ip = 0;
  SENTENCE_F[0] = '\0';
  SENTENCE_C[0] = '\0';
  SENTENCE_IP[0] = '\0';
  alarmc[0] = '\0';
  alarmf[0] = '\0';

  disp_offset = 0;

  chan_type[0] = 1; //Channel Type 0 = Voltage
  chan_type[1] = 1; //Channel Type 1 = Temperature (Thermister)
  chan_type[2] = 1; //Channel Type 2 = Pressure
  chan_type[3] = 0; //Channel Type 3 = Digital Logic
  chan_type[4] = 0;
  chan_type[5] = 0;
  chan_type[6] = 0;
  chan_type[7] = 2;
  chan_type[8] = 3;
  chan_type[9] = 3;
  chan_type[10] = 3;
  chan_type[11] = 3;
  chan_type[12] = 3;
  chan_type[13] = 3;
  chan_type[14] = 3;
  chan_type[15] = 3;
  chan_type[16] = 3;
  chan_type[17] = 3;

  #
  ifdef INITIALIZE_PARAMS
  settings[0] = 152;
  settings[1] = 25;
  settings[2] = 1;
  settings[3] = 1;
  settings[4] = 1;
  settings[5] = 1;
  save_data[0] = & settings;
  save_lens[0] = sizeof(settings);
  writeUserBlockArray(0, save_data, save_lens, 1);#
  endif

  readUserBlockArray(save_data, save_lens, 1, 0);

  //Print out what we are loading
  printf("\nLoading...\n");
  printf("settings.[0] = %ld\n", settings[0]);
  printf("settings.[1] = %ld\n", settings[1]);
  printf("settings.[2] = %ld\n", settings[2]);
  printf("settings.[3] = %ld\n", settings[3]);
  printf("settings.[4] = %ld\n", settings[4]);
  printf("settings.[5] = %ld\n", settings[5]);

  menudisp = 0;
  timeOn = (int) SEC_TIMER;

  ozone_power = 0;

  tm_rd( & rtc);
  hour = rtc.tm_hour;
  minute = rtc.tm_min;
  calc_day(); // Get Initial Sunrise/set numbers

  //----------------------------------------------------------------------
  //************************   MAIN LOOP    ******************************
  //----------------------------------------------------------------------

  anaOutPwr(1);
  while (1) {
    receive_packet_udp();
    if (UDPDL_Tick()) // this should be called at least twice a second.
    {
      printf("Download request pending!\n");
      // if you need to shut things down before the download, do it here.
    }
    for (i = 0; i < 3; i++) {
      MyHandle( & sock[i], buff[i], & bytes[i], & state[i], my_port[i], & statetime[i], & sent[i], & rcvd[i], str[i]);
      //PrintIt(0x20, 0x20, bytes_A, state_A, my_port_A, statetime_A, rcvd_A, sent_A, str_A);
    }

    costate {

      if (memo_second != (int) SEC_TIMER) {
        count++;
        memo_second = (int) SEC_TIMER;
        anaOutmAmps(3, (settings[1] * 0.2)); //Set Ozone Output Level
        //anaOutVolts(3, ((long)settings[1]* 0.05));

        // OZONE GENERATOR SETTINGS
        if (ozone_power && (ora > sunr && ora < suns) && gen_alarm_count < 5) {
          digOut(0, 0); //Turn H2O Bath on
          //printf("%d \n", digIn(16));
          if ((chan_value[2] < 25 || !settings[2]) && (chan_value[7] > 75 || !settings[3]) &&
            (digIn(16) || !settings[4])) digOut(1, 0); //Turn Ozone on if Temp !high, pressure, & flow
          else digOut(1, 1); //Turn ozone off if temp too high
        } else {
          digOut(0, 1); //Turn H2O Bath off
          digOut(1, 1); //Turn ozone off
        }

        //  CODE FOR OZONE TO RING PARAMETERS
        for (i = 0; i < 2; i++) {

          if (ozone_output[i] == TRUE) {
            purge[i] = FALSE;
            flow[i] = TRUE;
          } else {
            if (flow[i] == TRUE && purge[i] == FALSE) {
              flow[i] = FALSE;
              purge[i] = TRUE;
              purge_off[i] = (int) SEC_TIMER + PURGE_TIME;

            }

          }
          if ((flow[i] || purge[i]) == TRUE) {
            digOut(2 + i, 0);
          }
          if (purge[i] == TRUE && purge_off[i] <= (int) SEC_TIMER) {
            digOut(2 + i, 1);
            purge[i] = FALSE;
            flow[i] = FALSE;
          }
          if (flow[i] == TRUE) //  SET ANALOG OUTPUT IF FLOW and output = TRUE
          {
            anaOutVolts(1 + i, ozone_ANA_out[i]);
          } else {
            anaOutVolts(1 + i, 0);
          }
        }
        //  CODE FOR OZONE TO RING PARAMETERS CLOSE

        if (count > 4) {
          //ip_print_ifs();
          anaOutVolts(0, 1);
          read_o3();
          read_input();
          anaOutVolts(0, 0);
          update();
          read_alarm();
          print_data( & sock[0], & state[0]);

          count = 0;
          if (hour != rtc.tm_hour) //Calculate once per hour
          {
            calc_day();
            hour = rtc.tm_hour;
          }
          if (minute != rtc.tm_min) //Calculate once per hour
          {
            gen_alarm_count++;
            minute = rtc.tm_min;
          }
        }
      }
      hitwd();
    }

  }

}
void read_input() {

  digInBitsToSend = 0;
  mask = 0x8000; // mask bit is bit 15
  for (i = 31; i > 15; i--) {
    if (digIn(i) == 1) {
      digInBitsToSend = digInBitsToSend | mask; // Set bit
    }
    mask = mask >> 1; // shift mask bit right
  }
  //printf("%8.0f \n",(float)digInBitsToSend);

  for (i = 0; i < 18; i++) {
    switch (chan_type[i]) {
    case 0:
      chan_value[i] = anaInVolts(i, 1);
      break;

    case 1:
      chan_value[i] = readtemp(i);
      //chan_value[i] = 20;
      break;

    case 2:
      chan_value[i] = anaInVolts(i, 1) * 25 - 25;
      break;

    case 3:
      chan_value[i] = 0; //digIn(i-10);
      break;

    default:
    }
  }
}
float readtemp(int channel) {
  float R2, R1, V1, V2;
  float a[3], b[3], c[3], d[3], t; // variable to count temp readings
  float temptot; // variable to sum temp readings
  int tempn;
  float tempc;
  a[0] = .001116401;
  b[0] = 2.379829e-4;
  c[0] = -3.72283e-7;
  d[0] = 9.9063233e-8;
  temptot = 0.0;

  V1 = 1.0;
  R1 = 10000;

  temptot = 0.0;
  tempc = 0;

  for (tempn = 0; tempn < 10; tempn++) {

    V2 = anaInVolts(channel, 1); // read voltage from ADC0

    R2 = V2 * R1 / (V1 - V2); // compute thermistor resistance
    if (R2 > 100) {
      t = (1 / (a[0] + (b[0] * log(R2)) + (c[0] * log(R2) * log(R2)) + (d[0] * log(R2) * log(R2) * log(R2)))) - 273.15;
      temptot += t;
    } else {
      t = -9999;
      temptot += t;
    }
  }
  tempc = temptot / 10;

  return tempc;
}

void update() {
  int memo_second;
  memo_second = (int) SEC_TIMER + 1;
  while (memo_second > (int) SEC_TIMER) {

  }
  memo_second = (int) SEC_TIMER;
  //glPrintf(0,0, &fi6x8, "%02d/%02d/%04d  %02d:%02d:%02d", rtc.tm_mon,rtc.tm_mday,(1900 + rtc.tm_year),rtc.tm_hour,rtc.tm_min,rtc.tm_sec);

  //glPrintf(0,8, &fi6x8, "%s%02d : %6.2f", label[chan_type[disp_offset]], disp_offset + 0, chan_value[disp_offset]);
  //glPrintf(0,16, &fi6x8, "%s%02d : %6.2f", label[chan_type[disp_offset+1]], disp_offset + 1, chan_value[disp_offset+1]);
  //glPrintf(0,24, &fi6x8, "%s%02d : %6.2f", label[chan_type[disp_offset+2]], disp_offset + 2, chan_value[disp_offset+2]);
}

void read_o3() {
  int i;
  unsigned long timeout;
  serFrdFlush();
  sprintf(message, "%co3\r", 177);
  serFputs(message);
  while (serFwrFree() != FOUTBUFSIZE);
  timeout = MS_TIMER + 400;

  input_char = serFgetc();

  while (input_char != '\r') {
    receive_packet_udp();
    for (i = 0; i < 3; i++) {
      MyHandle( & sock[i], buff[i], & bytes[i], & state[i], my_port[i], & statetime[i], & sent[i], & rcvd[i], str[i]);
      //PrintIt(0x20, 0x20, bytes_A, state_A, my_port_A, statetime_A, rcvd_A, sent_A, str_A);
    }

    costate {

      input_char = serFgetc();
      if (input_char == '\r') {
        SENTENCE_F[string_pos_f++] = '\0';
        //printf("%s \n",SENTENCE_C);
        token = strtok(SENTENCE_F, delim);
        //printf("%s \n",token);
        token = strtok(NULL, delim);
        //printf("%s \n",token);
        strcpy(o3f, token);
        string_pos_f = 0;
        SENTENCE_F[0] = '\0';
      } else if (input_char > 0) {
        SENTENCE_F[string_pos_f] = input_char;
        string_pos_f++;
      }
      if (MS_TIMER > timeout) {
        strcpy(o3f, "-9999");
        break;

      }
    }
  }
  serCrdFlush();
  sprintf(message, "%co3\r", 177);
  serCputs(message);
  while (serCwrFree() != COUTBUFSIZE);
  timeout = MS_TIMER + 400;

  input_char = serCgetc();

  while (input_char != '\r') {
    receive_packet_udp();
    for (i = 0; i < 3; i++) {
      MyHandle( & sock[i], buff[i], & bytes[i], & state[i], my_port[i], & statetime[i], & sent[i], & rcvd[i], str[i]);
      //PrintIt(0x20, 0x20, bytes_A, state_A, my_port_A, statetime_A, rcvd_A, sent_A, str_A);
    }

    costate {

      input_char = serCgetc();
      if (input_char == '\r') {
        SENTENCE_C[string_pos_c++] = '\0';
        //printf("%s \n",SENTENCE_C);
        token = strtok(SENTENCE_C, delim);
        //printf("%s \n",token);
        token = strtok(NULL, delim);
        //printf("%s \n",token);
        strcpy(o3c, token);
        string_pos_c = 0;
        SENTENCE_C[0] = '\0';
      } else if (input_char > 0) {
        SENTENCE_C[string_pos_c] = input_char;
        string_pos_c++;
      }
      if (MS_TIMER > timeout) {
        strcpy(o3c, "-9999");
        break;

      }
    }
  }

}

void read_alarm() {
  unsigned long timeout;
  serFrdFlush();
  sprintf(message, "%cflags\r", 177);
  serFputs(message);
  while (serFwrFree() != FOUTBUFSIZE);

  timeout = MS_TIMER + 400;

  input_char = serFgetc();
  while (input_char != '\r') {
    receive_packet_udp();
    for (i = 0; i < 3; i++) {
      MyHandle( & sock[i], buff[i], & bytes[i], & state[i], my_port[i], & statetime[i], & sent[i], & rcvd[i], str[i]);
      //PrintIt(0x20, 0x20, bytes_A, state_A, my_port_A, statetime_A, rcvd_A, sent_A, str_A);
    }

    costate {

      input_char = serFgetc();
      if (input_char == '\r') {
        SENTENCE_F[string_pos_f++] = '\0';
        //printf("%s \n",SENTENCE_C);
        token = strtok(SENTENCE_F, delim);
        //printf("%s \n",token);
        token = strtok(NULL, delim);
        //printf("%s \n",token);
        strcpy(alarmf, token);
        string_pos_f = 0;
        SENTENCE_F[0] = '\0';
      } else if (input_char > 0) {
        SENTENCE_F[string_pos_f] = input_char;
        string_pos_f++;
      }
      if (MS_TIMER > timeout) {
        strcpy(alarmf, "-9999");
        break;
      }
    }
  }
  serCrdFlush();
  sprintf(message, "%cflags\r", 177);
  serCputs(message);
  while (serCwrFree() != COUTBUFSIZE);

  timeout = MS_TIMER + 400;

  input_char = serCgetc();
  while (input_char != '\r') {
    receive_packet_udp();
    for (i = 0; i < 3; i++) {
      MyHandle( & sock[i], buff[i], & bytes[i], & state[i], my_port[i], & statetime[i], & sent[i], & rcvd[i], str[i]);
      //PrintIt(0x20, 0x20, bytes_A, state_A, my_port_A, statetime_A, rcvd_A, sent_A, str_A);
    }

    costate {

      input_char = serCgetc();
      if (input_char == '\r') {
        SENTENCE_C[string_pos_c++] = '\0';
        //printf("%s \n",SENTENCE_C);
        token = strtok(SENTENCE_C, delim);
        //printf("%s \n",token);
        token = strtok(NULL, delim);
        //printf("%s \n",token);
        strcpy(alarmc, token);
        //printf("%s \n",alarmc);
        string_pos_c = 0;
        SENTENCE_C[0] = '\0';
      } else if (input_char > 0) {
        SENTENCE_C[string_pos_c] = input_char;
        string_pos_c++;
      }
      if (MS_TIMER > timeout) {
        strcpy(alarmc, "-9999");
        break;
      }
    }
  }

}
void print_data(tcp_Socket * sock, int * state) {
  auto int retval1;

  tm_rd( & rtc);
  ora = rtc.tm_hour + (float)(rtc.tm_min / 60.);

  sprintf(message, "%02d:%02d:%02d,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%s,%s,%s,%s,%.0f,%d\r", rtc.tm_hour, rtc.tm_min, rtc.tm_sec, chan_value[0], chan_value[1], chan_value[2], chan_value[3], chan_value[4], chan_value[5], chan_value[6], chan_value[7], chan_value[8], chan_value[9],
    o3f, alarmf, o3c, alarmc, (float) digInBitsToSend, settings[1]);
  if (state > 1) {
    retval1 = sock_fastwrite(sock, message, strlen(message));
  }

  if (!menudisp) {

  }

}

//------------------------------------------------------------------------
// Milli-sec delay function
//------------------------------------------------------------------------
nodebug
void msDelay(long sd) {
  auto unsigned long t1;

  t1 = MS_TIMER + sd;
  while ((long)(MS_TIMER - t1) < 0);
}

void receive_data(char * buf, int len) {
  /*
   * this is a user-function that will recieve the data
   * as it comes in. (buf points to the buffer, len is the
   * length of the data that was received.)
   *
   * Note - When this function returns, the buffer will be
   * overwritten, so if you want the data, it should be
   * coppied out of the buffer before the function returns.
   *
   * Nothing is done with it for now; it is just dropped on
   * the floor.
   */

  auto int i;
  for (i = 0; i < len; i++) {
    input_char = buf[i];
    if (input_char == '\r') {
      //printf("Received Return \n");
      SENTENCE_IP[string_pos_ip++] = '\0';
      readString_IP(SENTENCE_IP);
    } else if (input_char > 0) {
      SENTENCE_IP[string_pos_ip] = input_char;
      string_pos_ip++;
      //printf("%d \t %c \n",input_char,input_char);
    }
  }
}

void readString_IP(char * str, word my_port) {
  int i;
  int location;
  auto int retval1;
  printf("%d : %s \n", my_port, str);
  token = strtok(str, delim);

  switch ( * token) // Read Command
  {

  case 6: //Acknowledge Data Received on other end
    string_pos_ip = 0;
    //printf("Data Acknowledged \n");
    break;

  case 71: //Control Ozone Generator
    token = strtok(NULL, delim);
    ozone_power = atoi(token);
    token = strtok(NULL, delim);
    gen_alarm_count = 0;
    printf("Ozone Power Set to: %d \n", ozone_power);
    string_pos_ip = 0;
    break;

  case 72: //Control Primary Ring Parameters
    if (my_port == 1001) {
      i = 0;
    }
    if (my_port == 1002) {
      i = 1;
    }

    token = strtok(NULL, delim);
    ozone_output[i] = atoi(token); //removed !
    //digOut(2,!atoi(token));
    token = strtok(NULL, delim);
    ozone_ANA_out[i] = atof(token);
    printf("Wrote IP Channel %d \n", i);
    sprintf(message, "\x06 \r");

    if (my_port == 1001) {
      retval1 = sock_fastwrite( & sock[1], message, strlen(message));
    }

    if (my_port == 1002) {
      retval1 = sock_fastwrite( & sock[2], message, strlen(message));
    }
    printf("%d, %d, %4.2f \n", my_port, ozone_output[i], ozone_ANA_out[i]);
    string_pos_ip = 0;
    break;

  case 73: //Control Secondary Ring Parameters
    token = strtok(NULL, delim);
    digOut(3, !atoi(token));
    token = strtok(NULL, delim);
    anaOutVolts(2, atof(token));

    string_pos_ip = 0;
    break;

  case 74: // Set & Store Ozone power level
    token = strtok(NULL, delim);
    settings[1] = atof(token);
    anaOutmAmps(3, ((long) settings[1] * 0.2));
    //anaOutVolts(3, ((long)settings[1]* 0.05));
    printf("Power Level Set %ld \n", settings[1]);
    string_pos_ip = 0;
    save_data[0] = & settings;
    save_lens[0] = sizeof(settings);
    writeUserBlockArray(0, save_data, save_lens, 1);
    break;

  case 75: //Set Interlock Alarms
    token = strtok(NULL, delim);
    settings[2] = atoi(token);
    token = strtok(NULL, delim);
    settings[3] = atoi(token);
    token = strtok(NULL, delim);
    settings[4] = atoi(token);
    string_pos_ip = 0;
    save_data[0] = & settings;
    save_lens[0] = sizeof(settings);
    writeUserBlockArray(0, save_data, save_lens, 1);
    printf("Interlocks Set : %ld,%ld,%ld\n", settings[2], settings[3], settings[4]);
    break;

  case 82:
    string_pos_ip = 0;
    exit(0);
    break;

  case 84: //Set Time
    printf("Set Time \n");
    string_pos_ip = 0;
    token = strtok(NULL, delim);
    rtc.tm_mon = atoi(token); //set month
    token = strtok(NULL, delim);
    rtc.tm_mday = atoi(token); //set day
    token = strtok(NULL, delim);
    rtc.tm_year = (atoi(token) - 1900); //set year
    token = strtok(NULL, delim);
    rtc.tm_hour = atoi(token); //set hour
    token = strtok(NULL, delim);
    rtc.tm_min = atoi(token); //set minute
    token = strtok(NULL, delim);
    rtc.tm_sec = atoi(token); //set second

    tm_wr( & rtc);
    SEC_TIMER = mktime( & rtc);
    printf("Set Time \n");
    break;

  default:

  }
  string_pos_ip = 0;
}

void calc_day(void) {
  double d, m, y;
  tm_rd( & rtc);
  y = 2005;

  CalcSun(rtc.tm_year, rtc.tm_mon, rtc.tm_mday, 40.04, -88.13, -6);
  sunr = (float) riset;
  suns = (float) settm;
  ora = rtc.tm_hour + (float)(rtc.tm_min / 60.);
  printf("Sunrise: %f,  Sunset:  %f,  CurrentTime:  %f \n", sunr, suns, ora);

}
