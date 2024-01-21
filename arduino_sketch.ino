#include "WisLTEBG96Serial.h"
#include "SoftwareSerial.h"
//#include "string.h"
#define withinRange true
#define debug false

SoftwareSerial Serial1(9, 10); // RX, TX

#define DSerial Serial1
#define ATSerial Serial

//#define AT_TX_PIN  11
//#define AT_RX_PIN  10
//SoftwareSerial  DSerial(AT_RX_PIN, AT_TX_PIN);

WisLTEBG96Serial WisLTE(ATSerial, DSerial);

void setup() {
  // put your setup code here, to run once:
  //Serial.begin(115200);
  //while(!Serial){}
  DSerial.begin(115200);
    while(DSerial.read() >= 0);
    DSerial.println("This is the WisLTE Debug Serial!");
    delay(1000);
    
    ATSerial.begin(115200);
}

bool nextCommand = true;
const int coordenateListMaxSize = 1;
const char * coordenateList[coordenateListMaxSize];
int coordenateListCount = 0;
String inData;
bool gotNumber = false;
String numberId;
bool sendingData = false;
const char *commands[]={
  "ATE0",
  "ATI",
  "AT+CGMR",
  "AT+CGDCONT=1,\"IP\",\"nbiotshared.prtop\"",
  "AT+COPS=1,2,\"26803\",9",
  "AT+QCFG=\"band\",0,0,80000,1",
  "AT+QCFG=\"nwscanseq\",030102,1",
  "AT+QCFG=\"nwscanmode\",3,1",
  "AT+QCFG=\"iotopmode\",1,1",
  "AT+QCFG=\"servicedomain\",1,1",
  "AT+QCFG=\"nb1/bandprior\",14",
  "AT+QCFG=\"nbsibscramble\",0",
  "AT+QNWINFO",
  "AT+QCSQ",
  "AT+CSQ",
  "AT+CGACT?",
  "AT+CGPADDR",
  "AT+QIACT?",
  "AT+QICLOSE=0",
  "AT+QIOPEN=1,0,\"UDP\",\"10.0.100.2\",10000,10000,1",
  "AT+QGPSCFG=\"nmeasrc\",1",
  "AT+QGPSXTRA=1"
  };
const int commandSize = 22;
int currentCommandIndex = 0;

char* barray2hexstr (const char* data, size_t datalen) {
  size_t final_len = datalen * 2;
  char* chrs = (char *) malloc((final_len + 1) * sizeof(*chrs));
  unsigned int j = 0;
  for(j = 0; j<datalen; j++) {
    chrs[2*j] = (data[j]>>4)+48;
    chrs[2*j+1] = (data[j]&15)+48;
    if (chrs[2*j]>57) chrs[2*j]+=7;
    if (chrs[2*j+1]>57) chrs[2*j+1]+=7;
  }
  chrs[2*j]='\0';
  return chrs;
}

void sendData(){
  String dataToSend = "{"+numberId+ "\"coordinates\":[";
  for(int x = 0; x < coordenateListMaxSize;x++){
    dataToSend+=coordenateList[x];
  }
  dataToSend+="]}";

  Serial.println(dataToSend);

  if(debug == true){
    Serial.println(dataToSend);
    Serial.println(strlen(dataToSend.c_str()));
  }

  char *hexToSend = barray2hexstr(dataToSend.c_str(),strlen(dataToSend.c_str()));
  if(debug == true){
    Serial.println(hexToSend);
    Serial.println(strlen(hexToSend));
  }
  if(withinRange == true){
    Serial.write("AT+QISENDEX=0,\"");
    Serial.write(hexToSend);
    Serial.write("\"\r\n");
  }
  free(hexToSend);
  sendingData = false;
}

void addToList(const char * coordenate){
  if(debug == true){
    Serial.println(coordenate);
    Serial.println(coordenateListCount);
  }
  coordenateList[coordenateListCount] = coordenate;
  coordenateListCount++;
  if(coordenateListCount > coordenateListMaxSize -1){
    sendingData = true;
    sendData();
    for(int x = 0; x <  coordenateListMaxSize; x++){
        coordenateList[x] = "";
    }
    coordenateListCount = 0;
  }
}

void loop() {
  
  // put your main code here, to run repeatedly:
  while(Serial.available()>0){
    char received = Serial.read();
    inData += received;
    if(received == '\n'){
      if(debug==true){
        Serial.println(inData);
      }
      if(strstr(inData.c_str(),"+QGPSLOC: ") && sendingData == false){
        inData.remove(inData.length()-2,2);
        inData = "{\"coordinate\":\""+inData;
        inData = inData + "\"}";
        addToList(inData.c_str());
        //+QGPSLOC: 223224.0,3877.067874N,00909.715171W,0.8,56.0,2,174.14,0.0,0.0,310818,09
      }else if(strstr(inData.c_str(),"+QGPSGNMEA: ")){
        inData.remove(inData.length()-2,2);
        inData = "{\"coordinate\":\""+inData;
        inData = inData +"\"}";
        addToList(inData.c_str());
        //+QGPSGNMEA: $GPGGA,122136.00,4127.586860,N,00838.474976,W,1,0
      }
      else if(gotNumber == false && (currentCommandIndex == commandSize)){
        if(strlen(inData.c_str()) > 8){
          inData.remove(inData.length()-2,2);
          numberId = "\"numberId\":\""+inData;
          numberId = numberId +"\",";
          gotNumber = true;
          if(debug==true){
            Serial.println(numberId);
          }
        }
      }
      inData = "";  
    }
   }
   
   if(currentCommandIndex < commandSize){
      if(withinRange == true){
        delay(100);
        Serial.write(commands[currentCommandIndex]);
        Serial.write("\r\n");
        delay(250);
      }
      currentCommandIndex++;
      if(currentCommandIndex == commandSize){
        delay(10000); 
      }
    }
    else if(gotNumber == false){
      Serial.write("AT+CIMI\r\n");
      delay(5000);
    }
    else{
      if(withinRange == true){
        delay(500);
        Serial.write("AT+QGPSGNMEA=\"GGA\"\r\n");
        delay(10000);
      }
      if(debug==true){
        delay(1000);
        addToList("+QGPSLOC: 223224.0,3877.067874N,00909.715171W,0.8,56.0,2,174.14,0.0,0.0,310818,09");
      }
    }
   //Serial.write("AT+QPING=1,\"10.0.100.2\",5,5\r\n");
   //WisLTE.AT_bypass();
}
