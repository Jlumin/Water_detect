#include "SIMcore.h"

char cmdBuf[CMD_BUFFER_SIZE];
char pgmBuf[16];
//static SoftwareSerial* _serial;
SIMcore::SIMcore(SoftwareSerial *ss, uint8_t pin_PWK, uint8_t pin_SLEEP) {
    _serial = ss;
    _pinPWK = pin_PWK;
    _pinSLEEP = pin_SLEEP;
    pinMode(pin_PWK, OUTPUT);
    pinMode(pin_SLEEP, OUTPUT);
    digitalWrite(_pinSLEEP, LOW);
    digitalWrite(_pinPWK, HIGH);
}

void SIMcore::begin(long rate) {    
    _serial->begin(rate);
}

// _state
// 0 power on/off
// 1 AT command ok
// 2 SIM card ok
// 3 Signal ok
// 4 Network attach ok
// 5 socket
// 6 server
// 7 upload


bool SIMcore::init(void)
{
    uint8_t count = 0;
    digitalWrite(_pinSLEEP, HIGH);
    //digitalWrite(_pinSLEEP, LOW);
    digitalWrite(_pinPWK, HIGH);
    delay(2000);
    digitalWrite(_pinPWK, LOW);
    delay(1000);
    begin();
    // pgm_read_word from avr/pgmspace.h
    strcpy_P(cmdBuf, (char*) pgm_read_word(&(ATTable[12])));  // "AT\r\n"
    while(count < 3){
        if(check_send_cmd(cmdBuf, AT_RESP_OK)){
            _state = 0b00000011;
            break;
        }else{
            count++;
            delay(300);
        }
    }
    if(count == 3){
        _state = 0b00000001;
        return false;
    }
    count = 0;
    strcpy_P(cmdBuf, (char*) pgm_read_word(&(ATTable[8])));  // "AT+CPIN?\r\n"
    strcpy_P(pgmBuf, (char*) pgm_read_word(&(ATTable[14])));  // "READY"
    while(count < 3){
        if(check_send_cmd(cmdBuf, pgmBuf)){
            break;
        }else{
            count++;
            delay(300);
        }
    }
    if(count == 3){
        _state = 0b00000011;
        return false;
    }

    strcpy_P(cmdBuf, (char*) pgm_read_word(&(ATTable[19])));  // "AT+CFUN=0\r\n"
    check_send_cmd(cmdBuf, AT_RESP_OK);


    strcpy_P(cmdBuf, (char*) pgm_read_word(&(ATTable[21])));  // "AT*MCGDEFCONT=\"IP\",\"internet.iot\"\r\n"
    check_send_cmd(cmdBuf, AT_RESP_OK);

    strcpy_P(cmdBuf, (char*) pgm_read_word(&(ATTable[20])));  // "AT+CFUN=1\r\n"
    check_send_cmd(cmdBuf, AT_RESP_OK);
    //delay(2000);

    strcpy_P(cmdBuf, (char*) pgm_read_word(&(ATTable[22])));  // "AT+CBAND=8,28\r\n"
    check_send_cmd(cmdBuf, AT_RESP_OK);

    strcpy_P(cmdBuf, (char*) pgm_read_word(&(ATTable[23])));  // "AT+CGREG=1\r\n"
    check_send_cmd(cmdBuf, AT_RESP_OK);
    // TODO add into config
    //check_send_cmd("AT+CSCLK=1\r\n", AT_RESP_OK);
    //check_send_cmd("AT+CSOSENDFLAG=0\r\n", AT_RESP_OK);
    //check_send_cmd("AT+CSORCVFLAG=1\r\n", AT_RESP_OK);
    _state = 0b00000111;
    return true;
}

bool  SIMcore::turnOFF(void)
{
    //closeSocket();
    //digitalWrite(_pinSLEEP, HIGH);
    //digitalWrite(_pinSLEEP, LOW);
    strcpy_P(cmdBuf, (char*) pgm_read_word(&(ATTable[30]))); // AT+CPOWD=1  // Modify the procedure
    check_send_cmd(cmdBuf, AT_RESP_POWOFF);
    delay(500);

    digitalWrite(_pinPWK, HIGH);
    delay(500);

    _state = 0b00000000;
    Serial.println(F("Info:  SIM7020 power down"));
    cleanBuffer(_buffer, BUFFER_SIZE);
    readBuffer(_buffer, BUFFER_SIZE-1);
    //_buffer[len++] = '\0';
    //Serial.println(_buffer);
    return true;
}


bool SIMcore::check_send_cmd(const char* cmd, const char* resp, uint8_t timeout)
{
    Serial.print(F("==> check_send_cmd cmd: "));
    Serial.println(cmd);

    sendCmd(cmd);
    int idx;
    bool cont = false;
    while (1) {
        Serial.println(F("==> check_send_cmd cmd: while"));
	    cleanBuffer(_buffer, BUFFER_SIZE);
        idx = readBuffer(_buffer, BUFFER_SIZE-1, cont, timeout);
	    cont = true;
        Serial.print(F("==> check_send_cmd while, idx = "));
        Serial.println(idx);
        if (_buffer[idx-1] == '\n') {
            break;
	    }
	    if (idx == 0) {
            return false;
	    }
	// TODO, retry break
    }

    Serial.println(F("==> check_send_cmd resp:"));
    Serial.println(_buffer);
    #ifdef BUFFER_DEBUG
    Serial.print(F("==> index = "));
    Serial.println(idx);
    #endif
    if (NULL != strstr(_buffer, resp)){
        return  true;
    } else {
        return  false;
    }
}

void SIMcore::send_cmd(const char* cmd)
{
    sendCmd(cmd);
    _serial->flush();
}

void SIMcore::flushSerial(void)
{
    _serial->flush();
}

int SIMcore::checkReadable(void)
{
    return _serial->available();
}

void SIMcore::sendCmd(const char* cmd)
{
    _serial->write(cmd);
    flushSerial();
}

void SIMcore::send_buff(const char* buff,size_t num)
{
    _serial->write(buff,num);
}

void SIMcore::send_String(String buff)
{
    _serial->print(buff);
}

String SIMcore::get_String(String buff)
{
    uint64_t t1 = millis();
    while(1){
        while(checkReadable()){
            Serial.print(char(_serial->read()));
            t1 = millis();
        }
        //Serial.println();
        if((millis() - t1) > 10000){
            return buff;
        }
    }
}

void SIMcore::cleanBuffer(char *buffer,uint8_t count)
{
    memset(buffer, 0, count);
}

int  SIMcore::readBuffer(char *buffer, int count, bool cont, uint8_t timeout, unsigned int chartimeout)
{
    int i = 0;
    char ch;
    bool nlFlag = false;
    bool bufferDebug = true;
    unsigned long timerStart, prevChar, charDiff;
    timerStart = millis();
    prevChar = 0;
    
    if (bufferDebug) {
        Serial.print(F("timeout "));
        Serial.println(timeout);
        Serial.print(F("chartimeout "));
        Serial.println(chartimeout);
    }

    while (1){
        while (checkReadable()){
            if (bufferDebug && nlFlag) {
	        Serial.print(millis());  Serial.print(F("\t"));  Serial.print(i);
            }
            ch = _serial->read();
	    if (nlFlag || cont) {
            	buffer[i++] = ch;
	    }

	    if (ch == '\r') {
                if (bufferDebug && nlFlag) {
		    Serial.println(F("\t<CR>"));
		}
	    }
	    else if (ch == '\n') {
                if (bufferDebug && nlFlag) {
		    Serial.println(F("\t<NL>"));
		}
		nlFlag = true;
	    }
	    else {
                if (bufferDebug && nlFlag) {
		    Serial.print(F("\t"));
		    Serial.println(ch);
		}
	    }

            prevChar = millis();
            if (i >= count) {
                return i;
            }
	    //delay(10);
        } // check readable
	
        if(timeout){
            if((unsigned long) (millis() - timerStart) > timeout*1000){
		Serial.println(F("timeout"));
                break;
            }
        }
	
	charDiff = millis() - prevChar;
        if( (charDiff > chartimeout*10) && (prevChar != 0)){
	    Serial.print(F("char diff:\t"));
	    Serial.println(charDiff);
	    Serial.println(F("char timeout"));
            break;
        }
    }
    return i;
}

int SIMcore::checkSignalQuality() {
    int i = 0,j = 0,k = 0;
    char *signalQuality;
    cleanBuffer(_buffer, BUFFER_SIZE);
    strcpy_P(cmdBuf, (char*) pgm_read_word(&(ATTable[9])));  // "AT+CSQ\r\n"
    send_cmd(cmdBuf);
    readBuffer(_buffer, BUFFER_SIZE-1);
    strcpy_P(pgmBuf, (char*) pgm_read_word(&(ATTable[15])));  // "+CSQ:"
    if (NULL != (signalQuality = strstr(_buffer, pgmBuf))){
        i = *(signalQuality + 6) - 48;
        j = *(signalQuality + 7) - 48;
        k =  (i * 10) + j;
    }else{
        return 0;
    }
    if( k == 99){
        return 0;
    }else{
        return k;
    }
}

void SIMcore::checkNetwork(const uint8_t retry_count_max) {
    uint8_t retry_count = 0;
    // Modify the procedure
    while (1) {
        int csq = checkSignalQuality();
        Serial.print(F("CSQ: "));
        Serial.println(csq);
        Serial.print(F("Count Number: "));
        Serial.println(retry_count + 1);
        if (csq > 8 && csq != 99) {
            _state = 0b00010000; // We have good signal.
            break;
	    }
        delay(2000);
        if (retry_count >= retry_count_max) {
            // TODO try reboot SIM7020
            _state = 0b0001000;  // We have bad signal.              
            break;
        }
	    retry_count++;
    }
    //_state |= 0b00001000; // DEC 8
    //_state |= 0b00010000; // DEC 16
/*
    strcpy_P(cmdBuf, (char*) pgm_read_word(&(ATTable[10])));  // "AT+CEREG=1\r\n"
    check_send_cmd(cmdBuf, AT_RESP_OK);
    retry_count = 0;
    strcpy_P(cmdBuf, (char*) pgm_read_word(&(ATTable[11])));  // "AT+CEREG?\r\n"
    strcpy_P(pgmBuf, (char*) pgm_read_word(&(ATTable[13])));  // "+CEREG: 1"
    while (1) {
        //if (check_send_cmd(cmdBuf, "+CEREG: 1"))
        if (check_send_cmd(cmdBuf, pgmBuf)) {
            _state |= 0b00010000;
            break;
        }
	if (retry_count >= retry_count_max) {
            break;
	}
        delay(1000);
	retry_count++;
	Serial.print(F("SIM7000 checkNetwork\t"));
	Serial.println(retry_count);
    }
*/
}


bool SIMcore::openSocket() {
    uint8_t retry_count = 0;
    char foo[2];
    //closeSocket();
    strcpy_P(cmdBuf, (char*) pgm_read_word(&(ATTable[0])));
    strcpy_P(pgmBuf, (char*) pgm_read_word(&(ATTable[16])));  // "+CSOC: "
    while (1) {
        //if (check_send_cmd(cmdBuf, "+CSOC: ", DEFAULT_TIMEOUT))
        if (check_send_cmd(cmdBuf, pgmBuf, DEFAULT_TIMEOUT)) {
	    foo[0] = _buffer[7];
	    foo[1] = '\0';
            _sid = atoi(foo);
	    Serial.print(F("debug:  _buffer = "));
            Serial.println(_buffer);
	    Serial.print(F("debug:  sid = "));
            Serial.println(_sid);
            _state |= 0b00100000;
	    break;
        }
	if (retry_count >= 10) {
            _state = 0b00011111;
	    return false;
	}
	retry_count++;
	delay(2000);
    }
    return true;
}

bool SIMcore::closeSocket() {
    strcpy_P(pgmBuf, (char*) pgm_read_word(&(ATTable[3])));  // AT+CSOCL=
    sprintf(cmdBuf, "%s%d\r\n", pgmBuf, _sid);
    check_send_cmd(cmdBuf, AT_RESP_OK);
    _state = 0b00011111;
}

bool SIMcore::httpCreate(const char *server) {
    uint8_t retry_count = 0;
    char foo[2];
    httpDestroy(0);
    cleanBuffer(cmdBuf, CMD_BUFFER_SIZE);
    strcpy_P(pgmBuf, (char*) pgm_read_word(&(ATTable[29])));   // AT+CHTTPCREATE=
    sprintf(cmdBuf, "%s\"%s\"\r\n", pgmBuf, server);

    //strcpy_P(pgmBuf, (char*) pgm_read_word(&(ATTable[24])));  // "+CHTTPCREATE: "
    while (1) {
        if (check_send_cmd(cmdBuf, "+CHTTPCREATE: ", DEFAULT_SOCKET_TIMEOUT)) {
        // 14th char means returned token ID
	// For example "+CHTTPCREATE: 0"
	//              0123 ...      ^ 14th
	//
	    foo[0] = _buffer[14];        
	    foo[1] = '\0';
            _sid = atoi(foo);
	    Serial.print(F("debug:  _buffer = "));
            Serial.println(_buffer);
	    Serial.print(F("debug:  sid = "));
            Serial.println(_sid);
            _state |= 0b00100000;
	    break;
        }
	if (retry_count >= 5) { // modify retry_count 20 to 5
            _state = 0b00011111;
	    return false;
	}
	retry_count++;
	delay(2000);
    }
    return true;
}

bool SIMcore::httpConnect() {
    strcpy_P(pgmBuf, (char*) pgm_read_word(&(ATTable[27])));  // AT+CHTTPCON=
    cleanBuffer(cmdBuf, CMD_BUFFER_SIZE);
    sprintf(cmdBuf, "%s%d\r\n", pgmBuf, _sid);
    if (check_send_cmd(cmdBuf, AT_RESP_OK, 60)) {
	// TODO state update
	Serial.println(F("Info:  http server connected"));
        _state |= 0b01000000;
	return true;
    }
    _state = 0b00111111;
    return false;
}

bool SIMcore::httpSend(const char* path) {
    strcpy_P(pgmBuf, (char*) pgm_read_word(&(ATTable[28])));  // AT+CHTTPSEND=
    cleanBuffer(cmdBuf, CMD_BUFFER_SIZE);
    sprintf(cmdBuf, "%s%d,0,\"%s\"\r\n", pgmBuf, _sid, path);

    //send_cmd(cmdBuf);
    //get_String("aaa");
    
    if (check_send_cmd(cmdBuf, AT_RESP_OK, 60)) {
	// TODO state update
	Serial.println(F("Info:  http get data sent"));
        _state |= 0b01000000;
	//return true;
    }
    
    _state = 0b00111111;
    //return false;
}

bool SIMcore::httpDisconnect() {
    strcpy_P(pgmBuf, (char*) pgm_read_word(&(ATTable[25])));  // AT+CHTTPDISCON=
    sprintf(cmdBuf, "%s%d\r\n", pgmBuf, _sid);
    check_send_cmd(cmdBuf, AT_RESP_OK);
    _state = 0b00011111;
}


bool SIMcore::httpDestroy(uint8_t id) {
    strcpy_P(pgmBuf, (char*) pgm_read_word(&(ATTable[26])));  // AT+CHTTPDESTROY=
    cleanBuffer(cmdBuf, CMD_BUFFER_SIZE);
    sprintf(cmdBuf, "%s%d\r\n", pgmBuf, id);
    check_send_cmd(cmdBuf, AT_RESP_OK);
    _state = 0b00011111;
}

bool SIMcore::connectServer(const char* ip, const char* port) {
    Serial.print(F("Info:  server: "));
    Serial.println(ip);
    Serial.print(F("Info:  port: "));
    Serial.println(port);
    strcpy_P(pgmBuf, (char*) pgm_read_word(&(ATTable[1])));  // AT+CSOCON=
    cleanBuffer(cmdBuf, CMD_BUFFER_SIZE);
    sprintf(cmdBuf, "%s%d,%s,\"%s\"\r\n", pgmBuf, _sid, port, ip);
    if (check_send_cmd(cmdBuf, AT_RESP_OK, 60)) {
	// TODO state update
	Serial.println(F("Info:  server connected"));
        _state |= 0b01000000;
	return true;
    }
    _state = 0b00111111;
    return false;
}

void SIMcore::sendStrBySocket(const char* ip, const char* port, const char* data, bool newSocket, unsigned int timeout) {
    
    // open socket
    if (newSocket) {
        if (!openSocket()) {
            return; 
        }
        // connect to server
        if (!connectServer(ip, port)) {
            return; 
	}
    }
    
    // send data
    setCSOSENDHex(data);
    if (!check_send_cmd(cmdBuf, NULL, timeout)) {
	// serial hanging
        _state = 0b01111111;
    }

    Serial.println(F("Info: after send, check buffer content"));
    Serial.println(_buffer);

    strcpy_P(pgmBuf, (char*) pgm_read_word(&(ATTable[17])));  // "+CSONMI"
    if (strstr(_buffer, pgmBuf)) {
        _state |= 0b10000000;
    }

    strcpy_P(pgmBuf, (char*) pgm_read_word(&(ATTable[18])));  // "+CSOERR"
    // +CSOERR, socket error (TODO error handling)
    if (strstr(_buffer, pgmBuf)) {
        _state = 0b01111111;
    }
    //
}

char* SIMcore::getAck() {
    return _buffer;
}

bool SIMcore::httpGet(const char* server, const char* path) {

    httpDisconnect();

    if (!httpCreate(server)) {
        //return;
        return false;
    }
    
    if (!httpConnect()) {
        //return;
        return false;
    }

    if (!httpSend(path)) {
        //return;
    }

    if (!httpDisconnect()) {
        //return;
    }

    return true;

    //if (!httpDestroy()) {
    //    return;
    //}
}

void SIMcore::getCCID(String &ccid) {
    check_send_cmd("AT+CCID\r\n", AT_RESP_OK);
}

void SIMcore::getCCLK(char* timeStr) {
    strcpy_P(cmdBuf, (char*) pgm_read_word(&(ATTable[6])));  // "AT+CCLK?\r\n"
    if (check_send_cmd(cmdBuf, "+CCLK", DEFAULT_TIMEOUT)) {
	    Serial.print(F("debug:  _buffer = "));
            Serial.println(_buffer);

            //
            //  \n+CCLK: 19/06/20,15:40:16+32
	    //
	    for (uint8_t i=0; i<17; i++) {
                timeStr[i] = _buffer[i+7];
            }
	    timeStr[17] = '\0';
    }
}

bool SIMcore::syncNTP() {
    bool NTPstate;
    strcpy_P(cmdBuf, (char*) pgm_read_word(&(ATTable[7])));  // "AT+CSNTPSTART=\"tw.pool.ntp.org\"\r\n"
    if (check_send_cmd(cmdBuf, "+CSNTP", DEFAULT_SOCKET_TIMEOUT)) {
        NTPstate = true;
        _state |= 0b00100000;
    } else {
        NTPstate = false;
    }
    strcpy_P(cmdBuf, (char*) pgm_read_word(&(ATTable[5])));  // "AT+CSNTPSTOP\r\n"
    check_send_cmd(cmdBuf, AT_RESP_OK);
    return NTPstate;
}

bool SIMcore::isNetworkReady() {
    return (_state >> 4) & 0x1;
}

bool SIMcore::isServerOK() {
    return (_state >> 6) & 0x1;
}
bool SIMcore::isUploadCompleted() {
    return (_state >> 7) & 0x1;
}

void SIMcore::setCSOSENDHex(const char* inputChar) {
    uint16_t len = strlen(inputChar);
    uint8_t *src, *dst, x, h, l;
    cleanBuffer(cmdBuf, CMD_BUFFER_SIZE);
    strcpy_P(pgmBuf, (char*) pgm_read_word(&(ATTable[2])));  // AT+CSOSEND=
    sprintf(cmdBuf, "%s%d,%d,", pgmBuf, _sid, len*2);
    src = inputChar;
    dst = cmdBuf + strlen(cmdBuf);

    for (uint16_t i=0; i<len; i++) {
	x = *src;
        h = (x >> 4) & 0xf;
        l = x & 0xf;
        dst[0] = (h < 10) ? (h + '0') : ( h - 10 + 'A'); 
        dst[1] = (l < 10) ? (l + '0') : ( l - 10 + 'A'); 
        src += 1;
        dst += 2;
    }
    *dst++ = 0x0d; // \r
    *dst++ = 0x0a; // \n
}
