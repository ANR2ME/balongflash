// 
// Процедуры обработки цифровых подписей
// 
#include <stdio.h>
#include <stdint.h>
#ifndef WIN32
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <strings.h>
#include <termios.h>
#include <unistd.h>
#include <arpa/inet.h>
#else
#include <windows.h>
#include "getopt.h"
#include "printf.h"
#include "buildno.h"
#endif

#include "hdlcio.h"
#include "ptable.h"
#include "flasher.h"
#include "util.h"
#include "zlib.h"

// таблица параметров ключа -g

struct {
  uint8_t type;
  uint32_t len;
  char* descr;
} signbase[] = {
  {1,2958,"Основная прошивка"},
  {1,2694,"Прошивка E3372s-stick"},
  {2,1110,"Вебинтерфейс+ISO для HLINK-модема"},
  {6,1110,"Вебинтерфейс+ISO для HLINK-модема"},
  {2,846,"ISO (dashboard) для stick-модема"},
  {7,3750,"Прошивка+ISO+вебинтерфейс"},
  {99,3750,"универсальная"},
};

#define signbaselen 7

// таблица типов подписей
struct {
  uint8_t code;
  char* descr;
} fwtypes[]={
  {1,"ONLY_FW"},
  {2,"ONLY_ISO"},
  {3,"FW_ISO"},
  {4,"ONLY_WEBUI"},
  {5,"FW_WEBUI"},
  {6,"ISO_WEBUI"},
  {7,"FW_ISO_WEBUI"},
  {99,"COMPONENT_MAX"},
  {0,0}
};  


// результирующая строка ^signver-команды
uint8_t signver[200];

// Флаг режима цифровой подписи
extern int gflag;

//****************************************************
//* Получение описания типа прошивки по коду
//****************************************************
char* fw_description(uint8_t code) {
  
int i;  
for (i=0; (fwtypes[i].code != 0); i++) {
  if (code == fwtypes[i].code) return fwtypes[i].descr;
}
return 0;
}

//****************************************************
//* Получение списка параметров ключа -g
//****************************************************
void glist() {
  
int i;
printf("\n #  длина  тип описание \n--------------------------------------");
for (i=0; i<signbaselen; i++) {
  printf("\n%1i  %5i  %2i   %s",i,signbase[i].len,signbase[i].type,signbase[i].descr);
}
printf("\n\n Также можно указать произвольные параметры подписи в формате:\n  -g *,type,len\n\n");
exit(0);
}

//***************************************************
//* Обработка параметров ключа -g
//***************************************************
void gparm(char* sparm) {
  
int index;  
char* sptr;
char parm[100];

// Параметры текущей цифровой подписи
uint32_t signtype; // тип прошивки
uint32_t signlen;  // длина подписи


if (gflag != 0) {
  printf("\n Дублирующийся ключ -g\n\n");
  exit(-1);
}  

strcpy(parm,sparm); // локальная копия параметров

if (parm[0] == 'l') {
  glist();
  exit(0);
}  

if (strncmp(parm,"*,",2) == 0) {
  // произвольные параметры
  // выделяем длину
  sptr=strrchr(parm,',');
  if (sptr == 0) goto perror;
  signlen=atoi(sptr+1);
  *sptr=0;
  // выделяем тип раздела
  sptr=strrchr(parm,',');
  if (sptr == 0) goto perror;
  signtype=atoi(sptr+1);
  if (fw_description(signtype) == 0) {
    printf("\n Ключ -g: неизвестный тип прошивки - %i\n",signtype);
    exit(-1);
  }  
}
else {  
  index=atoi(parm);
  if (index >= signbaselen) goto perror;
  signlen=signbase[index].len;
  signtype=signbase[index].type;
}

gflag=1;
sprintf(signver,"^SIGNVER=%i,0,778A8D175E602B7B779D9E05C330B5279B0661BF2EED99A20445B366D63DD697,%i",signtype,signlen);
printf("\n Режим цифровой подписи: %s (%i байт)",fw_description(signtype),signlen);
// printf("\nstr - %s",signver);
return;

perror:
 printf("\n Ошибка в параметрах ключа -g\n");
 exit(-1);
} 
  

//***************************************************
//* Отправка цифровой подписи
//***************************************************
void send_signver() {
  
uint32_t res;
// ответ на ^signver
unsigned char SVrsp[]={0x0d, 0x0a, 0x30, 0x0d, 0x0a, 0x0d, 0x0a, 0x4f, 0x4b, 0x0d, 0x0a};
uint8_t replybuf[200];
  
res=atcmd(signver,replybuf);
if ( (res<sizeof(SVrsp)) || (memcmp(replybuf,SVrsp,sizeof(SVrsp)) != 0) ) {
   printf("\n ! Ошибка проверки цифровой сигнатуры - %02x\n",replybuf[2]);
   exit(-2);
}
}
