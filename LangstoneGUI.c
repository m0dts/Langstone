
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <iio.h>
#include <unistd.h>
#include <wiringPi.h>
#include <stdio.h>
#include <string.h>
#include "Graphics.h"
#include "Touch.h"
#include "Mouse.h"

void setFreq(double fr);
void setFreqInc();
void setTx(int ptt);
void setMode(int mode);
void setVolume(int vol);
void setSquelch(int sql);
void setSSBMic(int mic);
void setFMMic(int mic);
void setBandBits(int b);
void processTouch();
void processMouse(int mbut);
void initGUI();
void sendFifo(char * s);
void initFifo();
void setPlutoRxFreq(long long rxfreq);
void setPlutoTxFreq(long long rxfreq);
void setHwRxFreq(double fr);
void setHwTxFreq(double fr);
void PlutoTxEnable(int txon);
void PlutoRxEnable(int rxon);
void detectHw();
int buttonTouched(int bx,int by);
void setKey(int k);
void displayMenu(void);
void displaySetting(int se);
void changeSetting(void);
void processGPIO(void);
void initGPIO(void);
int readConfig(void);
int writeConfig(void);
int satMode(void);
int txvtrMode(void);
void setMoni(int m);
void setFFTRef(int ref);
void initSDR(void);
void setFFTPipe(int cntl);
void waterfall(void);
void init_fft_Fifo();
void setRit(int rit);
void setInputMode(int n);
void gen_palette(char colours[][3],int num_grads);


double freq;
double freqInc=0.001;
#define numband 10
int band=2;
double bandFreq[numband] = {144.200,432.200,1296.200,2320.200,2400.100,3400.100,5760.100,10368.200,24048.200,10489.55};
double bandTxOffset[numband]={0.0,0.0,0.0,0.0,0.0,0.0,0.0,9936.0,23616.0,10069.5};
double bandRxOffset[numband]={0.0,0.0,0.0,0.0,0.0,0.0,0.0,9936.0,23616.0,10345.0};
int bandBits[numband]={0,1,2,3,4,5,6,7,8,9};
int bbits=0;
#define minFreq 0.0
#define maxFreq 99999.99999
#define minHwFreq 70.0
#define maxHwFreq 5999.99999


#define nummode 5
int mode=0;
char * modename[nummode]={"USB","LSB","CW ","CWN","FM "};

#define numSettings 6
int settingNo=0;
int inputMode=0;
char * settingText[numSettings]={"SSB Mic Gain = ","FM Mic Gain = ","Txvtr Rx Offset = ","Txvtr Tx Offset = ","Band Bits = ","FFT Ref = "};

//GUI Layout values X and Y coordinates for each group of buttons.

#define volButtonX 660
#define volButtonY 300
#define sqlButtonX 30
#define sqlButtonY 300
#define ritButtonX 670
#define ritButtonY 60
#define funcButtonsY 429
#define funcButtonsX 30
#define buttonHeight 50
#define buttonSpaceY 55
#define buttonWidth 100
#define buttonSpaceX 105
#define freqDisplayX 80
#define freqDisplayY 55
#define freqDisplayCharWidth 48 
#define freqDisplayCharHeight 54
#define txX 600
#define txY 15
#define modeX 200
#define modeY 15
#define txvtrX 400
#define txvtrY 15
#define moniX 500
#define moniY 15
#define settingX 200
#define settingY 390



int ptt=0;
int ptts=0;
int moni=0;
int fifofd;
int sendDots=0;
int dotCount=0;

long long lastLOhz;

int lastKey=1;

int volume=20;
#define maxvol 100

int squelch=20;
#define maxsql 100

int rit=0;
#define minrit -1000
#define maxrit 1000

int SSBMic=50;
#define maxSSBMic 100

int FMMic=50;
#define maxFMMic 100

int tuneDigit=8;
#define maxTuneDigit 11

#define TXDELAY 10000      //100ms delay between setting Tx output bit and sending tx command to SDR
#define RXDELAY 10000       //100ms delay between sending rx command to SDR and setting Tx output bit low. 

char mousePath[20];
char touchPath[20];
int mousePresent;
int touchPresent;
int plutoPresent;

#define pttPin 0        // Wiring Pi pin number. Physical pin is 11
#define keyPin 1        //Wiring Pi pin number. Physical pin is 12
#define txPin 29        //Wiring Pi pin number. Physical pin is 40      
#define bandPin1 31     //Wiring Pi pin number. Physical pin is 28
#define bandPin2 24     //Wiring Pi pin number. Physical pin is 35
#define bandPin3 7      //Wiring Pi pin number. Physical pin is 7
#define bandPin4 6      //Wiring Pi pin number. Physical pin is 22



//robs Waterfall

float inbuf[2];
FILE *fftstream;
float buf[512][150];
int points=512;
int rows=150;
int FFTRef = -30;
int spectrum_rows=50;
unsigned char * palette;




int main(int argc, char* argv[])
{

clock_t lastClock;

  fftstream=fopen("/tmp/langstonefft","r");                 //Open FFT Stream from GNU Radio 
  fcntl(fileno(fftstream), F_SETFL, O_RDONLY | O_NONBLOCK);
  
  lastClock=0;
  readConfig();
  detectHw();
  initFifo();
  init_fft_Fifo();
  initScreen();
  initGPIO();
  if(touchPresent) initTouch(touchPath);
  if(mousePresent) initMouse(mousePath);
  initGUI(); 
  initSDR(); 
  //              RGB Vals   Black >  Blue  >  Green  >  Yellow   >   Red     4 gradients    //number of gradients is varaible
  gen_palette((char [][3]){ {0,0,0},{0,0,255},{0,255,0},{255,255,0},{255,0,0}},4);

  setFFTPipe(1);            //Turn on FFT Stream from GNU RAdio

  

  while(1)
  {
  
    processGPIO();
  
   if(touchPresent)
     {
       if(getTouch()==1)
        {
         processTouch();
        }
     }
    
    if(mousePresent)
      {
        int but=getMouse();
        if(but>0)
          {
             processMouse(but);
          }           
      }
      
      
    
    if(sendDots==1)
      {
        dotCount=dotCount+1;
        if(dotCount==1)
          {
            setKey(1);
          }
        if(dotCount==12)
          {
            setKey(0);
          }
        if(dotCount==25)
          {
          dotCount=0;
          }
      } 
    waterfall();
    while(clock() < (lastClock + CLOCKS_PER_SEC/100));        //delay until the next iteration at 100 per second
    lastClock=clock();
  }
}

void gen_palette(char colours[][3], int num_grads){
  //allocate some memory, size of palette
  palette = malloc(sizeof(char)*256*3);

  int diff[3];
  float scale=256/(num_grads);
  int pos=0;

  for(int i=0;i<num_grads;i++){
      //get differences in colours for current gradient
      diff[0]=(colours[i+1][0]-colours[i][0])/(scale-1);
      diff[1]=(colours[i+1][1]-colours[i][1])/(scale-1);
      diff[2]=(colours[i+1][2]-colours[i][2])/(scale-1);
      //fprintf(stdout,"diff 0:%i 1:%i 2:%i\n",diff[0],diff[1],diff[2]);

      //create the palette built up of multiple gradients
      for(int n=0;n<scale;n++){
          palette[pos*3+2]=colours[i][0]+(n*diff[0]);
          palette[pos*3+1]=colours[i][1]+(n*diff[1]);
          palette[pos*3]=colours[i][2]+(n*diff[2]);
          //fprintf(stdout,"n:%i r:%i g:%i b:%i\n",pos,palette[pos*3+2],palette[pos*3+1],palette[pos*3]);
          pos++;
      }

  }
}

void waterfall()
{
  int level,level2;
  int ret;
  int bwBarStart=3;
  int bwBarEnd=34;
  int centreShift=0;
  
      //check if data avilable to read
      ret = fread(&inbuf,sizeof(float),1,fftstream);
      if(ret>0)
    {    
    
        //shift buffer
        for(int r=rows-1;r>0;r--)
        {  
          for(int p=0;p<points;p++)
          {  
            buf[p][r]=buf[p][r-1];
          }
        }
    
        buf[0+points/2][0]=inbuf[0];    //use the read value
    
        //Read in float values, shift centre and store in buffer 1st 'row'
        for(int p=1;p<points;p++)
        {  
        fread(&inbuf,sizeof(float),1,fftstream);
          if(p<points/2)
          {
            buf[p+points/2][0]=inbuf[0];
          }else
          {
            buf[p-points/2][0]=inbuf[0];
          }
        }
    
        //RF level adjustment
    
        int baselevel=FFTRef-50;
        float scaling = 255.0/(float)(FFTRef-baselevel);
    
        //draw waterfall
        for(int r=0;r<rows;r++)
        {  
          for(int p=0;p<points;p++)
          {  
            //limit values displayed to range specified
            if (buf[p][r]<baselevel) buf[p][r]=baselevel;
            if (buf[p][r]>FFTRef) buf[p][r]=FFTRef;
    
            //scale to 0-255
            level = (buf[p][r]-baselevel)*scaling;   
            setPixel(p+140,206+r,palette[level*3+2],palette[level*3+1],palette[level*3]);
          }
        }
    
        //clear spectrum area
        for(int r=0;r<spectrum_rows+1;r++)
        { 
          for(int p=0;p<points;p++)
          {   
            setPixel(p+140,186-r,0,0,0);
          }
        }
    
        //draw spectrum line
        
        scaling = spectrum_rows/(float)(FFTRef-baselevel);
        for(int p=0;p<points-1;p++)
        {  
            //limit values displayed to range specified
            if (buf[p][0]<baselevel) buf[p][0]=baselevel;
            if (buf[p][0]>FFTRef) buf[p][0]=FFTRef;
    
            //scale to display height
            level = (buf[p][0]-baselevel)*scaling;   
            level2 = (buf[p+1][0]-baselevel)*scaling;
            drawLine(p+140, 186-level, p+1+140, 186-level2,255,255,255);
        }
          
          //draw Bandwidth indicator
          if (mode==0)
          {
           bwBarStart=3;
           bwBarEnd=34;
           centreShift=0;
          }
          if (mode==1)
          {
           bwBarStart=-34;
           bwBarEnd=-3;
           centreShift=0;          
          }
          if (mode==2)
          {
           bwBarStart=3;
           bwBarEnd=34;
           centreShift=9;
          }
          if (mode==3)
          {
           bwBarStart=6;
           bwBarEnd=12;
           centreShift=9;          
          }
          if (mode==4)
          {
           bwBarStart=-87;
           bwBarEnd=87;
           centreShift=0;          
          }
          int p=points/2;

          drawLine(p+140+bwBarStart, 186-spectrum_rows+5, p+140+bwBarStart, 186-spectrum_rows,255,140,0);
          drawLine(p+140+bwBarStart, 186-spectrum_rows, p+140+bwBarEnd, 186-spectrum_rows,255,140,0);
          drawLine(p+140+bwBarEnd, 186-spectrum_rows+5, p+140+bwBarEnd, 186-spectrum_rows,255,140,0);  
          //draw centre line (displayed frequency)
          drawLine(p+140+centreShift, 186-10, p+140+centreShift, 186-spectrum_rows,255,0,0);   
   }       
}

void setFFTRef(int ref)
{
  FFTRef=ref;
}

void detectHw()
{
  FILE * fp;
  char * ln=NULL;
  size_t len=0;
  ssize_t rd;
  int p;
  char handler[2][10];
  char * found;
  p=0;
  mousePresent=0;
  touchPresent=0;
  fp=fopen("/proc/bus/input/devices","r");
   while ((rd=getline(&ln,&len,fp)!=-1))
    {
      if(ln[0]=='N')        //name of device
      {
        if(strstr(ln,"FT5406")!=NULL) p=1; else p=0;     //Found Raspberry Pi TouchScreen entry
      }
      
      if(ln[0]=='H')        //handlers
      {
         if(strstr(ln,"mouse")!=NULL)
         {
           found=strstr(ln,"event");
           strcpy(handler[p],found);
           handler[p][6]=0;
           if(p==0) 
            {
              sprintf(mousePath,"/dev/input/%s",handler[0]);
              mousePresent=1;
            }
           if(p==1) 
           {
             sprintf(touchPath,"/dev/input/%s",handler[1]);
             touchPresent=1;
           }
         }
      }   
    }
  fclose(fp);
  if(ln)  free(ln);
    plutoPresent=1;      //this will be reset by setPlutoFreq if Pluto is not present.
}


void setPlutoRxFreq(long long rxfreq)
{
  struct iio_context *ctx;
  struct iio_device *phy;
   if(plutoPresent)
    {
      ctx = iio_create_context_from_uri("ip:192.168.2.1");
      if(ctx==NULL)
      {
        plutoPresent=0;
        gotoXY(220,120);
        setForeColour(255,0,0);
        textSize=2;
        displayStr("PLUTO NOT DETECTED");
      }
      else
      { 
      phy = iio_context_find_device(ctx, "ad9361-phy"); 
      iio_channel_attr_write_longlong(iio_device_find_channel(phy, "altvoltage0", true),"frequency", rxfreq); //Rx LO Freq
      }
      iio_context_destroy(ctx); 
    }
  
}

void setPlutoTxFreq(long long txfreq)
{
  struct iio_context *ctx;
  struct iio_device *phy;
   if(plutoPresent)
    {
      ctx = iio_create_context_from_uri("ip:192.168.2.1");
      if(ctx==NULL)
      {
        plutoPresent=0;
        gotoXY(220,120);
        setForeColour(255,0,0);
        textSize=2;
        displayStr("PLUTO NOT DETECTED");
      }
      else
      { 
      phy = iio_context_find_device(ctx, "ad9361-phy"); 
      iio_channel_attr_write_longlong(iio_device_find_channel(phy, "altvoltage1", true),"frequency", txfreq); //Tx LO Freq
      }
      iio_context_destroy(ctx); 
    }
  
}

void PlutoTxEnable(int txon)
{
  struct iio_context *ctx;
  struct iio_device *phy;
  
  if(plutoPresent)
    {
      ctx = iio_create_context_from_uri("ip:192.168.2.1");
      phy = iio_context_find_device(ctx, "ad9361-phy"); 
      if(txon==0)
        {
        iio_channel_attr_write_bool(iio_device_find_channel(phy, "altvoltage1", true),"powerdown", true); //turn off TX LO
        }
      else
        {
        iio_channel_attr_write_bool(iio_device_find_channel(phy, "altvoltage1", true),"powerdown", false); //turn on TX LO
        }
      
      iio_context_destroy(ctx);
    }

}

void PlutoRxEnable(int rxon)
{
  struct iio_context *ctx;
  struct iio_device *phy;
  
  if(plutoPresent)
    {
      ctx = iio_create_context_from_uri("ip:192.168.2.1");
      phy = iio_context_find_device(ctx, "ad9361-phy"); 
      if(rxon==0)
        {
        iio_channel_attr_write_bool(iio_device_find_channel(phy, "altvoltage0", true),"powerdown", true); //turn off RX LO
        }
      else
        {
        iio_channel_attr_write_bool(iio_device_find_channel(phy, "altvoltage0", true),"powerdown", false); //turn on RX LO
        }
      
      iio_context_destroy(ctx);
    }

}



void initFifo()
{
 if(access("/tmp/langstonein",F_OK)==-1)   //does fifo exist already?
    {
        mkfifo("/tmp/langstonein", 0666);
    }
}

void init_fft_Fifo()
{
 if(access("/tmp/langstonefft",F_OK)==-1)   //does fifo exist already?
    {
        mkfifo("/tmp/langstonefft", 0666);
    }
}

void sendFifo(char * s)
{
  char fs[50];
  strcpy(fs,s);
  strcat(fs,"\n");
  fifofd=open("/tmp/langstonein",O_WRONLY);
  write(fifofd,fs,strlen(fs));
  close(fifofd);
  delay(5);
}

void initGPIO(void)
{
  wiringPiSetup();
  pinMode(pttPin,INPUT);
  pinMode(keyPin,INPUT);
  pinMode(txPin,OUTPUT);
  pinMode(bandPin1,OUTPUT); 
  pinMode(bandPin2,OUTPUT);  
  pinMode(bandPin3,OUTPUT);  
  pinMode(bandPin4,OUTPUT);  
  digitalWrite(txPin,LOW);
  digitalWrite(bandPin1,LOW); 
  digitalWrite(bandPin2,LOW); 
  digitalWrite(bandPin3,LOW); 
  digitalWrite(bandPin4,LOW);    
  lastKey=1;
}

void processGPIO(void)
{
  int v=digitalRead(pttPin);
  if(v==0)
      {
        if(ptt==0)
          {
            ptt=1;
            setTx(ptt|ptts);
          }
      }
   else
      {
        if(ptt==1)
          {
            ptt=0;
            setTx(ptt|ptts);
          }
      }
    v=digitalRead(keyPin);
    if(v!=lastKey)
  {
  setKey(!v);
  lastKey=v;
  }
}


void sqlButton(int show)
{
 char sqlStr[5]; 
  gotoXY(sqlButtonX,sqlButtonY);
  if(show==1)
    {
      setForeColour(0,255,0);
    }
  else
    {
      setForeColour(0,0,0);
    }
  displayButton("SQL");
  textSize=2;
  gotoXY(sqlButtonX+30,sqlButtonY-25);
  displayStr("   ");
  gotoXY(sqlButtonX+30,sqlButtonY-25);
  sprintf(sqlStr,"%d",squelch);
  displayStr(sqlStr);
}

void ritButton(int show)
{
 char ritStr[5]; 
 int to;
  if(show==1)
    {
      setForeColour(0,255,0);
    }
  else
    {
      setForeColour(0,0,0);
      gotoXY(ritButtonX,ritButtonY+buttonSpaceY);
      displayButton("Zero"); 
    }
  gotoXY(ritButtonX,ritButtonY);
  displayButton("RIT");
  textSize=2;
  to=0;
  if(abs(rit)>0) to=8;
  if(abs(rit)>9) to=16;
  if(abs(rit)>99) to=24;
  if(abs(rit)>999) to=32;
  gotoXY(ritButtonX,ritButtonY-25);
  displayStr("         ");
  gotoXY(ritButtonX+38-to,ritButtonY-25);
  if (rit==0)
  {
  sprintf(ritStr,"0");
  }
  else
  {
    sprintf(ritStr,"%+d",rit);
  }
  displayStr(ritStr);

}

void initGUI()
{
  clearScreen();


// Volume Button
  gotoXY(volButtonX,volButtonY);
  setForeColour(0,255,0);
  displayButton("Vol");
 

 //bottom row of buttons
  displayMenu();
  freq=bandFreq[band];
  bbits=bandBits[band];
  setBandBits(bbits);

  
  if(mode==4) 
    {
    sqlButton(1);
    }
  else
    {
    sqlButton(0);
    }

    //clear waterfall buffer.
        //shift buffer
    for(int r=0;r<rows;r++){  
      for(int p=0;p<points;p++){  
        buf[p][r]=-100;
      }
    }

}


void initSDR(void)
{
  setMode(mode); 
  setVolume(volume);
  setSquelch(squelch);
  setRit(0);
  setSSBMic(SSBMic);
  setFMMic(FMMic);
  setFreqInc();
  lastLOhz=0;
  setFreq(freq);
}

void displayMenu()
{
gotoXY(funcButtonsX,funcButtonsY);
  setForeColour(0,255,0);
  displayButton("BAND");
  displayButton("MODE");
  displayButton(" ");
  displayButton("SET");
  displayButton("    ");
  displayButton("DOTS");
  displayButton("PTT");

}




void processMouse(int mbut)
{
  if(mbut==128)       //scroll whell turned 
    {
      if(inputMode==0)
      {
        freq=freq+(mouseScroll*freqInc);
        mouseScroll=0;
        if(freq < minFreq) freq=minFreq;
        if(freq > maxFreq) freq=maxFreq;
        setFreq(freq);
        return;      
      }
      if(inputMode==1)
      {
        changeSetting();
        return;
      }
      if(inputMode==2)
      {
        volume=volume+mouseScroll;
        mouseScroll=0;
        if(volume < 0) volume=0;
        if(volume > maxvol) volume=maxvol;
        setVolume(volume);
        return;      
      }    
      if(inputMode==3)
      {
        squelch=squelch+mouseScroll;
        mouseScroll=0;
        if(squelch < 0) squelch=0;
        if(squelch > maxsql) squelch=maxsql;
        setSquelch(squelch);
        return;      
      }   
      if(inputMode==4)
      {
        rit=rit+mouseScroll*10;
        mouseScroll=0;
        if(rit < minrit) rit=minrit;
        if(rit > maxrit) rit=maxrit;
        setRit(rit);
        return;      
      }     
    }
    
  if(mbut==1+128)      //Left Mouse Button down
    {
      tuneDigit=tuneDigit-1;
      if(tuneDigit<0) tuneDigit=0;
      if(tuneDigit==5) tuneDigit=4;
      if(tuneDigit==9) tuneDigit=8;
      setFreqInc();
      setFreq(freq);     
    }
    
  if(mbut==2+128)      //Right Mouse Button down
    {
      tuneDigit=tuneDigit+1;
      if(tuneDigit > maxTuneDigit) tuneDigit=maxTuneDigit;
      if(tuneDigit==5) tuneDigit=6;
      if(tuneDigit==9) tuneDigit=10;
      setFreqInc();
      setFreq(freq);       
    }
    
  if(mbut==4+128)      //Extra Button down
    {
 
    }   
    
  if(mbut==5+128)      //Side Button down
    {
 
    }
  
    
}

void setFreqInc()
{
  if(tuneDigit==0) freqInc=10000.0;
  if(tuneDigit==1) freqInc=1000.0;
  if(tuneDigit==2) freqInc=100.0;
  if(tuneDigit==3) freqInc=10.0;
  if(tuneDigit==4) freqInc=1.0;
  if(tuneDigit==5) tuneDigit=6;
  if(tuneDigit==6) freqInc=0.1;
  if(tuneDigit==7) freqInc=0.01;
  if(tuneDigit==8) freqInc=0.001;                                    
  if(tuneDigit==9) tuneDigit=10;
  if(tuneDigit==10) freqInc=0.0001;
  if(tuneDigit==11) freqInc=0.00001;   
}
       


void processTouch()
{ 


   
// Volume Button   


if(buttonTouched(volButtonX,volButtonY))    //Vol
    {
      if(inputMode==2)
        {
        setInputMode(0);
        }
      else
        {
        setInputMode(2);
        }
      return;
    }
                                                        


// Squelch Button   


if(buttonTouched(sqlButtonX,sqlButtonY))    //sql
    {
     if(mode==4)
     {
      if(inputMode==3)
        {
        setInputMode(0);
        }
      else
        {
        setInputMode(3);
        }
      return;
      }
    }


//RIT Button

if(buttonTouched(ritButtonX,ritButtonY))    //rit
    {
     if(mode!=4)
     {
      if(inputMode==4)
        {
        setInputMode(0);
        }
      else
        {
        setInputMode(4);
        }
      return;
      }
    }

//RIT Zero Button

if(buttonTouched(ritButtonX,ritButtonY+buttonSpaceY))    //rit zero
    {
     setRit(0);
     setInputMode(0);
    }
//Function Buttons


if(buttonTouched(funcButtonsX,funcButtonsY))    //Button 1 = BAND or MENU
    {
    if(inputMode==0)
    {
      bandFreq[band]=freq;
      band=band+1;
      if(band==numband) band=0;
      freq=bandFreq[band];
      setFreq(freq);
      bbits=bandBits[band];
      setBandBits(bbits);
      writeConfig();          //save all settings when changing band. 
      return;
    }
    else
    {
      setInputMode(0);
      return; 
    }
      
        
    }      
if(buttonTouched(funcButtonsX+buttonSpaceX,funcButtonsY))    //Button 2 = MODE or Blank
    {
     if(inputMode==0)
      {
      mode=mode+1;
      if(mode==nummode) mode=0;
      setMode(mode);
      return;
      }
      else
      {
      setInputMode(0);
      return;
      }
    }
      
if(buttonTouched(funcButtonsX+buttonSpaceX*2,funcButtonsY))  // Button 3 =Blank or NEXT
    {
     if(inputMode==0)
      {
      return;
      }
      else if(inputMode==1)
      {
      settingNo=settingNo+1;
      if(settingNo==numSettings) settingNo=0;
      displaySetting(settingNo);
      return;
      }
      else
      {
      setInputMode(0);
      }
    }
      
if(buttonTouched(funcButtonsX+buttonSpaceX*3,funcButtonsY))    // Button4 =SET or Blank
    {
     if(inputMode==0)
      {
      setInputMode(1);
      return;
      }
      else
      {
      setInputMode(0);
      return;
      }
    }
       
if(buttonTouched(funcButtonsX+buttonSpaceX*4,funcButtonsY))    //Button 5 =MONI (only allowed in Sat mode)  or PREV
    {
    if(inputMode==0)
      {
      if(satMode()==1)
        {
        if(moni==1) setMoni(0); else setMoni(1);
        }      
      return;
      }
      else  if (inputMode==1)
      {
      settingNo=settingNo-1;
      if(settingNo<0) settingNo=numSettings-1;
      displaySetting(settingNo);
      return;
      }
      else
      {
      setInputMode(0);
      }
      
 
    }      

if(buttonTouched(funcButtonsX+buttonSpaceX*5,funcButtonsY))    //Button 6 = DOTS  or Blank
    {
    if(inputMode==0)
      {
      if(sendDots==0)
        {
          sendDots=1;
          setMode(2);
          setTx(1); 
          gotoXY(funcButtonsX+buttonSpaceX*5,funcButtonsY);
          setForeColour(255,0,0);
          displayButton("DOTS");  
          ptts=1;
          gotoXY(funcButtonsX+buttonSpaceX*6,funcButtonsY);
          setForeColour(255,0,0);
          displayButton("PTT");       
        }
      else
        {
          sendDots=0;
          setTx(0);
          setKey(0);
          setMode(mode);
          gotoXY(funcButtonsX+buttonSpaceX*5,funcButtonsY);
          setForeColour(0,255,0);
          displayButton("DOTS");  
          ptts=0;
          gotoXY(funcButtonsX+buttonSpaceX*6,funcButtonsY);
          setForeColour(0,255,0);
          displayButton("PTT");       
        }
      return;
      }
      else
      {
      setInputMode(0);
      return;
      }
    } 
         
if(buttonTouched(funcButtonsX+buttonSpaceX*6,funcButtonsY))   //Button 7 = PTT  or OFF
    {
    if(inputMode==0)
      {
      if(ptts==0)
        {
          ptts=1;
          setTx(ptt|ptts);
          gotoXY(funcButtonsX+buttonSpaceX*6,funcButtonsY);
          setForeColour(255,0,0);
          displayButton("PTT"); 
        }
      else
        {
          ptts=0;
          setTx(ptt|ptts);
          gotoXY(funcButtonsX+buttonSpaceX*6,funcButtonsY);
          setForeColour(0,255,0);
          displayButton("PTT"); 
        }
      return;
      }
      else if (inputMode==1)
      {
      sendFifo("Q");       //kill the SDR
      writeConfig();
      system("sudo cp /home/pi/Langstone/splash.bgra /dev/fb0");
      sleep(5);
      system("sudo poweroff");                          
      return;
      }
      else
      {
      setInputMode(0);
      }
      

    }      



   
//Touch on Frequency Digits moves cursor to digit and sets tuning step. 

if((touchY>freqDisplayY) & (touchY < freqDisplayY+freqDisplayCharHeight) & (touchX>freqDisplayX) & (touchX < freqDisplayX+12*freqDisplayCharWidth))   
  {
    setInputMode(0);
    int tx=touchX-freqDisplayX;
    tx=tx/freqDisplayCharWidth;
    tuneDigit=tx;
    setFreqInc();
    setFreq(freq);
    return;
  }

}


int buttonTouched(int bx,int by)
{
  return ((touchX > bx) & (touchX < bx+buttonWidth) & (touchY > by) & (touchY < by+buttonHeight));
}


void setVolume(int vol)
{
  char volStr[10];
  sprintf(volStr,"V%d",vol);
  sendFifo(volStr);
  setForeColour(0,255,0);
  textSize=2;
  gotoXY(volButtonX+30,volButtonY-25);
  displayStr("   ");
  gotoXY(volButtonX+30,volButtonY-25);
  sprintf(volStr,"%d",vol);
  displayStr(volStr);
}

void setSquelch(int sql)
{
  char sqlStr[10];
  sprintf(sqlStr,"Z%d",sql);
  sendFifo(sqlStr);
  if(mode==4)
  {
  setForeColour(0,255,0);
  textSize=2;
  gotoXY(sqlButtonX+30,sqlButtonY-25);
  displayStr("   ");
  gotoXY(sqlButtonX+30,sqlButtonY-25);
  sprintf(sqlStr,"%d",sql);
  displayStr(sqlStr);
  }
}

void setInputMode(int m)

{
if(inputMode==1)
  {
  gotoXY(settingX,settingY);
  setForeColour(255,255,255);
  displayStr("                                ");
  writeConfig();
  displayMenu();
  }
if(inputMode==2)
  {
    gotoXY(volButtonX,volButtonY);
    setForeColour(0,255,0);
    displayButton("Vol");
  }
if(inputMode==3)
  {
    gotoXY(sqlButtonX,sqlButtonY);
    setForeColour(0,255,0);
    displayButton("SQL"); 
  }
if(inputMode==4)
  {
    gotoXY(ritButtonX,ritButtonY);
    setForeColour(0,255,0);
    displayButton("RIT");
    gotoXY(ritButtonX,ritButtonY+buttonSpaceY);
    setForeColour(0,0,0);
    displayButton("Zero");
  }
  
inputMode=m;

if(inputMode==1)
  {
    gotoXY(funcButtonsX,funcButtonsY);
    setForeColour(0,255,0);
    displayButton("MENU");
    displayButton(" ");
    displayButton("NEXT");
    displayButton(" ");
    displayButton("PREV");
    displayButton(" ");
    setForeColour(255,0,0);
    displayButton("OFF");
    mouseScroll=0;
    displaySetting(settingNo); 
  }
if(inputMode==2)
  {
    gotoXY(volButtonX,volButtonY);
    setForeColour(255,0,0);
    displayButton("Vol");
  }
if(inputMode==3)
  {
    gotoXY(sqlButtonX,sqlButtonY);
    setForeColour(255,0,0);
    displayButton("SQL"); 
  }
if(inputMode==4)
  {
    gotoXY(ritButtonX,ritButtonY);
    setForeColour(255,0,0);
    displayButton("RIT");
    gotoXY(ritButtonX,ritButtonY+buttonSpaceY);
    setForeColour(255,0,0);
    displayButton("Zero");
  }

}

void setRit(int ri)
{
  char ritStr[10];
  int to;
  if(mode!=4)
  {
  rit=ri;
  setForeColour(0,255,0);
  textSize=2;
  to=0;
  if(abs(rit)>0) to=8;
  if(abs(rit)>9) to=16;
  if(abs(rit)>99) to=24;
  if(abs(rit)>999) to=32; 
  gotoXY(ritButtonX,ritButtonY-25);
  displayStr("         ");
  gotoXY(ritButtonX+38-to,ritButtonY-25);
  if (rit==0)
  {
  sprintf(ritStr,"0");
  }
  else
  {
    sprintf(ritStr,"%+d",rit);
  }
  displayStr(ritStr);
  setFreq(freq);
  }
}


void setSSBMic(int mic)
{
  char micStr[10];
  sprintf(micStr,"G%d",mic);
  sendFifo(micStr);
}

void setFMMic(int mic)
{
  char micStr[10];
  sprintf(micStr,"g%d",mic);
  sendFifo(micStr);
}

void setKey(int k)
{
if(k==0) sendFifo("k"); else sendFifo("K");
}

void setFFTPipe(int ctrl)
{
if(ctrl==0) sendFifo("p"); else sendFifo("P");
}

void setMoni(int m)
{
  if(m==1)
    {
     sendFifo("M");
     moni=1;
     gotoXY(moniX,moniY);
     textSize=2;
     setForeColour(0,255,0);
     displayStr("MONI");
    } 
  else
    {
     sendFifo("m");
     moni=0;
     gotoXY(moniX,moniY);
     textSize=2;
     displayStr("    ");
    }
}

void setMode(int md)
{
  gotoXY(modeX,modeY);
  setForeColour(255,255,0);
  textSize=2;
  displayStr(modename[md]);
  if(md==0)
    {
    sendFifo("U");    //USB
    setFreq(freq);    //set the frequency to adjust for CW offset.
    sqlButton(0);
    ritButton(1);
    setRit(0);
    } 
  
  if(md==1)
    {
    sendFifo("L");    //USB
    setFreq(freq);    //set the frequency to adjust for CW offset.
    sqlButton(0);
    ritButton(1);
    setRit(0);
    } 
  
  if(md==2)
    {
    sendFifo("C");    //CW
    setFreq(freq);    //set the frequency to adjust for CW offset.
    sendFifo("W");    //wide CW Filter
    sqlButton(0);
    ritButton(1);
    setRit(0);
    } 
    
  if(md==3)
    {
    sendFifo("C");    //CW
    setFreq(freq);    //set the frequency to adjust for CW offset.
    sendFifo("N");    //Narrow CW Filter
    sqlButton(0);
    ritButton(1);
    setRit(0);
    } 
  if(md==4)
    {
    sendFifo("F");    //FM
    setFreq(freq);    //set the frequency to adjust for CW offset.
    sqlButton(1);
    ritButton(0);
    setRit(0);
    } 



}

void setTx(int pt)
{
  gotoXY(txX,txY);
  textSize=2;
  if(pt)
    {
      digitalWrite(txPin,HIGH);
      usleep(TXDELAY);
      setHwTxFreq(freq);
      PlutoTxEnable(1);
      if(satMode()==0)
      {
        setFFTPipe(0);                        //turn off the FFT stream
        setHwRxFreq(freq+10.0);               //offset the Rx frequency to prevent unwanted mixing. (happens even if disabled!) 
        PlutoRxEnable(0);
      }

      sendFifo("T");
      setForeColour(255,0,0);
      displayStr("Tx");  
    }
  else
    {
      sendFifo("R");
      PlutoTxEnable(0);
      PlutoRxEnable(1);
      setFFTPipe(1);                //turn on the FFT Stream
      setHwRxFreq(freq);
      setForeColour(0,255,0);
      displayStr("Rx");
      usleep(RXDELAY);
      digitalWrite(txPin,LOW);
    }
}

void setHwRxFreq(double fr)
{
  long long rxoffsethz;
  long long LOrxfreqhz;
  long long rxfreqhz;
  double frRx;
  double frTx;
  
  frRx=fr-bandRxOffset[band];
  
  rxfreqhz=frRx*1000000;
  
  rxoffsethz=(rxfreqhz % 100000)+50000;        //use just the +50Khz to +150Khz positive side of the sampled spectrum. This avoids the DC hump .
  
  LOrxfreqhz=rxfreqhz-rxoffsethz;
  rxoffsethz=rxoffsethz+rit;
  if((mode==2)|(mode==3))
    {
     rxoffsethz=rxoffsethz-800;         //offset  for CW tone of 800 Hz    
    }
  if(LOrxfreqhz!=lastLOhz)         
    {
      setPlutoRxFreq(LOrxfreqhz);          //Control Pluto directly to bypass problems with Gnu Radio Sink
      lastLOhz=LOrxfreqhz;
    }
  
  char offsetStr[32];
  sprintf(offsetStr,"O%d",rxoffsethz);   //send the rx offset tuning value 
  sendFifo(offsetStr);
}

void setHwTxFreq(double fr)
{
  long long txfreqhz;
  double frTx;
  
  frTx=fr-bandTxOffset[band];
  
  txfreqhz=frTx*1000000;
    
  if((mode==2)|(mode==3))
    {
     txfreqhz=txfreqhz-800;         //offset  for CW tone of 800 Hz    
    }
  
      setPlutoTxFreq(txfreqhz);          //Control Pluto directly to bypass problems with Gnu Radio Sink
}

void setFreq(double fr)
{
  long long freqhz;
  char digit[16]; 
  
  if(ptt | ptts) 
  {
    setHwTxFreq(fr);        //set Hardware Tx frequency if we are transmitting
  }
  else
  {
  setHwRxFreq(fr);       //set Hardware Rx frequency if we are receiving
  }

  fr=fr+0.0000001;   // correction for rounding errors.
  freqhz=fr*1000000;    
  freqhz=freqhz+100000000000;     //force it to be 12 digits long
  sprintf(digit,"%lld",freqhz);
  
  gotoXY(freqDisplayX,freqDisplayY);
  setForeColour(0,0,255);
  textSize=6;
  if(digit[1]>'0') displayChar(digit[1]); else displayChar(' ');
  if((digit[1]>'0') | (digit[2]>'0')) displayChar(digit[2]); else displayChar(' ');
  if((digit[1]>'0') | (digit[2]>'0') | digit[3]>'0')  displayChar(digit[3]); else displayChar(' ');
  displayChar(digit[4]);
  displayChar(digit[5]);
  displayChar('.');
  displayChar(digit[6]);
  displayChar(digit[7]);
  displayChar(digit[8]);
  displayChar('.');
  displayChar(digit[9]);
  displayChar(digit[10]);
  
// Underline the currently selected tuning digit
  
  for(int dtd=0;dtd<12;dtd++)
    {
      gotoXY(freqDisplayX+dtd*freqDisplayCharWidth+4,freqDisplayY+freqDisplayCharHeight+5);
      int bb = 0;
      if (dtd==tuneDigit) bb=255;
      for(int p=0; p < freqDisplayCharWidth; p++)
        {
          setPixel(currentX+p,currentY+1,0,0,bb);
          setPixel(currentX+p,currentY+2,0,0,bb);
          setPixel(currentX+p,currentY+3,0,0,bb);
        }
    }

//set TXVTR or SAT indication if needed.
  gotoXY(txvtrX,txvtrY);
  setForeColour(0,255,0);
  textSize=2;
  if( txvtrMode()==1)
    {
     displayStr(" TXVTR "); 
    }
  else if(satMode()==1)
    {
    displayStr("  SAT  ");
    }
  else
    {
      displayStr("       ");
    } 
 
   gotoXY(funcButtonsX+buttonSpaceX*4,funcButtonsY);
   setForeColour(0,255,0);
   if(inputMode==0)
   {
     if(satMode()==1)
      {
       displayButton("MONI");
      }  
      else
      {
       displayButton("    ");
      }
    }
     
}

int satMode(void)
{
if(abs(bandTxOffset[band]-bandRxOffset[band]) > 10 )      // if we have a differnt Rx and Tx offset then we must be in Sat mode. 
  {
  return 1;
  }
else
  {
  return 0;
  }
}

int txvtrMode(void)
{
if((abs(bandTxOffset[band]-bandRxOffset[band]) <1) & (abs(bandTxOffset[band]) >1 )  )       //if the tx and rx offset are the same and non zero then we are in Transverter mode
  {
  return 1;
  }
else
  {
  return 0;
  }
}


void setBandBits(int b)
{
if(b & 0x01) 
    {
    digitalWrite(bandPin1,HIGH);
    }
else
    {
    digitalWrite(bandPin1,LOW);
    }
    
if(b & 0x02) 
    {
    digitalWrite(bandPin2,HIGH);
    }
else
    {
    digitalWrite(bandPin2,LOW);
    }   

if(b & 0x04) 
    {
    digitalWrite(bandPin3,HIGH);
    }
else
    {
    digitalWrite(bandPin3,LOW);
    }   

if(b & 0x08) 
    {
    digitalWrite(bandPin4,HIGH);
    }
else
    {
    digitalWrite(bandPin4,LOW);
    }   
}


void changeSetting(void)
{
  if(settingNo==0)        //SSB Mic Gain
      {
      SSBMic=SSBMic+mouseScroll;
      mouseScroll=0;
      if(SSBMic<0) SSBMic=0;
      if(SSBMic>maxSSBMic) SSBMic=maxSSBMic;
      setSSBMic(SSBMic);
      displaySetting(settingNo);
      }
   if(settingNo==1)        // FM Mic Gain
      {
      FMMic=FMMic+mouseScroll;
      mouseScroll=0;
      if(FMMic<0) FMMic=0;
      if(FMMic>maxFMMic) FMMic=maxFMMic;
      setFMMic(FMMic);
      displaySetting(settingNo);
      }
   if(settingNo==2)        //Transverter Rx Offset 
      {
        bandRxOffset[band]=bandRxOffset[band]+mouseScroll*freqInc;
        displaySetting(settingNo);
        freq=freq+mouseScroll*freqInc;
        if(freq>maxFreq) freq=maxFreq;
        if(freq<minFreq) freq=minFreq;
        mouseScroll=0;
        setFreq(freq);
      } 
   if(settingNo==3)        //Transverter Tx Offset
      {
        bandTxOffset[band]=bandTxOffset[band]+mouseScroll*freqInc;
        displaySetting(settingNo);
        mouseScroll=0;
        setFreq(freq);
      }  
   if(settingNo==4)        // Band Bits
      {
      bandBits[band]=bandBits[band]+mouseScroll;
      mouseScroll=0;
      if(bandBits[band]<0) bandBits[band]=0;
      if(bandBits[band]>15) bandBits[band]=15;
      bbits=bandBits[band];
      setBandBits(bbits);
      displaySetting(settingNo);  
      }    
   if(settingNo==5)        // FFT Ref Level
      {
      FFTRef=FFTRef+mouseScroll;
      mouseScroll=0;
      if(FFTRef<-50) FFTRef=-50;
      if(FFTRef>0) FFTRef=0;
      setFFTRef(FFTRef);
      displaySetting(settingNo);  
      }                                   
}

               

void displaySetting(int se)
{
  char valStr[30];
  gotoXY(settingX,settingY);
  setForeColour(255,255,255);
  displayStr("                                ");
  gotoXY(settingX,settingY);
  displayStr(settingText[se]);
 
 if(se==0)
  {
  sprintf(valStr,"%d",SSBMic);
  displayStr(valStr);
  }
 if(se==1)
  {
  sprintf(valStr,"%d",FMMic);
  displayStr(valStr);
  }
  if(se==2)
  {
  sprintf(valStr,"%f",bandRxOffset[band]);
  displayStr(valStr);
  }
  
  if(se==3)
  {
  sprintf(valStr,"%f",bandTxOffset[band]);
  displayStr(valStr);
  }
  
  if(se==4)
  {
    if(bbits==0)  sprintf(valStr,"0000"); 
    if(bbits==1)  sprintf(valStr,"0001");
    if(bbits==2)  sprintf(valStr,"0010"); 
    if(bbits==3)  sprintf(valStr,"0011"); 
    if(bbits==4)  sprintf(valStr,"0100"); 
    if(bbits==5)  sprintf(valStr,"0101"); 
    if(bbits==6)  sprintf(valStr,"0110"); 
    if(bbits==7)  sprintf(valStr,"0111"); 
    if(bbits==8)  sprintf(valStr,"1000"); 
    if(bbits==9)  sprintf(valStr,"1001"); 
    if(bbits==10)  sprintf(valStr,"1010");
    if(bbits==11)  sprintf(valStr,"1011"); 
    if(bbits==12)  sprintf(valStr,"1100"); 
    if(bbits==13)  sprintf(valStr,"1101"); 
    if(bbits==14)  sprintf(valStr,"1110"); 
    if(bbits==15)  sprintf(valStr,"1111");                          
  displayStr(valStr);
  }
  if(se==5)
  {
  sprintf(valStr,"%d",FFTRef);
  displayStr(valStr);
  }
}

int readConfig(void)
{
FILE * conffile;
char variable[80];
char value[20];

conffile=fopen("/home/pi/Langstone/Langstone.conf","r");

if(conffile==NULL)
  {
    return -1;
  }

while(fscanf(conffile,"%s %s [^\n]\n",variable,value) !=EOF)
  {
    if(strstr(variable,"bandFreq0")) sscanf(value,"%lf",&bandFreq[0]);
    if(strstr(variable,"bandTxOffset0")) sscanf(value,"%lf",&bandTxOffset[0]);
    if(strstr(variable,"bandRxOffset0")) sscanf(value,"%lf",&bandRxOffset[0]);
    if(strstr(variable,"bandBits0")) sscanf(value,"%d",&bandBits[0]); 
    if(strstr(variable,"bandFreq1")) sscanf(value,"%lf",&bandFreq[1]);
    if(strstr(variable,"bandTXOffset1")) sscanf(value,"%lf",&bandTxOffset[1]);  
    if(strstr(variable,"bandRxOffset1")) sscanf(value,"%lf",&bandRxOffset[1]);
    if(strstr(variable,"bandBits1")) sscanf(value,"%d",&bandBits[1]); 
    if(strstr(variable,"bandFreq2")) sscanf(value,"%lf",&bandFreq[2]);
    if(strstr(variable,"bandTxOffset2")) sscanf(value,"%lf",&bandTxOffset[2]);  
    if(strstr(variable,"bandRxOffset2")) sscanf(value,"%lf",&bandRxOffset[2]);
    if(strstr(variable,"bandBits2")) sscanf(value,"%d",&bandBits[2]); 
    if(strstr(variable,"bandFreq3")) sscanf(value,"%lf",&bandFreq[3]);
    if(strstr(variable,"bandTxOffset3")) sscanf(value,"%lf",&bandTxOffset[3]);  
    if(strstr(variable,"bandRxOffset3")) sscanf(value,"%lf",&bandRxOffset[3]);
    if(strstr(variable,"bandBits3")) sscanf(value,"%d",&bandBits[3]); 
    if(strstr(variable,"bandFreq4")) sscanf(value,"%lf",&bandFreq[4]);
    if(strstr(variable,"bandTxOffset4")) sscanf(value,"%lf",&bandTxOffset[4]);  
    if(strstr(variable,"bandRxOffset4")) sscanf(value,"%lf",&bandRxOffset[4]);
    if(strstr(variable,"bandBits4")) sscanf(value,"%d",&bandBits[4]); 
    if(strstr(variable,"bandFreq5")) sscanf(value,"%lf",&bandFreq[5]);
    if(strstr(variable,"bandTxOffset5")) sscanf(value,"%lf",&bandTxOffset[5]);  
    if(strstr(variable,"bandRxOffset5")) sscanf(value,"%lf",&bandRxOffset[5]);
    if(strstr(variable,"bandBits5")) sscanf(value,"%d",&bandBits[5]); 
    if(strstr(variable,"bandFreq6")) sscanf(value,"%lf",&bandFreq[6]);
    if(strstr(variable,"bandTxOffset6")) sscanf(value,"%lf",&bandTxOffset[6]);  
    if(strstr(variable,"bandRxOffset6")) sscanf(value,"%lf",&bandRxOffset[6]);
    if(strstr(variable,"bandBits6")) sscanf(value,"%d",&bandBits[6]); 
    if(strstr(variable,"bandFreq7")) sscanf(value,"%lf",&bandFreq[7]);
    if(strstr(variable,"bandTxffset7")) sscanf(value,"%lf",&bandTxOffset[7]); 
    if(strstr(variable,"bandRxOffset7")) sscanf(value,"%lf",&bandRxOffset[7]);
    if(strstr(variable,"bandBits7")) sscanf(value,"%d",&bandBits[7]); 
    if(strstr(variable,"bandFreq8")) sscanf(value,"%lf",&bandFreq[8]);
    if(strstr(variable,"bandTxOffset8")) sscanf(value,"%lf",&bandTxOffset[8]);  
    if(strstr(variable,"bandRxOffset8")) sscanf(value,"%lf",&bandRxOffset[8]);
    if(strstr(variable,"bandBits8")) sscanf(value,"%d",&bandBits[8]); 
    if(strstr(variable,"bandFreq9")) sscanf(value,"%lf",&bandFreq[9]);
    if(strstr(variable,"bandTxOffset9")) sscanf(value,"%lf",&bandTxOffset[9]);  
    if(strstr(variable,"bandRxOffset9")) sscanf(value,"%lf",&bandRxOffset[9]);
    if(strstr(variable,"bandBits9")) sscanf(value,"%d",&bandBits[9]); 

    if(strstr(variable,"currentBand")) sscanf(value,"%d",&band);
    if(strstr(variable,"tuneDigit")) sscanf(value,"%d",&tuneDigit);   
    if(strstr(variable,"mode")) sscanf(value,"%d",&mode);
    if(strstr(variable,"SSBMic")) sscanf(value,"%d",&SSBMic);
    if(strstr(variable,"FMMic")) sscanf(value,"%d",&FMMic);
    if(strstr(variable,"volume")) sscanf(value,"%d",&volume);
    if(strstr(variable,"squelch")) sscanf(value,"%d",&squelch);
    if(strstr(variable,"FFTRef")) sscanf(value,"%d",&FFTRef);

            
  }

fclose(conffile);
return 0;

}



int writeConfig(void)
{
FILE * conffile;
char variable[80];
int value;

conffile=fopen("/home/pi/Langstone/Langstone.conf","w");

if(conffile==NULL)
  {
    return -1;
  }

fprintf(conffile,"bandFreq0 %lf\n",bandFreq[0]);
fprintf(conffile,"bandTxOffset0 %lf\n",bandTxOffset[0]);
fprintf(conffile,"bandRxOffset0 %lf\n",bandRxOffset[0]);
fprintf(conffile,"bandBits0 %d\n",bandBits[0]);
fprintf(conffile,"bandFreq1 %lf\n",bandFreq[1]);
fprintf(conffile,"bandTxOffset1 %lf\n",bandTxOffset[1]);
fprintf(conffile,"bandRxOffset1 %lf\n",bandRxOffset[1]);
fprintf(conffile,"bandBits1 %d\n",bandBits[1]);
fprintf(conffile,"bandFreq2 %lf\n",bandFreq[2]);
fprintf(conffile,"bandTxOffset2 %lf\n",bandTxOffset[2]);
fprintf(conffile,"bandRxOffset2 %lf\n",bandRxOffset[2]);
fprintf(conffile,"bandBits2 %d\n",bandBits[2]);
fprintf(conffile,"bandFreq3 %lf\n",bandFreq[3]);
fprintf(conffile,"bandTxOffset3 %lf\n",bandTxOffset[3]);
fprintf(conffile,"bandRxOffset3 %lf\n",bandRxOffset[3]);
fprintf(conffile,"bandBits3 %d\n",bandBits[3]);
fprintf(conffile,"bandFreq4 %lf\n",bandFreq[4]);
fprintf(conffile,"bandTxOffset4 %lf\n",bandTxOffset[4]);
fprintf(conffile,"bandRxOffset4 %lf\n",bandRxOffset[4]);
fprintf(conffile,"bandBits4 %d\n",bandBits[4]);
fprintf(conffile,"bandFreq5 %lf\n",bandFreq[5]);
fprintf(conffile,"bandTxOffset5 %lf\n",bandTxOffset[5]);
fprintf(conffile,"bandRxOffset5 %lf\n",bandRxOffset[5]);
fprintf(conffile,"bandBits5 %d\n",bandBits[5]);
fprintf(conffile,"bandFreq6 %lf\n",bandFreq[6]);
fprintf(conffile,"bandTxOffset6 %lf\n",bandTxOffset[6]);
fprintf(conffile,"bandRxOffset6 %lf\n",bandRxOffset[6]);
fprintf(conffile,"bandBits6 %d\n",bandBits[6]);
fprintf(conffile,"bandFreq7 %lf\n",bandFreq[7]);
fprintf(conffile,"bandTxOffset7 %lf\n",bandTxOffset[7]);
fprintf(conffile,"bandRxOffset7 %lf\n",bandRxOffset[7]);
fprintf(conffile,"bandBits7 %d\n",bandBits[7]);
fprintf(conffile,"bandFreq8 %lf\n",bandFreq[8]);
fprintf(conffile,"bandTxOffset8 %lf\n",bandTxOffset[8]);
fprintf(conffile,"bandRxOffset8 %lf\n",bandRxOffset[8]);
fprintf(conffile,"bandBits8 %d\n",bandBits[8]);
fprintf(conffile,"bandFreq9 %lf\n",bandFreq[9]);
fprintf(conffile,"bandTxOffset9 %lf\n",bandTxOffset[9]);
fprintf(conffile,"bandRxOffset9 %lf\n",bandRxOffset[9]);
fprintf(conffile,"bandBits9 %d\n",bandBits[9]);

fprintf(conffile,"currentBand %d\n",band);
fprintf(conffile,"tuneDigit %d\n",tuneDigit);
fprintf(conffile,"mode %d\n",mode);
fprintf(conffile,"SSBMic %d\n",SSBMic);
fprintf(conffile,"FMMic %d\n",FMMic);
fprintf(conffile,"volume %d\n",volume);
fprintf(conffile,"squelch %d\n",squelch);
fprintf(conffile,"FFTRef %d\n",FFTRef);

fclose(conffile);
return 0;

}
