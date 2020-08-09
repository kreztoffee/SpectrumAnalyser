#include "SpectrumView.h"

SpectrumView::SpectrumView()
	: m_sampleRate(0.0f)
	, m_fftSize(0)
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
	cleanup();
}

void SpectrumView::setSampleRate(float sampleRate)
{
	m_sampleRate = sampleRate;
}

void SpectrumView::setFftSize(int size)
{
	if (size != m_fftSize)
		cleanup();

	m_fftSize = size;
	m_drawPoints = size / 2;

	m_fftIn = (double*)fftw_malloc(sizeof(double) * size);
	m_fftOut = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * size);
	m_fftForward = fftw_plan_dft_r2c_1d(size, m_fftIn, m_fftOut, FFTW_ESTIMATE);

	m_fifo = new double[size];
	m_drawingData = new float[size];
	memset(m_fifo, 0, size * sizeof(double));
	memset(m_drawingData, 0, size * sizeof(float));
}

void SpectrumView::cleanup()
{
	if (m_fftIn)
		fftw_free(m_fftIn);

	if (m_fftOut)
		fftw_free(m_fftOut);

	if (m_fftForward)
		fftw_destroy_plan(m_fftForward);

	if (m_fifo)
		delete[] m_fifo;

	if (m_drawingData)
		delete[] m_drawingData;

	m_fftSize = 0;
	m_drawPoints = 0;
	m_fifoIndex = 0;
	m_nextBlockReady = false;
}

void SpectrumView::pushSample(float sample)
{
	// if a new fft sized block is ready
	if (m_fifoIndex == m_fftSize)
	{
		// and it is time for a new block
		if (!m_nextBlockReady.get())
		{
			// Hann window
			for (int i = 0; i < m_fftSize; ++i)
				m_fifo[i] *= 0.5f - 0.5f * cos((2.0f * 3.141592654f * i) / m_fftSize);

			memset(m_fftIn, 0, m_fftSize);
			memcpy(m_fftIn, m_fifo, m_fftSize * sizeof(float));
			m_nextBlockReady = true;
		}

		m_fifoIndex = 0;
	}

	m_fifo[m_fifoIndex++] = sample;
}

void SpectrumView::drawFrame()
{
	const float fftWeight = 1.0f / m_fftSize;

	fftw_execute(m_fftForward);

	for (int i = 0; i < m_fftSize; ++i)
	{
		m_fftOut[i][0] *= fftWeight;
		m_fftOut[i][1] *= fftWeight;
	}

	float maxval = -100.0f;
	const float minDb = -100.0f;
	const float maxDb = 0.0f;
	
	for (int i = 0; i < m_drawPoints; ++i)
	{
		float incX = logf(1.0f + (i / (float)m_drawPoints));
		int fftDataIndex = jlimit(0, m_fftSize / 2, (int)(incX * (m_fftSize / 2.0f)));
		
		float mag = sqrtf((m_fftOut[fftDataIndex][0] * m_fftOut[fftDataIndex][0]) + (m_fftOut[fftDataIndex][1] * m_fftOut[fftDataIndex][1]));
		
		float dbVal = Decibels::gainToDecibels(mag);
		if (dbVal > maxval)
			maxval = dbVal;

		float level = jmap(jlimit(minDb, maxDb, dbVal), minDb, maxDb, 0.0f, 1.0f);
		m_drawingData[i] = level;
	}

	Logger::outputDebugString(String(maxval));
}

void SpectrumView::paint(Graphics& g)
{
	g.setColour(Colours::grey);
	g.drawRect(getLocalBounds());

	float width = static_cast<float>(getWidth());
	float height = static_cast<float>(getHeight());

	const float majorLines[] = { 1.0f, 10.0f, 100.0f, 1000.0f, 10000.0f };
	const float minorLines[] = { 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f };

	const float binSize = (m_sampleRate / 2.0f) / m_fftSize;
	const float logSampleRate = std::log10f(m_sampleRate / 2.0f);

	for (int i = 0; i < 5; ++i)
	{
		float pos = (std::log10f(majorLines[i]) / logSampleRate) * width;
		
		g.drawLine({ pos, 0.0f, pos, height });

		for (int j = 0; j < 8; ++j)
		{
			pos = (std::log10f(minorLines[j] * majorLines[i]) / logSampleRate) * width;
			g.drawLine({ pos, height - 6.0f, pos, height });
		}
	}

	g.setColour(Colours::limegreen);

	for (int i = 1; i < (int)width; ++i)
	{
		int idx0 = static_cast<int>(std::powf(10, ((i - 1) / width) * logSampleRate) / binSize);
		int idx1 = static_cast<int>(std::powf(10, (i / width) * logSampleRate) / binSize);

		float x0 = (float)i - 1.0f;
		float y0 = (float)jmap(m_drawingData[idx0], (float)getHeight(), 0.0f);
		float x1 = (float)i;
		float y1 = (float)jmap(m_drawingData[idx1], (float)getHeight(), 0.0f);
		
		g.drawLine({ x0, y0, x1, y1 });
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
		
		g.setColour(Colours::white);
		g.drawText("-" + String(i * incDb) + "dB", Rectangle<float>(1.0f, yPos, 60.0f, 20.0f), Justification::centredLeft);
		g.setColour(Colours::grey);
		g.drawLine({ 0.0f, yPos, (float)getWidth(), yPos }, 1.0f);
	}
}

void SpectrumView::resized()
{

}

void SpectrumView::timerCallback()
{
	if (m_nextBlockReady.get())
	{
		drawFrame();
		repaint();
		m_nextBlockReady = false;
	}
}
