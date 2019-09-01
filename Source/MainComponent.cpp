#include "MainComponent.h"

MainComponent::MainComponent()
	: m_transportState(kStopped)
	, m_thumbnailCache(32)
	, m_thumbnail(256, m_formatManager, m_thumbnailCache)
	, m_maxBlock(0)
	, m_sampleRate(0.0)
{
	setSize(800, 600);

	if (RuntimePermissions::isRequired(RuntimePermissions::recordAudio)
		&& !RuntimePermissions::isGranted(RuntimePermissions::recordAudio))
	{
		RuntimePermissions::request(RuntimePermissions::recordAudio,
									[&](bool granted) { if (granted)  setAudioChannels(2, 2); });
	}
	else
	{
		setAudioChannels(2, 2);
	}

	m_btnOpen.setButtonText("Open Audio File");
	m_btnOpen.addListener(this);
	addAndMakeVisible(m_btnOpen);

	m_btnPlay.setButtonText("Play");
	m_btnPlay.setEnabled(false);
	m_btnPlay.addListener(this);
	addAndMakeVisible(m_btnPlay);

	m_btnStop.setButtonText("Stop");
	m_btnStop.setEnabled(false);
	m_btnStop.addListener(this);
	addAndMakeVisible(m_btnStop);

	m_lblStatus.setText("status", NotificationType::dontSendNotification);
	addAndMakeVisible(m_lblStatus);

	m_lblPlayBackTime.setText("-/-", NotificationType::dontSendNotification);
	addAndMakeVisible(m_lblPlayBackTime);

	m_thumbnail.addChangeListener(this);

	m_formatManager.registerBasicFormats();
	m_transportSource.addChangeListener(this);

	m_spectrumView.setHopSize(4096);
	addAndMakeVisible(m_spectrumView);

	startTimer(100);
}

MainComponent::~MainComponent()
{
	stopTimer();
	shutdownAudio();
}

void MainComponent::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
	m_transportSource.prepareToPlay(samplesPerBlockExpected, sampleRate);

	if (samplesPerBlockExpected != m_maxBlock)
	{
		m_maxBlock = samplesPerBlockExpected;
	}

	if (sampleRate != m_sampleRate)
	{
		m_sampleRate = sampleRate;
	}
}

void MainComponent::getNextAudioBlock(const AudioSourceChannelInfo& bufferToFill)
{
	if (m_readerSource.get() == nullptr)
	{
		bufferToFill.clearActiveBufferRegion();
		return;
	}

	m_transportSource.getNextAudioBlock(bufferToFill);

	AudioBuffer<float>* thisBuffer = bufferToFill.buffer;

	for (int i = 0; i < thisBuffer->getNumSamples(); ++i)
	{
		m_spectrumView.pushSample(thisBuffer->getSample(0, i));
	}
}

void MainComponent::releaseResources()
{
}

void MainComponent::paint(Graphics& g)
{
	g.fillAll(getLookAndFeel().findColour(ResizableWindow::backgroundColourId));

	m_thumbnail.getNumChannels();

	g.setColour(Colours::white);

	Rectangle<int> thumbnail(150, 10, getWidth() - 150 - 10, 100);

	g.setColour(Colours::lightblue);
	g.drawRect(thumbnail);
	g.setColour(Colours::lightgrey);
	m_thumbnail.drawChannel(g, thumbnail, 0.0, 15.0, 0, 0.5);
}

void MainComponent::resized()
{
	m_btnOpen.setBounds(10, 10, 100, 20);
	m_btnPlay.setBounds(10, 50, 100, 20);
	m_btnStop.setBounds(10, 85, 100, 20);
	m_lblStatus.setBounds(150, 120, 100, 20);
	m_lblPlayBackTime.setBounds(270, 120, 100, 20);

	m_spectrumView.setBounds(10, 150, getWidth() - 70, getHeight() - 150 - 10);
}

void MainComponent::buttonClicked(juce::Button* button)
{
	if (button == &m_btnOpen)
	{
		FileChooser fc("Chose Audio File to Load...",
					   File::getCurrentWorkingDirectory(),
					   "*.wav");

		if (fc.browseForFileToOpen())
		{
			if (m_transportState != kStopped)
				changeState(kStopping);

			m_loadedFile = fc.getResult();

			auto* reader = m_formatManager.createReaderFor(m_loadedFile);

			if (reader != nullptr)
			{
				std::unique_ptr<AudioFormatReaderSource> newSource(new AudioFormatReaderSource(reader, true));
				m_transportSource.setSource(newSource.get(), 0, nullptr, reader->sampleRate);
				m_readerSource.reset(newSource.release());
			}

			m_thumbnail.setSource(new FileInputSource(m_loadedFile));

			m_btnPlay.setEnabled(true);
			m_btnStop.setEnabled(false);
			m_btnPlay.setButtonText("Play");
			repaint();
		}
	}
	else if (button == &m_btnPlay)
	{
		if ((m_transportState == kStopped) || (m_transportState == kPaused))
			changeState(kStarting);
		else if (m_transportState == kPlaying)
			changeState(kPausing);

		m_btnStop.setEnabled(true);
	}
	else if (button == &m_btnStop)
	{
		if (m_transportState == kPaused)
			changeState(kStopped);
		else if (m_transportState != kStopped)
			changeState(kStopping);

		m_btnStop.setEnabled(false);
	}
}

void MainComponent::changeState(eTransportState newState)
{
	if (m_transportState != newState)
	{
		m_transportState = newState;

		switch (m_transportState)
		{
			case kStopping:
				m_transportSource.stop();
				m_lblStatus.setText("stopping", NotificationType::dontSendNotification);
				break;
			case kStopped:
				m_transportSource.setPosition(0.0);
				m_lblStatus.setText("stopped", NotificationType::dontSendNotification);
				m_btnPlay.setButtonText("Play");
				break;
			case kStarting:
				m_transportSource.start();
				m_lblStatus.setText("starting", NotificationType::dontSendNotification);
				break;
			case kPlaying:
				m_lblStatus.setText("playing", NotificationType::dontSendNotification);
				m_btnPlay.setButtonText("Pause");
				break;
			case kPausing:
				m_transportSource.stop();
				m_lblStatus.setText("pausing", NotificationType::dontSendNotification);
				break;
			case kPaused:
				m_lblStatus.setText("paused", NotificationType::dontSendNotification);
				m_btnPlay.setButtonText("Play");
				break;
			default:
				m_lblStatus.setText("unknown", NotificationType::dontSendNotification);
				break;
		}
	}
}

void MainComponent::changeListenerCallback(ChangeBroadcaster* source)
{
	if (source == &m_transportSource)
	{
		if (m_transportSource.isPlaying())
			changeState(kPlaying);
		else if ((m_transportState == kStopping) || (m_transportState == kPlaying))
			changeState(kStopped);
		else if (m_transportState == kPausing)
			changeState(kPaused);
	}
	if (source == &m_thumbnail)
	{
		repaint();
	}
}

void MainComponent::timerCallback()
{
	double currentTime = m_transportSource.getCurrentPosition();
	double length = m_transportSource.getLengthInSeconds();
}
