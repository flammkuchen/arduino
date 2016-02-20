#include <SD.h>
#include <SPI.h>
#include <MusicPlayer.h>

//the time we give the sensor to calibrate (10-60 secs according to the datasheet)
int calibrationTime = 2; //30;        

//the time when the sensor outputs a high impulse
long unsigned int switchToHighTime;

//the time when the music stopped for the last time. In the time interval 
//[endOfPlayingMusicTime ---- endOfPlayingMusicTime + musicPauseDuration]   music won't be played back
//even if motion is detected
long unsigned int endOfPlayingMusicTime;

//the amount of milliseconds the sensor has to be high 
//before we assume a motion has happened
long unsigned int pause = 3000;  

//the amount of milliseconds the music should play
long unsigned int musicDuration = 10 * 1000; //2 * 60 * 1000

//the amount of seconds the music should pause before playing again
//this is to avoid to trigger the music several times if one person is in the bathroom
long unsigned int musicPauseDuration = 30;//10 * 60;

//the time when the music started playing
long unsigned int musicStartTime;

//is the music playing?
boolean playingMusic = false;
//has the music been played and we are in the music pause phase?
boolean playedMusic = false;

boolean newState = LOW;
boolean lastState = LOW;

boolean testing = true;

int pirPin = 2;    //the digital pin connected to the PIR sensor's output
int ledPin = 13;


/////////////////////////////
//SETUP
void setup()
{
  Serial.begin(9600);
  pinMode(pirPin, INPUT);
  pinMode(ledPin, OUTPUT);
  digitalWrite(pirPin, LOW);
  settingsSetup();
  calibrate();
  soundSetup();
  
}

void settingsSetup()
{
  if (testing)
  {
    calibrationTime = 2; //2s
    pause = 3000; //3s
    musicDuration = 10 * 1000; //10s
    musicPauseDuration = 30; //30s
  }
  else
  {
    calibrationTime = 30; //30s
    pause = 3000; //3s
    musicDuration = 30 * 1000; //30s
    musicPauseDuration = 10 * 60; //10 minutes
  }  
}

//give the sensor some time to calibrate. The led is blinking.
void calibrate()
{
  Serial.print("calibrating PIR sensor ");
  for(int i = 0; i < calibrationTime; i++){
    Serial.print(".");
    digitalWrite(ledPin, HIGH);
    delay(500);
    digitalWrite(ledPin, LOW);
    delay(500);
    }
  Serial.println(" done");
  Serial.println("SENSOR ACTIVE");
}

void soundSetup()
{
  player.keyDisable(); //keys disable first;
  player.digitalControlEnable();
  player.begin();                      //will initialize the hardware and set default mode to be normal.
  //player.attachDigitOperation(2,playMusic,HIGH);
  player.setPlayMode(PM_NORMAL_PLAY);
  player.scanAndPlayAll(); //If the current playlist is empty,it will add all the songs in the root directory to the playlist.
}

////////////////////////////
//LOOP
void loop()
{  
  //always update the led to reflect the current state of the PIR sensor
  newState = digitalRead(pirPin);
  
  Serial.print("newState is ");
  Serial.println(newState);
  Serial.print("lastState is ");
  Serial.println(lastState);
  
  if (newState != lastState)
    digitalWrite(ledPin, newState);  
 
  //when the music is playing and not yet finished, do nothing but play
  if (playingMusic)
  {
    player.play();
    if (millis() - musicStartTime < musicDuration)
    {
      //TODO
      //player.play();
      //Serial.println("music is playing...");
//      Serial.print("millis is ");
//      Serial.println(millis());
//      Serial.print("musicStartTime is ");
//      Serial.println(musicStartTime);
//      Serial.print("musicDuration is ");
//      Serial.println(musicDuration);
      //delay(50);
    }
    else
    {
      //TODO:stop the music
      //player.play();
      stopMusic();
      playingMusic = false;
    }
    return;
  }
  
//  Serial.print("millis is ");
//  Serial.println(millis());
//  Serial.print("endOfPlayingMusicTime is ");
//  Serial.println(endOfPlayingMusicTime);
//  Serial.print("musicPauseDuration in sec is ");
//  Serial.println(musicPauseDuration);
  if (playedMusic) {  
    //are we in the pause time after playing the song
    if (millis() - endOfPlayingMusicTime < musicPauseDuration * 1000)
    {
      //if this case, do not evaluate the motion sensor at all
      return;
    }
    //end of music pause has been reached
    playedMusic = false;
  }
  
  if(newState == HIGH)
  {
    if(lastState == LOW)
    {  
      //switch from low to high has just happened
      //store the timestamp and output switch
      //lastState = HIGH;
      switchToHighTime = millis();     
      Serial.print("motion detected at ");
      Serial.print(switchToHighTime/1000);
      Serial.println(" sec"); 
      //delay(50);
    }
    else
    {
      // motion is still detected
      if (!playingMusic && (millis() - switchToHighTime > pause))
      {
        //motion detected discontinously for enough time
        //trigger music
        playMusic();
        playingMusic = true;
      }
    }  
  }
  else
  {
    if (lastState == HIGH)
    {
      Serial.print("motion lost at ");
      Serial.print(millis()/1000);
      Serial.println(" sec");  
    }    
  }
  lastState = newState;
}

void playMusic()
{
  Serial.print("now playing phantom of the opera at ");
  Serial.print(millis()/1000);
  Serial.println(" sec"); 
  musicStartTime = millis();
  //TODO: play music
  player.opPlay();
  playingMusic = 1;
  
}

void stopMusic()
{
  Serial.print("now stopping phantom of the opera at ");
  Serial.print(millis()/1000);
  Serial.println(" sec"); 
  endOfPlayingMusicTime = millis();
  lastState = LOW;//reset state
  //TODO: stop/pause music
  player.opPause();
  playingMusic = 0;
  playedMusic = true;   
}
