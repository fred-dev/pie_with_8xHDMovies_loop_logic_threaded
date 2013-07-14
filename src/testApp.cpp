
#include "testApp.h"
#include "ofxGamepadHandler.h"

//--------------------------------------------------------------
void testApp::setup(){
	ofxGamepadHandler::get()->enableHotplug();
	
	//CHECK IF THERE EVEN IS A GAMEPAD CONNECTED
	if(ofxGamepadHandler::get()->getNumPads()>0){
		ofxGamepad* pad = ofxGamepadHandler::get()->getGamepad(0);
		ofAddListener(pad->onAxisChanged, this, &testApp::axisChanged);
		ofAddListener(pad->onButtonPressed, this, &testApp::buttonPressed);
		ofAddListener(pad->onButtonReleased, this, &testApp::buttonReleased);
	}
	ofEnableAlphaBlending();
    ofSetVerticalSync(true);
	for (int i=0; i<8; i++) {
        mask[i].loadImage("images/slice"+ ofToString(i+1) +".png");
        readyToPlay[i]=true;
        hasFirstLoop[i]=false;
        
        ofAddListener(recordedVideoPlayback[i].threadedVideoEvent, this, &testApp::threadedVideoEvent);
        recordedVideoPlayback[i].setUseQueue(true);
        recordedVideoPlayback[i].setUseTexture(true);
        
        }
    masterLoopRecorded=false;
    vidRecorder = ofPtr<ofQTKitGrabber>( new ofQTKitGrabber() );
    vidGrabber.setGrabber(vidRecorder);
    videoDevices = vidRecorder->listVideoDevices();
    audioDevices = vidRecorder->listAudioDevices();
    //vidRecorder->setUseAudio(false);
	ofAddListener(vidRecorder->videoSavedEvent, this, &testApp::videoSaved);

    vidGrabber.listDevices();
    vidGrabber.setDeviceID(2);
    vidRecorder->setAudioDeviceID(2);
    vidGrabber.initGrabber(1920,1080);
    vidRecorder->initGrabberWithoutPreview();
    
    videoCodecs = vidRecorder->listVideoCodecs();
    for(size_t i = 0; i < videoCodecs.size(); i++){
        ofLogVerbose("Available Video Codecs") << videoCodecs[i];
    }
    
    audioCodecs = vidRecorder->listAudioCodecs();
    for(size_t i = 0; i < audioCodecs.size(); i++){
        ofLogVerbose("Available Video Codecs") << videoCodecs[i];
    }
    
    //vidRecorder->setVideoCodec("QTCompressionOptionsHD1080SizeH264Video");
    //vidRecorder->setVideoCodec("QTCompressionOptionsLosslessAnimationVideo");
    vidRecorder->setVideoCodec("QTCompressionOptionsLosslessAppleIntermediateVideo");
    //vidRecorder->setVideoCodec("QTCompressionOptionsJPEGVideo");
    vidRecorder->setAudioCodec("QTCompressionOptionsHighQualityAACAudio");
    vidRecorder->initRecording();
    sliceCount=0;
    masterSpeed=1;
	
}   

//--------------------------------------------------------------
void testApp::update(){
    //vidGrabber.update();
    for (int i =0; i<8; i++) {
        if (recordedVideoPlayback[i].isLoaded()==false) {
            recordedVideoPlayback[i].loadMovie("slice"+ofToString(i+1)+".mov");
            cout<<"we got here \n";
        }
        
        if(recordedVideoPlayback[i].isLoaded()){
            recordedVideoPlayback[i].update();
            cout << "video "+ ofToString(i)+ " is loaded\n";
            recordedVideoPlayback[i].setSpeed(masterSpeed);
            if (recordedVideoPlayback[i].getIsMovieDone()==true) {
                 readyToPlay[i]=true;
            }
            if (loopStamp[i]==recordedVideoPlayback[0].getCurrentFrame()&&readyToPlay[i]==true) {
                recordedVideoPlayback[i].play();
    
                readyToPlay[i]=false;
            }
        }
    }
}

//--------------------------------------------------------------
void testApp::draw(){
	//first draw the bottom layer
	//bottomLayer.draw(0, 0);
	ofBackground(0, 0, 0);
    for (int i=0; i<8; i++) {
        
        if(recordedVideoPlayback[i].isLoaded()==true ){
            //&& isReRecording[i]==false
            cout<<"it should play video"+ofToString(i+1)+"\n";
            maskShader[i].load("composite");
            maskShader[i].begin();
            maskShader[i].setUniformTexture("Tex0", recordedVideoPlayback[i].getTextureReference(), 0);
            maskShader[i].setUniformTexture("Tex1", mask[i].getTextureReference(), 1);
            maskShader[i].end();
            maskShader[i].begin();
            //our shader uses two textures, the top layer and the alpha
            //we can load two textures into a shader using the multi texture coordinate extensions
            glActiveTexture(GL_TEXTURE0_ARB);
            recordedVideoPlayback[i].getTextureReference().bind();
            
            glActiveTexture(GL_TEXTURE1_ARB);
            mask[i].getTextureReference().bind();
            
            //draw a quad the size of the frame
            glBegin(GL_QUADS);
            glMultiTexCoord2d(GL_TEXTURE0_ARB, 0, 0);
            glMultiTexCoord2d(GL_TEXTURE1_ARB, 0, 0);
            glVertex2f( 0, 0);
            
            glMultiTexCoord2d(GL_TEXTURE0_ARB, mask[i].getWidth(), 0);
            glMultiTexCoord2d(GL_TEXTURE1_ARB, mask[i].getWidth(), 0);
            glVertex2f( ofGetWidth(), 0);
            
            glMultiTexCoord2d(GL_TEXTURE0_ARB, mask[i].getWidth(), mask[i].getHeight());
            glMultiTexCoord2d(GL_TEXTURE1_ARB, mask[i].getWidth(), mask[i].getHeight());
            glVertex2f( ofGetWidth(), ofGetHeight());
            
            glMultiTexCoord2d(GL_TEXTURE0_ARB, 0, mask[i].getHeight());
            glMultiTexCoord2d(GL_TEXTURE1_ARB, 0, mask[i].getHeight());
            glVertex2f( 0, ofGetHeight() );
            
            glEnd();
            
            //deactive and clean up
            glActiveTexture(GL_TEXTURE1_ARB);
            mask[i].getTextureReference().unbind();
            
            glActiveTexture(GL_TEXTURE0_ARB);
            recordedVideoPlayback[i].getTextureReference().unbind();
            maskShader[i].end();
        }
    }

    string text=ofToString(ofGetFrameRate());
    ofDrawBitmapString(text, 10, 10);
	//then draw a quad for the top layer using our composite shader to set the alpha
	ofPushStyle();
    ofSetColor(255);
    ofDrawBitmapString("' ' space bar to toggle recording", 1600, 980);
    ofDrawBitmapString("'v' switches video device", 1600, 1000);
    ofDrawBitmapString("'a' swiches audio device", 1600, 1020);
    
    //draw video device selection
    ofDrawBitmapString("VIDEO DEVICE", 20, 540);
    for(int i = 0; i < videoDevices.size(); i++){
        if(i == vidRecorder->getVideoDeviceID()){
			ofSetColor(255, 100, 100);
        }
        else{
            ofSetColor(255);
        }
        ofDrawBitmapString(videoDevices[i], 20, 560+i*20);
    }
    
    //draw audio device;
    int startY = 580+20*videoDevices.size();
    ofDrawBitmapString("AUDIO DEVICE", 20, startY);
    startY += 20;
    for(int i = 0; i < audioDevices.size(); i++){
        if(i == vidRecorder->getAudioDeviceID()){
			ofSetColor(255, 100, 100);
        }
        else{
            ofSetColor(255);
        }
        ofDrawBitmapString(audioDevices[i], 20, startY+i*20);
    }
    ofPopStyle();
}
void testApp::startRecording(int sliceNumber){
    if(recordedVideoPlayback[sliceNumber].isLoaded()){
        recordedVideoPlayback[sliceNumber].stop();
        
        file.removeFile("Slice" + ofToString(sliceNumber+1)+ ".mov");
    }
    if (hasFirstLoop[sliceNumber]==true) {
        isReRecording[sliceNumber]=true;
    }
    if (hasFirstLoop[sliceNumber]==false) {
        hasFirstLoop[sliceNumber]=true;
    }
    
    loopStamp[sliceNumber]=recordedVideoPlayback[0].getCurrentFrame();
    vidRecorder->startRecording("Slice" + ofToString(sliceNumber+1)+ ".mov");
}

void testApp::stopRecording(int sliceNumber){
    sliceCount=sliceNumber;
    if(vidRecorder->isRecording()){
        vidRecorder->stopRecording();
        isReRecording[sliceNumber]=false;
    }
    if (sliceCount==0) {
        recordedVideoPlayback[sliceCount].loadMovie("Slice" + ofToString(sliceCount+1)+ ".mov");
        //recordedVideoPlayback[sliceCount].setUseTexture(true);
        recordedVideoPlayback[sliceCount].play();
        if (recordedVideoPlayback[sliceCount].isLoaded()) {
            cout<<"loaded master loop\n";
        }
        
    }
    if (sliceCount!=0) {
        recordedVideoPlayback[sliceCount].loadMovie("Slice" + ofToString(sliceCount+1)+ ".mov");
        //recordedVideoPlayback[sliceCount].setUseTexture(true);
       // recordedVideoPlayback[sliceCount].setLoopState(OF_LOOP_NONE);
        
        
        
    }
    
}
//--------------------------------------------------------------
void testApp::keyPressed(int key){
    if (key=='1') {
        if(masterLoopRecorded==true){
            for (int i=0; i<8; i++) {
                recordedVideoPlayback[i].stop();
               // recordedVideoPlayback[i].closeMovie();
                file.removeFile("Slice" + ofToString(i+1)+ ".mov");
                hasFirstLoop[i]=false;
            }
            
            file.removeFile("Slice1.mov");
        }
        
        vidRecorder->startRecording("Slice1.mov");
        cout << "video "+ ofToString(1)+ " is recording\n";
    }
    if (key=='2') {
        startRecording(1);
    }
    if (key=='3') {
        startRecording(2);
    }
    if (key=='4') {
        startRecording(3);
    }
    if (key=='5') {
        startRecording(4);
    }
    if (key=='6') {
        startRecording(5);
    }
    if (key=='7') {
        startRecording(6);
    }
    if (key=='8') {
        startRecording(7);
    }
}

//--------------------------------------------------------------
void testApp::keyReleased(int key){
    
    if (key=='1') {
        masterLoopRecorded=true;
        stopRecording(0);
    }
    if (key=='2') {
        stopRecording(1);
    }
    if (key=='3') {
        stopRecording(2);
    }
    if (key=='4') {
        stopRecording(3);
    }
    if (key=='5') {
        stopRecording(4);
    }
    if (key=='6') {
        stopRecording(5);
    }
    if (key=='7') {
        stopRecording(6);
    }
    if (key=='8') {
        stopRecording(7);
    }
    
    
	if(key == 'v'){
		vidRecorder->setVideoDeviceID( (vidRecorder->getVideoDeviceID()+1) % videoDevices.size() );
    }
	if(key == 'a'){
        vidRecorder->setAudioDeviceID( (vidRecorder->getAudioDeviceID()+1) % audioDevices.size() );
    }

}
void testApp::axisChanged(ofxGamepadAxisEvent& e)
{
	cout << "AXIS " << ofToString(e.axis) << " VALUE " << ofToString(e.value) << endl;
    if (e.axis==3) {
        if (e.value!=0) {
            masterSpeed = ofMap(e.value, -1.0, 1.0, 0.2, 4.0);
        }
        if (e.value==0) {
            masterSpeed=1;
        }
    }
}
void testApp::buttonPressed(ofxGamepadButtonEvent& e)
{
    
    if (e.button==0) {
        stopRecording(0);
    }
    if (e.button==1) {
        stopRecording(1);
    }
    if (e.button==2) {
        stopRecording(2);
    }
    if (e.button==3) {
        stopRecording(3);
    }
    if (e.button==4) {
        stopRecording(4);
    }
    if (e.button==5) {
        stopRecording(5);
    }
    if (e.button==6) {
        stopRecording(6);
    }
    if (e.button==7) {
        stopRecording(7);
    }
	cout << "BUTTON " << ofToString(e.button) << " RELEASED" << endl;
}
void testApp::buttonReleased(ofxGamepadButtonEvent& e){
    if (e.button==0) {
        if(masterLoopRecorded==true){
            for (int i=0; i<8; i++) {
                recordedVideoPlayback[i].stop();
                //recordedVideoPlayback[i].closeMovie();
            }
            
            file.removeFile("Slice1.mov");
        }
        
        vidRecorder->startRecording("Slice1.mov");
    }
    if (e.button==1) {
        startRecording(1);
    }
    if (e.button==2) {
        startRecording(2);
    }
    if (e.button==3) {
        startRecording(3);
    }
    if (e.button==4) {
        startRecording(4);
    }
    if (e.button==5) {
        startRecording(5);
    }
    if (e.button==6) {
        startRecording(6);
    }
    if (e.button==7) {
        startRecording(7);
    }
	cout << "BUTTON " << ofToString(e.button) << " PRESSED" << endl;
}
void testApp::threadedVideoEvent(ofxThreadedVideoEvent & event){
    ofLogVerbose() << "VideoEvent:" << event.eventTypeAsString << "for" << event.path;
}
//--------------------------------------------------------------
void testApp::videoSaved(ofVideoSavedEventArgs& e){
	// the ofQTKitGrabber sends a message with the file name and any errors when the video is done recording
	if(e.error.empty()){
        
	    
	}
	else {
		ofLogError("videoSavedEvent") << "Video save error: " << e.error;
	}
}


//--------------------------------------------------------------
void testApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void testApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::windowResized(int w, int h){

}

