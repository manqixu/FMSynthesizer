// Music 256a / CS 476a | fall 2016
// CCRMA, Stanford University
//
// Author: Maggie Xu (manqixu@stanford.edu)
// Description: JUCE FM Synthesizer

#ifndef MAINCOMPONENT_H_INCLUDED
#define MAINCOMPONENT_H_INCLUDED

#include "../JuceLibraryCode/JuceHeader.h"
#include "Sine.h"
#include "Smooth.h"
#include "FaustReverb.h"


// Seven Pitches starting with C5
const int NumPitch = 8;
const float pitch[8] = {523.25, 587.33, 659.25, 698.46, 783.99, 880.00, 987.77, 1046.50};

class MainContentComponent :
    public AudioAppComponent,
    private Slider::Listener,
    private Button::Listener
{
public:
    MainContentComponent()
    :carrierFrequency(440.0),
    index(0.0),
    onOff (0.0)
    {
        for (int i = 0; i < NumPitch; i++) {
            pitch_keys[i] = KeyPress(i+49);
        } 
        
        gain_keys[0] = KeyPress('-');
        gain_keys[1] = KeyPress('=');
        
        // configuring frequency sliders and adding them to the main window
        addAndMakeVisible (frequencySlider);
        frequencySlider.setRange (50.0, 5000.0);
        frequencySlider.setSkewFactorFromMidPoint (500.0);
        frequencySlider.setValue(1000);
        frequencySlider.addListener (this);
        
        // configuring carrier label box and adding it to the main window
        addAndMakeVisible(carrierLabel);
        carrierLabel.setText ("Frequency", dontSendNotification);
        carrierLabel.attachToComponent (&frequencySlider, true);
        
        // configuring gain slider and adding it to the main window
        addAndMakeVisible (gainSlider);
        gainSlider.setRange (0.0, 1.0);
        gainSlider.setValue(0.5);
        gainSlider.addListener (this);
        
        // configuring gain label and adding it to the main window
        addAndMakeVisible(gainLabel);
        gainLabel.setText ("Gain", dontSendNotification);
        gainLabel.attachToComponent (&gainSlider, true);
        
        // configuring on/off button and adding it to the main window
        addAndMakeVisible(onOffButton);
        onOffButton.addListener(this);
        
        // configuring on/off label and adding it to the main window
        addAndMakeVisible(onOffLabel);
        onOffLabel.setText ("On/Off", dontSendNotification);
        onOffLabel.attachToComponent (&onOffButton, true);
        
        setSize (600, 190);
		nChans = 2;
        setAudioChannels (0, nChans); // no inputs, n output

		audioBuffer = new float*[nChans];
    }
    
    ~MainContentComponent()
    {
        shutdownAudio();
        delete [] audioBuffer;
    }
    
    void resized() override
    {
        // placing the UI elements in the main window
        // getWidth has to be used in case the window is resized by the user
        const int sliderLeft = 80;
        frequencySlider.setBounds (sliderLeft, 90, getWidth() - sliderLeft - 20, 20);
        gainSlider.setBounds (sliderLeft, 120, getWidth() - sliderLeft - 20, 20);
        onOffButton.setBounds (sliderLeft, 150, getWidth() - sliderLeft - 20, 20);
    }
    
    void paint (Graphics& g) override {
        g.fillAll (Colours::lightblue);
        g.setColour (Colours::darkblue);
        g.setFont (Font("Times New Roman", 20.0f, Font::italic));
        g.drawText ("FM Synthesizer", 250, 10, 200, 40, true);
        g.setFont (14.0f);
        g.drawText ("Press 1-8 for pitches and < or > for volume", 200, 40, 400, 40, true);
    }
    
    void sliderValueChanged (Slider* slider) override
    {
        if (carrier.getSamplingRate() > 0.0){
            if (slider == &frequencySlider)
            {
                carrierFrequency = frequencySlider.getValue()/2;
                modulator.setFrequency(frequencySlider.getValue());
                index = 200;
            }
        }
    }
    
    void buttonClicked (Button* button) override
    {
        if(button == &onOffButton && onOffButton.getToggleState()){
            onOff = 1.0;
        }
        else{
            onOff = 0.0;
        }
    }
    
    // configuring keypress listener
    bool keyPressed (const KeyPress &key) override
    {
        if(key == gain_keys[0]) {
            gainSlider.setValue(gainSlider.getValue()-0.1);
        } else if(key == gain_keys[1]) {
            gainSlider.setValue(gainSlider.getValue()+0.1);
        } else {
           for (int i = 0; i < NumPitch; i++) {
                if(key == pitch_keys[i]){
                    frequencySlider.setValue(pitch[i]);
                }
            } 
        }
        
        return true;   
    }
    
    void prepareToPlay (int /*samplesPerBlockExpected*/, double sampleRate) override
    {
        carrier.setSamplingRate(sampleRate);
        carrier.setFrequency(carrierFrequency);
        modulator.setSamplingRate(sampleRate);
        modulator.setFrequency(50.0);
        for(int i=0; i<4; i++){
            smooth[i].setSmooth(0.999);
        }
        reverb.init(sampleRate);
		reverb.buildUserInterface(&reverbControl);
    }
    
    void releaseResources() override
    {
    }
    
    void getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill) override
    {
        const float level = gainSlider.getValue(); // gain is updated every block
        // getting the audio output buffer to be filled
		for(int i=0; i<nChans; i++){
			audioBuffer[i] = bufferToFill.buffer->getWritePointer (i, bufferToFill.startSample);
		}        
        // computing one block
        for (int sample = 0; sample < bufferToFill.numSamples; ++sample)
        {
            carrier.setFrequency(smooth[0].tick(carrierFrequency) + modulator.tick()*smooth[3].tick(index)); // modulating freq of the carrier
            audioBuffer[0][sample] = carrier.tick() * smooth[1].tick(level) * smooth[2].tick(onOff); // computing sample
        }
		reverb.compute(bufferToFill.numSamples,audioBuffer,audioBuffer);
    }
    
    
private:
    Slider frequencySlider;
    Slider gainSlider;
    ToggleButton onOffButton;
    
    Label carrierLabel;
    Label gainLabel;
    Label onOffLabel;
    
    // KeyPress Elements
    KeyPress* pitch_keys = new KeyPress[NumPitch];
    KeyPress* gain_keys = new KeyPress[2];
    
    Sine carrier, modulator;
    Smooth smooth[4]; // to prevent cliking of some of the parameters
    
    FaustReverb reverb;
	MapUI reverbControl;
	
    float** audioBuffer;
    int nChans;
    
    double carrierFrequency, index, onOff;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainContentComponent)
};

Component* createMainContentComponent()     { return new MainContentComponent(); }


#endif  // MAINCOMPONENT_H_INCLUDED
