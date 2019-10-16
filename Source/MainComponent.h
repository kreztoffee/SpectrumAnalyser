#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "SpectrumView.h"

enum eFftSize
{
	k128,
	k256,
	k512,
	k1024,
	k2048,
	k4096,
	k8192
};

extern StringArray strFftSize;
extern Array<int> nFftSize;

class MainComponent
	: public AudioAppComponent
	, public ComboBox::Listener
	, public Button::Listener
	, public ChangeListener
	, public Timer
{
public:
	MainComponent();
	~MainComponent();

	void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
	void getNextAudioBlock(const AudioSourceChannelInfo& bufferToFill) override;
	void releaseResources() override;

private:
	enum eTransportState
	{
		kStopping,
		kStopped,
		kStarting,
		kPlaying,
		kPausing,
		kPaused
	};

	eTransportState m_transportState;
	AudioTransportSource m_transportSource;

	void paint(Graphics& g) override;
	void resized() override;
	void buttonClicked(juce::Button* button) override;
	void comboBoxChanged(ComboBox* comboBox) override;
	void changeState(eTransportState newState);
	void changeListenerCallback(ChangeBroadcaster* source) override;
	void timerCallback() override;

	TextButton m_btnOpen;

	File m_loadedFile;

	TextButton m_btnPlay;
	TextButton m_btnStop;
	Label m_lblStatus;
	Label m_lblPlayBackTime;

	ComboBox m_selFftSize;

	SpectrumView m_spectrumView;

	AudioFormatManager m_formatManager;
	std::unique_ptr<AudioFormatReaderSource> m_readerSource;
	AudioThumbnailCache m_thumbnailCache;
	AudioThumbnail m_thumbnail;

	int m_maxBlock;
	double m_sampleRate;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
