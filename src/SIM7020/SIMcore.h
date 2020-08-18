#ifndef __SIMCORE_H__
#define __SIMCORE_H__

#include <Arduino.h>
#include <stdint.h>
#include <Wire.h>
#include <stdio.h>
#include <SoftwareSerial.h>
#include <HardwareSerial.h>
#include <avr/pgmspace.h>

const char AT_CMD_OPEN_SOCKET[]        PROGMEM = "AT+CSOC=1,1,1\r\n"; //0
const char AT_CMD_CSOCON_PREFIX[]      PROGMEM = "AT+CSOCON="; // 1
const char AT_CMD_CSOSEND_PREFIX[]     PROGMEM = "AT+CSOSEND="; // 2
const char AT_CMD_CSOCL_PREFIX[]       PROGMEM = "AT+CSOCL="; // 3
const char AT_CMD_CSNTPSTART_GOOGLE[]  PROGMEM = "AT+CSNTPSTART=\"time.google.com\"\r\n"; // 4
const char AT_CMD_CSNTPSTART_TW[]      PROGMEM = "AT+CSNTPSTART=\"tw.pool.ntp.org\"\r\n"; // 5
const char AT_CMD_CSNTPSTOP[]          PROGMEM = "AT+CSNTPSTOP\r\n"; // 6
const char AT_CMD_CCLK[]               PROGMEM = "AT+CCLK?\r\n"; // 7
const char AT_CMD_CPIN_Q[]             PROGMEM = "AT+CPIN?\r\n"; // 8
const char AT_CMD_CSQ[]                PROGMEM = "AT+CSQ\r\n"; // 9
const char AT_CMD_CEREG_1[]            PROGMEM = "AT+CEREG=1\r\n"; // 10
const char AT_CMD_CEREG_Q[]            PROGMEM = "AT+CEREG?\r\n"; // 11
const char AT_CMD_AT[]                 PROGMEM = "AT\r\n"; // 12
const char AT_RES_CEREG_1[]            PROGMEM = "+CEREG: 1"; // 13
const char AT_RES_CPIN[]               PROGMEM = "READY"; // 14
const char AT_RES_CSQ[]                PROGMEM = "+CSQ:"; // 15
const char AT_RES_CSOC[]               PROGMEM = "+CSOC: "; // 16
const char AT_RES_CSONMI[]             PROGMEM = "+CSONMI"; // 17
const char AT_RES_CSOERR[]             PROGMEM = "+CSOERR"; //18 
const char AT_CMD_CFUN_0[]             PROGMEM = "AT+CFUN=0\r\n";  // 19
const char AT_CMD_CFUN_1[]             PROGMEM = "AT+CFUN=1\r\n";  // 20
const char AT_CMD_MCGDEFCONT_CHT_NB[]  PROGMEM = "AT*MCGDEFCONT=\"IP\",\"internet.iot\"\r\n";  // 21
//const char AT_CMD_MCGDEFCONT_CHT_NB[]  PROGMEM = "AT*MCGDEFCONT=\"IP\",\"NBIOT\"\r\n";  // 21
const char AT_CMD_CBAND_8_28[]         PROGMEM = "AT+CBAND=8\r\n";  // 22
//const char AT_CMD_CBAND_8_28[]         PROGMEM = "AT+CBAND=28\r\n";  // 22
const char AT_CMD_CGREG_1[]            PROGMEM = "AT+CGREG=1\r\n";  // 23
const char AT_RES_CHTTPCREATE[]         PROGMEM = "+CHTTPCREATE: ";  // 24
const char AT_CMD_CHTTPDISCON_PREFIX[] PROGMEM = "AT+CHTTPDISCON="; // 25
const char AT_CMD_CHTTPDESTROY_PREFIX[] PROGMEM = "AT+CHTTPDESTROY="; // 26
const char AT_CMD_CHTTPCON_PREFIX[]    PROGMEM = "AT+CHTTPCON="; // 27
const char AT_CMD_CHTTPSEND_PREFIX[]   PROGMEM = "AT+CHTTPSEND="; // 28
const char AT_CMD_CHTTPCREATE_PREFIX[] PROGMEM = "AT+CHTTPCREATE="; // 29
const char AT_CMD_POWER_DOWN[]         PROGMEM = "AT+CPOWD=1\r\n"; // 30
const char* const ATTable[] PROGMEM = {
    AT_CMD_OPEN_SOCKET,
    AT_CMD_CSOCON_PREFIX,
    AT_CMD_CSOSEND_PREFIX,
    AT_CMD_CSOCL_PREFIX,
    AT_CMD_CSNTPSTART_GOOGLE,
    AT_CMD_CSNTPSTOP,
    AT_CMD_CCLK,
    AT_CMD_CSNTPSTART_TW,
    AT_CMD_CPIN_Q,
    AT_CMD_CSQ,
    AT_CMD_CEREG_1,
    AT_CMD_CEREG_Q,
    AT_CMD_AT,
    AT_RES_CEREG_1,
    AT_RES_CPIN,
    AT_RES_CSQ,
    AT_RES_CSOC,
    AT_RES_CSONMI,
    AT_RES_CSOERR,
    AT_CMD_CFUN_0,
    AT_CMD_CFUN_1,
    AT_CMD_MCGDEFCONT_CHT_NB,
    AT_CMD_CBAND_8_28,
    AT_CMD_CGREG_1,
    AT_RES_CHTTPCREATE,
    AT_CMD_CHTTPDISCON_PREFIX,
    AT_CMD_CHTTPDESTROY_PREFIX,
    AT_CMD_CHTTPCON_PREFIX,
    AT_CMD_CHTTPSEND_PREFIX,
    AT_CMD_CHTTPCREATE_PREFIX,
    AT_CMD_POWER_DOWN
};

#define BUFFER_DEBUG
#define DEFAULT_TIMEOUT                 5    //seconds
#define DEFAULT_SOCKET_TIMEOUT         60    //seconds
#define DEFAULT_INTERCHAR_TIMEOUT    1500    //miliseconds
#define BUFFER_SIZE                    75
#define CMD_BUFFER_SIZE                210
#define AT_RESP_OK                  "OK\r\n"
#define AT_RESP_POWOFF              "NORMAL POWER DOWN\r\n"

class SIMcore
{
public:
    SIMcore(SoftwareSerial *ss, uint8_t pin_PWK, uint8_t pin_SLEEP);
    bool                   init(void);
    int                    checkSignalQuality();
    void                   checkNetwork(const uint8_t retry_count_max);
    void                   sendStrBySocket(const char* ip, const char* port, const char* data, bool newSocket=true, unsigned int timeout=DEFAULT_SOCKET_TIMEOUT);
    void                   getCCID(String &ccid);
    void                   getCCLK(char* timeStr);
    bool                   syncNTP();
    char*                  getAck();
    bool                   check_send_cmd(const char* cmd, const char* resp, uint8_t timeout=DEFAULT_TIMEOUT);
    void                   send_cmd(const char* cmd);
    int                    checkReadable(void);
    void                   sendCmd(const char* cmd);
    void                   send_buff(const char* buff,size_t num);
    void                   send_String(String buff);
    String                 get_String(String buff);
    void                   cleanBuffer(char *buffer,uint8_t count);
    void                   flushSerial(void);
    bool                   turnOFF(void);
    bool                   isNetworkReady();
    bool                   isUploadCompleted();
    bool                   isServerOK();
    bool                   httpGet(const char* server, const char* path);
private:
    void                   begin(long rate=19200);
    int                    readBuffer(char *buffer, int count=BUFFER_SIZE, bool cont=false, uint8_t timeout = DEFAULT_TIMEOUT, unsigned int chartimeout = DEFAULT_INTERCHAR_TIMEOUT);
    bool                   openSocket();
    bool                   closeSocket();
    bool                   httpCreate(const char* server);
    bool                   httpSend(const char* path);
    bool                   httpConnect();
    bool                   httpDisconnect();
    bool                   httpDestroy(uint8_t id);
    bool                   connectServer(const char* ip, const char* port);
    void                   setCSOSENDHex(const char* inputChar);
    byte                   _state=0x0;
    SoftwareSerial*        _serial;
    uint8_t                _pinPWK;
    uint8_t                _pinSLEEP;
    char                   _buffer[BUFFER_SIZE];
    //char                   _buffer2[20];
    int8_t                _sid;
};

//extern   SIMcore   SIMcore;

#endif
