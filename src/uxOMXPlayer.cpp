#include "uxOMXPlayer.h"

//--------------------------------------------------------------
void uxOMXPlayer::init(bool audioHDMI, bool glsl)
{			
	//GLSL CONFIG 
	enableGLSL = glsl;
	
	//SCAN VIDEO FOLDER
	currentIndex = 0;
	videoFiles.clear();
	enableLoopingList = false;

	//SHADER AND FRAME BUFFER LOAD
	if (enableGLSL) 
	{
		shader.load("shader/shaderExample");
		fbo.allocate(ofGetWidth(), ofGetHeight());
	}
	
	//OMXPLAYER Settings
	currentVolume = 0.5;
	muteVolume = false;
	this->settings.doFlipTexture		= false; //true on older firmware
	this->settings.useHDMIForAudio 		= audioHDMI;
	this->settings.enableTexture 		= true;
	this->settings.enableLooping 		= false;	
	this->settings.videoPath 			= "";
	
	ofLog(OF_LOG_NOTICE,"-HP- omxPlayer initialized");
}

//--------------------------------------------------------------
void uxOMXPlayer::display(){
	
	if (!this->isPlaying()) return;
	
	//WIDTH
	if (this -> getHeight() > 0) this->dim.width = floor( ofGetHeight() * this->getWidth() / this -> getHeight() );
	else this->dim.width = 0;
	if (this->dim.width > ofGetWidth()) this->dim.width = ofGetWidth();
	
	//HEIGHT
	if (this -> getWidth() > 0) this->dim.height = floor( width * this -> getHeight() / this->getWidth() );
	else this->dim.height = 0;
	
	//DRAW IF DIMS ARE VALID
	if ((this->dim.width > 0) && (this->dim.height > 0))
	{
		//MARGINS
		this->dim.marginX = floor((ofGetWidth()-this->dim.width)/2);
		this->dim.marginY = floor((ofGetHeight()-this->dim.height)/2);
	
		//DRAW VIDEO
		if (enableGLSL)
		{
			fbo.begin();
				shader.begin();
					//Here we tell pass our shader some changing values
			
					//We pass our texture id 
					shader.setUniformTexture("tex0", this->getTextureReference(), this->getTextureID());
			
					//We give it an incrementing value to use
					shader.setUniform1f("time", ofGetElapsedTimef());
			
					//And a resolution
					shader.setUniform2f("resolution", ofGetWidth(), ofGetHeight());
	
					//We then send our texture that kicks it off
					this->draw(this->dim.marginX, this->dim.marginY, this->dim.width, this->dim.height);
				shader.end();
			fbo.end();
		
			fbo.draw(0, 0);
		}
		else this->draw(this->dim.marginX, this->dim.marginY, this->dim.width, this->dim.height);
	}
	else 
	{
		this->dim.marginX = 0;
		this->dim.marginY = 0;
	}
}

/*PLAYLIST*/

//--------------------------------------------------------------
void uxOMXPlayer::play(vector<ofFile> playlist, bool loop)
{

	this->videoFiles = playlist;
	this->enableLoopingList = loop;
	this->play(0);
}


//--------------------------------------------------------------
void uxOMXPlayer::play(int index){

	if ((index >= 0) && (index < videoFiles.size())) 
	{
		currentIndex = index;
		this->play(videoFiles[currentIndex].path(),false);
	}
}

/*PLAY SINGLE FILE*/

//--------------------------------------------------------------
void uxOMXPlayer::play(string filepath, bool loop){

	this->settings.enableLooping = loop;
	this->play(filepath);
}

/*PLAY COMMON*/

//--------------------------------------------------------------
void uxOMXPlayer::play(string filepath){
	
	ofFile file(filepath);	
	if (!file.isFile()) 
	{
		ofLog(OF_LOG_NOTICE,"-HP- file not found: "+filepath);
		return;
	}
	
	this->settings.videoPath = file.path();
		
	if (this->isOpen) this->stop(); 
	this->setup(this->settings);
		
	this->volume();	
	this->setPaused(false);
	
	ofLog(OF_LOG_NOTICE,"-HP- play "+this->settings.videoPath);
}

//--------------------------------------------------------------
void uxOMXPlayer::next(){

	int index = currentIndex+1;
	if (index >= videoFiles.size()) index = 0;
	this->play(index);
}

//--------------------------------------------------------------
void uxOMXPlayer::stop(){
	
	this->close();
	this->getTextureReference().clear();
	
	ofLog(OF_LOG_NOTICE,"-HP- stop ");
}

//--------------------------------------------------------------
void uxOMXPlayer::pause(){
	this->setPaused(true);
	
	ofLog(OF_LOG_NOTICE,"-HP- pause ");
}

//--------------------------------------------------------------
void uxOMXPlayer::resume(){
	this->setPaused(false);
	
	ofLog(OF_LOG_NOTICE,"-HP- resume ");
}

//--------------------------------------------------------------
bool uxOMXPlayer::autoloop(){
	return this->settings.enableLooping;
	//TODO GERER LE LOOPING SELON LIST OU PAS (attention le onVideoLoop dépend de ça!!)
}


//--------------------------------------------------------------
void uxOMXPlayer::volume(){
	this->volume(currentVolume);
}

//--------------------------------------------------------------
void uxOMXPlayer::volume(int v){
	this->volume(v/100.0);
}


//--------------------------------------------------------------
void uxOMXPlayer::volume(double v){
	if (v > 1.0) v=v/100.0;
	currentVolume = v;
		
	if (muteVolume) this->setVolume(0.0);
	else this->setVolume(currentVolume);

	ofLog(OF_LOG_NOTICE,"-HP- volume "+ofToString(this->getVolumeInt()));
}

//--------------------------------------------------------------
bool uxOMXPlayer::isMuted(){
	return muteVolume;
}

//--------------------------------------------------------------
void uxOMXPlayer::setMuted(bool mute){
	muteVolume = mute;
	this->volume();
}

//--------------------------------------------------------------
int uxOMXPlayer::getVolumeInt(){
	return static_cast<int>(currentVolume*100.0);
}

//--------------------------------------------------------------
string uxOMXPlayer::getFile(){
	return this->settings.videoPath;
}

//--------------------------------------------------------------
int uxOMXPlayer::getPositionMs(){
	return static_cast<int>(this->getCurrentFrame() * this->getDurationMs() / this->getTotalNumFrames());
}

//--------------------------------------------------------------
int uxOMXPlayer::getDurationMs(){
	return static_cast<int>(this->getDuration()*1000);
}


