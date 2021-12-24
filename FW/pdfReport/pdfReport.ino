/*
  Author : HADES
  Company : Solubiz.co.ltd
  history :
    - tinyusb : USB host save :: 파일 변경시 log 표시
    - C:\Users\dhha1\AppData\Local\Arduino15\packages\adafruit\hardware\samd\1.7.5\boards.txt :: board modify
    - C:\Users\dhha1\OneDrive\바탕 화면\문서\Arduino\libraries\Adafruit_SPIFlash\src\Adafruit_SPIFlashBase.cpp   static const SPIFlash_Device_t possible_devices[] :: nor flash list add --> [ok work]
    - ds1307 rtc : 
*/

// Feather M0 Express test
/*********************************************************************
 Adafruit invests time and resources providing this open source code,
 please support Adafruit and open-source hardware by purchasing
 products from Adafruit!

 MIT license, check LICENSE for more information
 Copyright (c) 2019 Ha Thach for Adafruit Industries
 All text above, and the splash screen below must be included in
 any redistribution
*********************************************************************/

/* This example demo how to expose on-board external Flash as USB Mass Storage.
 * Following library is required
 *   - Adafruit_SPIFlash https://github.com/adafruit/Adafruit_SPIFlash
 *   - SdFat https://github.com/adafruit/SdFat
 *
 * Note: Adafruit fork of SdFat enabled ENABLE_EXTENDED_TRANSFER_CLASS and FAT12_SUPPORT
 * in SdFatConfig.h, which is needed to run SdFat on external flash. You can use original
 * SdFat library and manually change those macros
 */

#include "SPI.h"
#include "SdFat.h"
#include "Adafruit_SPIFlash.h"
#include "Adafruit_TinyUSB.h"
#include <Adafruit_NeoPixel.h>

#if defined(FRAM_CS) && defined(FRAM_SPI)
  Adafruit_FlashTransport_SPI flashTransport(FRAM_CS, FRAM_SPI);

#elif defined(ARDUINO_ARCH_ESP32)
  // ESP32 use same flash device that store code.
  // Therefore there is no need to specify the SPI and SS
  Adafruit_FlashTransport_ESP32 flashTransport;

#else
  // On-board external flash (QSPI or SPI) macros should already
  // defined in your board variant if supported
  // - EXTERNAL_FLASH_USE_QSPI
  // - EXTERNAL_FLASH_USE_CS/EXTERNAL_FLASH_USE_SPI
  #if defined(EXTERNAL_FLASH_USE_QSPI)
    Adafruit_FlashTransport_QSPI flashTransport;

  #elif defined(EXTERNAL_FLASH_USE_SPI)
    Adafruit_FlashTransport_SPI flashTransport(EXTERNAL_FLASH_USE_CS, EXTERNAL_FLASH_USE_SPI);

  #else
    #error No QSPI/SPI flash are defined on your board variant.h !
  #endif
#endif

Adafruit_SPIFlash flash(&flashTransport);

// file system object from SdFat
FatFileSystem fatfs;

FatFile root;
FatFile file;

// USB Mass Storage object
Adafruit_USBD_MSC usb_msc;

// Set to true when PC write to flash
bool changed;

char gDataBuffer[1024] = {0x31,0x33,0x35,0x36};
int gCount = 1024;

//float tempY[] = {-20, -35, -18, -12 , 80, -28, -28, -28, -22};

float tempY[100] = {};

//float tempY[650] = {0, }; // 배열 정의, 동적 메모리 73%사용 
//uint16_t tempY_tracks = 100; // 셔플 숫자 갯수 
//unsigned long int atime; // 시작 시간, 밀리 초 
//void tempY_num() { 
//  tempY[0] = random(1, tempY_tracks+1);
//  uint16_t track_count = 1; 
//  bool duplecate = false; 
//  while (track_count < tempY_tracks) { 
//    uint16_t temp = random(1, tempY_tracks+1); 
//    for (int i = 0; i < track_count; i++) { // 중복검사 
//     if (duplecate == false) { // 중복된 값이 아니면 
//      tempY[track_count] = temp; // 값 저장 
//      track_count++; // 저장 인덱스 증가
//     }
//    }
//  }
//}

int dataLen = sizeof(tempY)/sizeof(float);


float min0 = 0;
float max0 = 0;
int flag = 0;

char Model[8] = "Solubiz";
char Firmware[5] = "V1.0";
char SerialNo[16] = "53:4F:4C:00:01";
char FileCreated[20] = "2021/09/23 03:05:32";
char RecordID[22] = "540B300161_KI24032456";
char LoggCycle[7] = "1m/20d";
char StartDelay[3] ="0s";
char CuerrStat[8] = "Stopped";
char TripNo[11] = "KI24032456";
//char MAXtempY = max0;
char MAXDate[20] = "2021/10/01 17:56:56";
//char MINtempY[7] = "-20.6";
char MINDate[20] = "2021/09/21 15:00:56";
char TripLength[10] = "19d23h59m";
int DataPoint = dataLen;
char AlarmSetting[18] = "No Alarm Setting";
char MKTtempY[5] = "21.2";
char AVGtempY[5] = "10.0";

int startTime = 0;
int stopTime = 0;

int incomingByte;

// the setup function runs once when you press reset or power the board
void setup()
{

  pinMode(LED_BUILTIN, OUTPUT);

  flash.begin();

  // Set disk vendor id, product id and revision with string up to 8, 16, 4 characters respectively
  usb_msc.setID("Adafruit", "External Flash", "1.0");

  // Set callback
  usb_msc.setReadWriteCallback(msc_read_cb, msc_write_cb, msc_flush_cb);

  // Set disk size, block size should be 512 regardless of spi flash page size
  usb_msc.setCapacity(flash.size()/512, 512);

  // MSC is ready for read/write
  usb_msc.setUnitReady(true);
  
  usb_msc.begin();

  // Init file system on the flash
  fatfs.begin(&flash);

  Serial.begin(115200);
  while ( !Serial ) delay(10);   // wait for native usb

  delay(1000);


  
  for (int mT = 0; mT < dataLen; mT++) {
    min0 = min(min0, tempY[mT]);
    max0 = max(max0, tempY[mT]);
   }

  startTime = 0;
  stopTime = DataPoint;
  
  Serial.println();
  Serial.print("max0: ");
  Serial.println(max0);
  Serial.print("min0: ");
  Serial.println(min0);
  Serial.print("DataPoint: ");
  Serial.println(DataPoint);
  Serial.println("Adafruit TinyUSB Mass Storage External Flash example");
  //Serial.print("FLASH CHIP: "); Serial.println(EXTERNAL_FLASH_DEVICES);
  Serial.print("JEDEC ID: "); Serial.println(flash.getJEDECID(), HEX);
  Serial.print("Flash size: "); Serial.println(flash.size());

  changed = false; // to print contents initially
  makefile();  //file save test

}

#define FILE_BASE_NAME "Solubiz.pdf"

// _printf -> file.write %d, %s, %.2f 
void _printf(const char *s,...){
    va_list args;
    va_start(args, s);
    int n = vsnprintf(NULL, 0, s, args);
    char *str = new char[n+1];
    vsprintf(str, s, args);
    va_end(args);
    file.write(str);
    delete [] str;
}

// Write data header.
void writeHeader() {

  // gheo write - 2021-12-03
  // PDF Set Header
  file.write("%PDF-1.7 \n\n1 0 obj\n<<\n  /Pages 2 0 R\n>>\nendobj\n\n2 0 obj\n<<\n  /Type /Pages\n  /Count 1\n  /Kids [3 0 R]\n>>\nendobj\n3 0 obj\n");
  file.write("<<\n  /Type /Page\n  /Contents 4 0 R\n  /Parent 2 0 R\n  /Resources <<\n      /Font <<\n         /F1 <<\n            /Type /Font\n            /Subtype /Type1\n            /BaseFont /Arial\n         >>\n      >>\n   >>\n>>\nendobj\n\n");

  // DATA Format
  file.write("\n4 0 obj\n<</Length 43391>>stream\n0.8 w\n0 0 0 RG\n[] 0 d\n85 753 m 567 753 l h S\n0.5 w\n[] 0 d\n85 710 m 567 710 l h S\n85 606 m 217 606 l h S\n242 606 m 567 606 l h S\n85 511 m 567 511 l h S\n28 753 m 71 753 l h S\n28 661 m 71 661 l h S\n28 661 m 71 661 l h S\n");
  file.write("BT\n0.5 w\n1 0 0 rg\n[] 0 d\n");
  file.write("472.9 798 m 472.9 811 l 484.8 811 l 484.8 808.6 l 475.7 808.6 l 475.7 805.8 l 481.8 805.8 l 481.8 803.5 l 475.7 803.5 l 475.7 800.4 l 485.1 800.4 l 485.1 798 l f\n");
  file.write("506.6 798 m 506.6 811.1 l 515 811.1 l 514 808.7 l 509.4 808.7 l 509.4 805.7 l 514 805.7 l 515 803.4 l 509.4 803.4 l 509.4 798 l f\n");
  file.write("515 811.1 m 515.8 811 l 516.7 810.8 l 517.5 810.5 l 518 810.2 l 518.5 809.8 l 518.8 809.3 l 519 808.9 l 519.1 808.6 l 519.3 807.7 l 519.3 806.5 l 519.2 806.1 l 519 805.5 l 518.8 805.1 l 518.6 804.8 l 518.2 804.4 l 517.8 804.1 l 517.4 803.9 l 516.6 803.6 l 516.1 803.5 l 515 803.4 l 514 805.7 l 515 805.8 l 515.8 806 l 516.1 806.2 l 516.3 806.4 l 516.5 807 l 516.5 807.4 l 516.4 807.7 l 516.3 808 l 515.9 808.3 l 515.5 808.5 l 514.8 808.6 l 514 808.7 l f\n");
  file.write("522 802.7 m 522.9 802.1 l 524.4 801.4 l 526.2 800.8 l 527.7 800.5 l 529 800.5 l 529.9 800.6 l 531.1 801 l 532.1 801.7 l 532.5 802.6 l 532.5 803 l 532.3 803.4 l 531.6 803.8 l 530.5 804.1 l 526 804.6 l 523.9 805.1 l 522.6 805.8 l 521.9 806.7 l 522 807.5 l 521.2 808.6 l 521.3 810.3 l 521.8 812 l 522.6 812.5 l 523.5 813.2 l 524.5 813.7 l 525.7 814 l 527.1 814.2 l 528.4 814.2 l 529.4 814.1 l 531 813.8 l 533.3 812.9 l 535.1 811.8 l 533.7 809.3 l 531.7 810.6 l 530.3 811.1 l 528.6 811.4 l 527.3 811.4 l 526.3 811.3 l 525.1 810.9 l 524.5 810.4 l 524.1 809.7 l 524.1 809.2 l 524.6 808.6 l 524.9 808.4 l 526.2 808.1 l 528.1 807.9 l 531.3 807.5 l 532.9 807 l 534 806.3 l 535 805.2 l 535.5 803.7 l 535.5 802.3 l 535.4 801.8 l 535.1 800.9 l 534.6 800 l 534 799.4 l 533.2 798.8 l 532 798.3 l 529.6 797.7 l 527.6 797.7 l 526 798 l 524.6 798.4 l 522.9 799 l 520.9 800 l f\n");
  file.write("538.3 798 m 538.3 811 l 550.2 811 l 550.2 808.6 l 541.1 808.6 l 541.1 805.8 l 547.2 805.8 l 547.2 803.5 l 541.1 803.5 l 541.1 800.4 l 550.5 800.4 l 550.5 798 l f\n");
  file.write("553.1 798 m 553.1 811 l 555.6 811 l 564.2 802.8 l 564.2 811 l 566.7 811 l 567.1 798 l 564.5 798 l 555.6 806.5 l 555.6 798 l f\n");
  file.write("ET\n");
  file.write("0.5 0.5 0.5 rg\nBT\n/F1 7 Tf\n28 690 Td\n(UTC +09:00)Tj\nET\n");
  file.write("0 0 0 rg\nBT\n/F2 10 Tf\n85 715 Td\n(Device)Tj\nET\n");
  file.write("BT\n/F2 8 Tf\n85 697 Td\n(Model:)Tj\nET\n");
  file.write("BT\n/F1 8 Tf\n115 697 Td\n");
  _printf("(%s)Tj\n", Model);
  file.write("ET\n");
  file.write("BT\n/F2 8 Tf\n85 668 Td\n(Firmware:)Tj\nET\n");
  file.write("BT\n/F1 8 Tf\n125 668 Td\n");
  _printf("(%s)Tj\n", Firmware);
  file.write("ET\n");
  file.write("BT\n/F2 8 Tf\n85 682 Td\n(Serial No.:)Tj\nET\n");
  file.write("BT\n/F1 8 Tf\n128 682 Td\n");
  _printf("(%s)Tj\n", SerialNo);
  file.write("ET\n");
  file.write("0.5 0.5 0.5 rg\nBT\n/F1 5 Tf\n28 15 Td\n(ERR 0004.0)Tj\nET\n");
  file.write("BT\n/F1 8 Tf\n85 798 Td\n");
  _printf("(File created@%s)Tj\n", FileCreated);
  file.write("ET\n");
  file.write("BT\n/F1 8 Tf\n85 808 Td\n");
  _printf("(Record ID:%s)Tj\n", RecordID);
  file.write("ET\n");
  file.write("0 0 0 rg\nBT\n/F2 8 Tf\n242 682 Td\n(Start Mode:)Tj\nET\n");
  file.write("\nBT\n/F1 8 Tf\n\n288 682 Td\n(Manual Start[Repeat])Tj\nET\n");
  file.write("BT\n/F2 8 Tf\n242 697 Td\n(Logging Interval/Cycle:)Tj\nET\n");
  file.write("BT\n/F1 8 Tf\n332 697 Td\n");
  _printf("(%s)Tj\n", LoggCycle);
  file.write("ET\n");
  
  file.write("BT\n/F2 8 Tf\n242 668 Td\n(Start Delay:)Tj\nET\n");
  file.write("BT\n/F1 8 Tf\n288 668 Td\n");
  _printf("(%s)Tj\n", StartDelay);
  file.write("ET\n");
  file.write("BT\n/F2 8 Tf\n85 654 Td\n(Current Status:)Tj\nET\n");
  file.write("BT\n/F1 8 Tf\n144 654 Td\n");
  _printf("(%s)Tj\n", CuerrStat);
  file.write("ET\n");
  file.write("0.5 0.5 0.5 rg\nBT\n/F2 7 Tf\n28 699 Td\n(Time Base)Tj\nET\n");
  file.write("BT\n/F2 8 Tf\n28 742 Td(Trip No.)Tj\nET\n");
  file.write("BT\n/F1 7 Tf\n28 732 Td\n");
  _printf("(%s)Tj\n", TripNo);
  file.write("ET\n");
  
  file.write("BT\n/F1 7 Tf\n28 672 Td\n(yyyy/MM/dd)Tj\nET\n");
  file.write("BT\n/F1 7 Tf\n28 663 Td\n(HH/mm/ss)Tj\nET\n");
  file.write("0 0 0 rg\nBT\n/F2 10 Tf\n85 610 Td\n(Logging Summary)Tj\nET\n");
  file.write("0 0 0 RG\n242 616.8 m 250 616.8 l 250 608.8 l 242 608.8 l f\n1 1 1 rg\nBT\n/F1 8 Tf\n243.5 610 Td\n(T)Tj\nET\n");
  file.write("0 0 0 rg\nBT\n/F2 8 Tf\n252 610 Td\n(tempYerature)Tj\nET\n");
  file.write(" 0 0 1 RG\n0.2 w\n[] 0 d\n72 72 m 100 72 l h S\nBT\n/F1 7 Tf\n105 70 Td\n0 0 0 rg\n(Data)Tj\nET\n");
  file.write("1 0 0 RG\n1 w\n[] 0 d\n127 72 m 155 72 l h S\nBT\n/F1 7 Tf\n160 70 Td\n(Limit)Tj\nET\n");
  file.write("BT\n/F2 8 Tf\n242 593 Td\n(Max:)Tj\nET\n");
  file.write("BT\n/F1 8 Tf\n265 593 Td\n");

  _printf("(  %.2fºC@%s )Tj\n", max0, MAXDate);
  file.write("ET\n");
  file.write("BT\n/F2 8 Tf\n242 578 Td\n(Min:)Tj\nET\n");
  file.write("BT\n/F1 8 Tf\n263 578 Td\n");
  _printf("(  %.2fºC@%s )Tj\n", min0, MINDate);
  file.write("ET\n");
  file.write("BT\n/F2 8 Tf\n85 593 Td\n(Start Time:)Tj\nET\n");
  file.write("BT\n/F2 8 Tf\n129 593 Td\n");
  _printf("(%d )Tj\nET\n", startTime);
  file.write("BT\n/F2 8 Tf\n85 578 Td\n(Stop Time:)Tj\nET\n");
  file.write("BT\n/F2 8 Tf\n129 578 Td\n");
  _printf("(%d )Tj\nET\n", stopTime);  
  file.write("BT\n/F2 8 Tf\n85 549 Td\n(Trip Length:)Tj\nET\n");
  file.write("BT\n/F1 8 Tf\n134 549 Td\n");
  _printf("(%d )Tj\n", TripLength);
  file.write("ET\n");
  file.write("BT\n/F2 8 Tf\n85 564 Td\n(Data Points:)Tj\nET\n");
  file.write("BT\n/F1 8 Tf\n134 564 Td\n");
  _printf("(%d)Tj\n", DataPoint);
  file.write("ET\n");
  file.write("BT\n/F2 10 Tf\n85 515 Td\n");
  _printf("(%s)Tj\n", AlarmSetting);
  file.write("ET\n");
  file.write("BT\n/F2 8 Tf\n85 502 Td\n(Alarm Zones)Tj\nET\nBT\n/F2 8 Tf\n176 502 Td\n(Alarm Delay)Tj\nET\nBT\n/F2 8 Tf\n275 502 Td\n(Total Time)Tj\nET\nBT\n/F2 8 Tf\n358 502 Td\n(Events)Tj\nET\nBT\n/F2 8 Tf\n412 502 Td\n(First Triggered@)Tj\nET\n");
  file.write("BT\n/F2 8 Tf\n412 502 Td\n(First Triggered@)Tj\nET\nBT\n/F2 8 Tf\n518 502 Td\n(Status)Tj\nET\nBT\n/F1 6 Tf\n558 26 Td\n(1/1)Tj\nET\n");
  file.write("BT\n/F2 8 Tf\n242 549 Td\n(MKT:)Tj\nET\n");
  file.write("BT\n/F1 8 Tf\n263 549 Td\n");
  _printf("(  %s\260C)Tj\n", MKTtempY);
  file.write("ET\n");
  file.write("BT\n/F2 8 Tf\n242 564 Td\n(Avg:)Tj\nET\n");
  file.write("BT\n/F1 8 Tf\n264 564 Td\n");
  _printf("(  %s\260C)Tj\n", AVGtempY);
  file.write("ET\n");
  file.write("BT\n/F1 9 Tf\n53 70 Td\n([\260C])Tj\nET\n");
  file.write("\n");
  graghSquare();
  graghVerticalLine();
  graghHorizontalLine();
  graghHorizontalLineTime();
  graghtempYData();

}

// PDF Draw gragh 
void graghSquare()
{
  // Create Square 
  file.write("1 w\n");
  file.write("0.5 0.5 0.5 RG\n");
  file.write("[] 0 d\n");
  file.write("53 100 496 320 re h S\n");
}

void graghVerticalLine()
{
  int minTempY = min0;
  int maxTempY = max0;

  int verticalStand = 100;
  int lenTempY = 0;

  Serial.print(tempY[5]);
  lenTempY = (abs(maxTempY) + abs(min0));

  int cnt = 10;
  int cntTempY = lenTempY/cnt;

  // MinTemp
  file.write("0.1 w\n");
  file.write("[2] 0 d\n");
  file.write("BT\n/F1 7 Tf\n");
  _printf("32  %d Td\n",  verticalStand);
  _printf("(%d)Tj\n", minTempY);
  file.write("ET\n");
  
  for(int i = 1; i < cnt; i++) {
    minTempY= minTempY + cntTempY;

    file.write("0.1 w\n");
    file.write("[2] 0 d\n");
    _printf("53 %d m 548 %d l 0 0 m s\n", verticalStand + (320/cnt * i), verticalStand + (320/cnt * i));
    file.write("BT\n/F1 7 Tf\n");
    _printf("32  %d Td\n",  verticalStand + (320/cnt * i));
    _printf("(%d)Tj\n", minTempY);
    file.write("ET\n");
  }
  // MaxTemp
  file.write("0.1 w\n");
  file.write("[2] 0 d\n");
  _printf("53 %d m 548 %d l 0 0 m s\n", verticalStand + (320/cnt * (cnt)), verticalStand + (320/cnt * (cnt)));
    file.write("BT\n/F1 7 Tf\n");
  _printf("32  %d Td\n",  verticalStand + (320/cnt * (cnt)));
  _printf("(%d)Tj\n", maxTempY);
  file.write("ET\n");          
  }

void graghHorizontalLine()
{
  int i = 0;
  int x = 8;
  int horizontalStand = 101;
  
  file.write("0.1 w\n0.5 0.5 0.5 RG\n[1] 0 d\n");
  _printf("%d 100 m %d 420 l 0 0 m s\n", horizontalStand, horizontalStand);
  while(i < x){
    i = i + 1;
    file.write("0.1 w\n0.5 0.5 0.5 RG\n[1] 0 d\n");
    _printf("%d 100 m %d 420 l 0 0 m s\n", horizontalStand + (50 * i), horizontalStand + (50 * i));
  }  
}

void graghHorizontalLineTime(){
  int a = 100;
  int addTime, avgTime; 
  avgTime = stopTime / 10;
  
  _printf("BT\n/F1 7 Tf\n0%d 91 TD\n(%d)Tj\nET\n", a - 50, startTime);

  for (int i = 1; i < 11; i++) {
    addTime = startTime + (10 * i);
    _printf("BT\n/F1 7 Tf\n%d 91 TD\n(%d)Tj\nET\n", a, addTime);
    a = a + 50;    
  }
  file.write("0.1 w\n0 0 1 RG\n[] 0 d\n");

}

void graghtempYData(){
  float graghTimeX, graghtempY;
  int maxTempY = max0;
  int minTempY = min0;
  int i = 0;

  graghTimeX = 52.8;
  graghtempY = 100 + ((320 / (abs(max0) + abs(min0))) * (tempY[i]+abs(min0))); // 280 = 380 - 100

  file.write("0.1 w\n0 0 1 RG\n[] 0 d\n");
  _printf("%.2f %.2f m", graghTimeX, graghtempY);  

  while (i < dataLen - 1) 
  {
     i = i + 1; 
    graghTimeX = 52.8 + (496.2 / (dataLen-1) * i);  //ok 496.2? = 550 - 52.8
    graghtempY = 100 + (320.00/(abs(max0) + abs(min0))) * (tempY[i]+abs(min0));
        
    _printf(" %.2f %.2f", graghTimeX, graghtempY);
    file.write(" l");
  }
  _printf(" %.2f %.2f", graghTimeX, graghtempY);

  file.write(" m h s\n");
  file.write("0 0 0 rg\nBT\n/F1 0.2 Tf\n560 20 Td\n(:1.8:)Tj\nET\n");
  
  file.write("endstream\nendobj\n");

  // END
  file.write("\nxref\n0 5\n0000000000 65535 f\n0000000010 00000 n\n0000000047 00000 n\n0000000111 00000 n\n0000000313 00000 n\ntrailer\n<<\n  /Root 1 0 R\n>>\n\n\n");
  file.write("startxref\n416\n%%EOF");
  
  Serial.print("File Save End");
}

void makefile()
{
  //const uint8_t BASE_NAME_SIZE = sizeof(FILE_BASE_NAME) - 1;
  char fileName[13] = FILE_BASE_NAME; //+ "00.csv";

  Serial.print(F("File TEST Start"));
    
  if (!file.open(fileName, O_WRONLY | O_CREAT | O_EXCL)) {
    Serial.print("file.open Error");
  }
  // Read any Serial data.

  do 
  {
    Serial.print("-");
    delay(10);
  } while (Serial.available() && Serial.read() >= 0);
  
  Serial.print(F("Logging to: "));
  Serial.println(fileName);
  Serial.println(F("Type any character to stop"));

  // Write data header.
  writeHeader();
  file.close();

  // NVIC_SystemReset();   //System reboot
  
}

void tmepData() {
  float tempY;
  float graghTimeX;
  float graghtempY;
  int min0 = -5;
  int max0 = 5;
  int i = 0;
  // flag = 0 처음 시작
  // flag = 1 중간 데이터
  // flag = 2 마지막 데이터

  // _printf(" %.2f %.2f", graghTimeX, graghtempY);
  // file.write(" l");
  while (true)
  {
    i ++;
    tempY = random(min0, max0);
    // graghTimeX = 52.8 + (497.2 / (dataLen-1) * i);  // ok 497.2? = 550 - 52.8
    graghtempY = 100 + (320.00 / (abs(max0) + abs(min0))) * (tempY + abs(min0));

    if (tempY == 1.00) 
    {  // stop read and pdf create
      flag = 2;
    }
    if (flag == 0) 
    {
      Serial.print("tempY: ");
      Serial.println(tempY);

      Serial.print("graghtempY: ");
      Serial.print(graghtempY);
      Serial.println(" m");
      flag = 1;
    }
    else if (flag == 1)
    {
      Serial.print("tempY: ");
      Serial.println(tempY);

      Serial.print("graghtempY: ");
      Serial.print(graghtempY);
      Serial.println(" l");
    }  
    else if (flag == 2) 
    {
      Serial.print("tempY: ");
      Serial.println(tempY);

      Serial.print("graghtempY: ");
      Serial.print(graghtempY);
      Serial.println(" m h s");
      flag = 3;
    }
    else if (flag == 3)
    {
      Serial.println("STOP");
      delay(5000);
    }
    delay(1000);    
  }
  
}

void loop()
{
  tmepData(); // tempData write gragh

  //LED TEST
  //PIN_NEOPIXEL
  digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
  //analogWrite(PIN_NEOPIXEL, 0xFF0000);
  //colorWipe(strip.Color(255, 0, 0), 50); //빨간색 출력
  //strip.Color(255, 0, 0);
  //Serial.println("HADES TEST PIN_NEOPIXEL, red");
  delay(1000);                       // wait for a second
  digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
  //colorWipe(strip.Color(0, 0, 255), 50); //파란색 출력
  //strip.Color(0, 0, 255);
  //Serial.println("HADES TEST PIN_NEOPIXEL, blue");
  
  delay(1000); 
  
  if ( changed )
  {
    changed = false;
    
    if ( !root.open("/") )
    {
      Serial.println("open root failed");
      return;
    }

    Serial.println("Flash contents:");
    delay(1000);

    // Open next file in root.
    // Warning, openNext starts at the current directory position
    // so a rewind of the directory may be required.


    root.close();

    Serial.println();
    delay(1000); // refresh every 0.5 second
  }
}

// Callback invoked when received READ10 command.
// Copy disk's data to buffer (up to bufsize) and 
// return number of copied bytes (must be multiple of block size) 
int32_t msc_read_cb (uint32_t lba, void* buffer, uint32_t bufsize)
{
  // Note: SPIFLash Bock API: readBlocks/writeBlocks/syncBlocks
  // already include 4K sector caching internally. We don't need to cache it, yahhhh!!
  return flash.readBlocks(lba, (uint8_t*) buffer, bufsize/512) ? bufsize : -1;
}

// Callback invoked when received WRITE10 command.
// Process data in buffer to disk's storage and 
// return number of written bytes (must be multiple of block size)
int32_t msc_write_cb (uint32_t lba, uint8_t* buffer, uint32_t bufsize)
{
  digitalWrite(LED_BUILTIN, HIGH);

  // Note: SPIFLash Bock API: readBlocks/writeBlocks/syncBlocks
  // already include 4K sector caching internally. We don't need to cache it, yahhhh!!
  return flash.writeBlocks(lba, buffer, bufsize/512) ? bufsize : -1;
}

// Callback invoked when WRITE10 command is completed (status received and accepted by host).
// used to flush any pending cache.
void msc_flush_cb (void)
{
  // sync with flash
  flash.syncBlocks();

  // clear file system's cache to force refresh
  fatfs.cacheClear();

  changed = true;

  digitalWrite(LED_BUILTIN, LOW);
}

// Converting from Hex to Decimal:

// NOTE: This function can handle a positive hex value from 0 - 65,535 (a four digit hex string).
//       For larger/longer values, change "unsigned int" to "long" in both places.


unsigned int hexToDec(String hexString) {
  
  unsigned int decValue = 0;
  int nextInt;
  
  for (int i = 0; i < hexString.length(); i++) {
    
    nextInt = int(hexString.charAt(i));
    if (nextInt >= 48 && nextInt <= 57) nextInt = map(nextInt, 48, 57, 0, 9);
    if (nextInt >= 65 && nextInt <= 70) nextInt = map(nextInt, 65, 70, 10, 15);
    if (nextInt >= 97 && nextInt <= 102) nextInt = map(nextInt, 97, 102, 10, 15);
    nextInt = constrain(nextInt, 0, 15);
    
    decValue = (decValue * 16) + nextInt;
  }
  
  return decValue;
}