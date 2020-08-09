#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include <fftw3.h>
#include <complex>

class SpectrumView
	: public Component
	, public Timer
{
public:
	SpectrumView();
	~SpectrumView();

	void setSampleRate(float sampleRate);
	void setFftSize(int size);
	void cleanup();
	void pushSample(float sample);

private:
	void drawFrame();

	void paint(Graphics& g) override;
	void resized() override;
	void timerCallback() override;

	float m_sampleRate;
	int m_fftSize;
	double* m_fifo;
	double* m_fftIn;
	fftw_complex* m_fftOut;
	fftw_plan m_fftForward;

	int m_drawPoints;
	float* m_drawingData;

	int m_fifoIndex;
	Atomic<bool> m_nextBlockReady;
	CriticalSection m_blockLock;
};
