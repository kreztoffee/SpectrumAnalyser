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

	void setHopSize(int size);
	void pushSample(float sample);

private:
	void drawFrame();

	void paint(Graphics& g) override;
	void resized() override;
	void timerCallback() override;

	int m_fftSize;
	float* m_fifo;
	float* m_fftIn;
	fftwf_complex* m_fftOut;
	fftwf_plan m_fftForward;

	int m_drawPoints;
	float* m_drawingData;

	int m_fifoIndex;
	bool m_nextBlockReady;
};
