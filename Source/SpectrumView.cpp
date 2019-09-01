#include "SpectrumView.h"

SpectrumView::SpectrumView()
	: m_fftSize(0)
	, m_fifo(nullptr)
	, m_fftIn(nullptr)
	, m_fftOut(nullptr)
	, m_fftForward(nullptr)
	, m_drawPoints(0)
	, m_drawingData(nullptr)
	, m_fifoIndex(0)
	, m_nextBlockReady(false)
{
	startTimer(100);
}

SpectrumView::~SpectrumView()
{
	fftwf_free(m_fftIn);
	fftwf_free(m_fftOut);
	fftwf_destroy_plan(m_fftForward);
	delete m_fifo;
	delete m_drawingData;
}

void SpectrumView::setHopSize(int size)
{
	m_fftSize = size;
	m_drawPoints = size / 2;

	m_fftIn = (float*)fftwf_malloc(sizeof(float) * size);
	m_fftOut = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex) * size);
	m_fftForward = fftwf_plan_dft_r2c_1d(size, m_fftIn, m_fftOut, FFTW_ESTIMATE);
	
	m_fifo = new float[size];
	m_drawingData = new float[size];
}

void SpectrumView::pushSample(float sample)
{
	if (m_fifoIndex == m_fftSize)
	{
		if (!m_nextBlockReady)
		{
			memset(m_fftIn, 0, m_fftSize);
			memcpy(m_fftIn, m_fifo, m_fftSize * sizeof(float));
			m_nextBlockReady = true;
			m_fifoIndex = 0;
		}
	}

	if(!m_nextBlockReady)
		m_fifo[m_fifoIndex++] = sample;
}

void SpectrumView::drawFrame()
{
	fftwf_execute(m_fftForward);

	const float minDb = -100.0f;
	const float maxDb = 0.0f;
	const float fftWeight = 1.0f / m_fftSize;

	for (int i = 0; i < m_drawPoints; ++i)
	{
		float incX = logf(1.0f + (i / (float)m_drawPoints));
		int fftDataIndex = jlimit(0, m_fftSize / 2, (int)(incX * (m_fftSize / 2.0f)));
		float mag = 2 * fftWeight * sqrtf((m_fftOut[0][fftDataIndex] * m_fftOut[0][fftDataIndex]) + (m_fftOut[1][fftDataIndex] * m_fftOut[1][fftDataIndex]));
		float level = jmap(jlimit(minDb, maxDb, Decibels::gainToDecibels(mag)), minDb, maxDb, 0.0f, 1.0f);
		m_drawingData[i] = level;
	}
}

void SpectrumView::paint(Graphics& g)
{
	g.setColour(Colours::white);
	g.drawRect(getLocalBounds());

	int width = getWidth();
	int height = getHeight();

	for (int i = 1; i < m_drawPoints; ++i)
	{
		//g.drawRect(Rectangle<float>((float)jmap(i - 1, 0, m_drawPoints - 1, 0, width), jmap(m_drawingData[i - 1], 0.0f, 1.0f, (float)height, 0.0f), 1, 1));
		
		
		
		g.drawLine({ (float)jmap(i - 1, 0, m_drawPoints - 1, 0, width), jmap(m_drawingData[i - 1], 0.0f, 1.0f, (float)height, 0.0f),
					(float)jmap(i, 0, m_drawPoints - 1, 0, width), jmap(m_drawingData[i], 0.0f, 1.0f, (float)height, 0.0f) });
	}

	const float minDb = -100.0f;
	const float maxDb = 0.0f;
	const float rangeDb = maxDb - minDb;
	const int gridLines = 10;
	const float incDb = rangeDb / gridLines;

	for (int i = 0; i < gridLines; ++i)
	{
		float inc = i / (float)gridLines;
		float yPos = (float)getHeight() - jmap(inc, (float)getHeight(), 0.0f);

		g.drawText("-" + String(i * incDb) + "dB", Rectangle<float>(1.0f, yPos, 60.0f, 20.0f), Justification::centredLeft);
		g.drawLine({ 0.0f, yPos, (float)getWidth(), yPos }, 1.0f);
	}

	for (int i = 0; i < 20; ++i)
	{
		//float incX = logf(1.0f + (i / (float)m_drawPoints));
		//float xPos = jmap(incX, 0.0f, (float)getWidth());
		//
		//g.drawLine({ xPos, 0.0f, xPos, (float)getHeight() });
	}


}

void SpectrumView::resized()
{

}

void SpectrumView::timerCallback()
{
	if (m_nextBlockReady)
	{
		drawFrame();
		repaint();
		m_nextBlockReady = false;
	}
}
