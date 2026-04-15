#include "resampler.h"
#include "avout.h"
#include "general/exception.h"

extern "C"
{
#include <libavutil/channel_layout.h>
}

using namespace std;


Resampler::Resampler() :
	m_output(NULL),
	m_swr(NULL),
	m_outFrame(NULL),
	m_bufferSize(0),
	m_timeBase(0.0)
{
	m_swr = swr_alloc();
}

Resampler::~Resampler()
{
	if (m_swr)
		swr_free(&m_swr);

	if (m_outFrame)
		av_frame_free(&m_outFrame);

	m_bufferSize = 0;
}

void Resampler::connectOutput(shared_ptr<AVOutput> output,
		const AudioFormat &format, int bufferSize)
{
	if (m_outFrame)
		av_frame_free(&m_outFrame);

	m_outFrame = av_frame_alloc();
	m_outFrame->format = format.sampleFormat;
	m_outFrame->sample_rate = format.sampleRate;
	m_outFrame->nb_samples = bufferSize;

#if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(57, 28, 100)
	if (format.channelLayout != 0)
	{
		m_outFrame->ch_layout.order = AV_CHANNEL_ORDER_NATIVE;
		m_outFrame->ch_layout.nb_channels = format.channelsNo;
		m_outFrame->ch_layout.u.mask = format.channelLayout;
	}
	else
	{
		av_channel_layout_default(&m_outFrame->ch_layout, format.channelsNo);
	}
#else
	m_outFrame->channels = format.channelsNo;
	m_outFrame->channel_layout = format.channelLayout;
#endif

	m_bufferSize = bufferSize;
	int res = av_frame_get_buffer(m_outFrame, 0);
	if (res < 0)
		throw EXCEPTION_FFMPEG("can't allocate output audio frame", res)
			.module("Resampler", "av_frame_get_buffer")
			.add("format", format.toString());

	m_output = output;
}

void Resampler::connectFormatChangeCallback(Resampler::FormatChangeCallback cb)
{
	m_formatChangeCb = cb;
}

void Resampler::setChannelMap(const ChannelsMap &channelsMap)
{
	m_channelsMap = channelsMap;
}

void Resampler::start(const AVStream *stream)
{
	m_timeBase = av_q2d(stream->time_base);

	if (m_output)
		m_output->start(stream);
}

void Resampler::stop()
{
	if (m_output)
		m_output->stop();

	swr_close(m_swr);
}

static inline AVChannel bitmaskToAVChannel(uint64_t mask)
{
	int pos = 0;
	while (mask > 1) { mask >>= 1; pos++; }
	return static_cast<AVChannel>(pos);
}

static vector<double> makeMixMap(const AudioFormat &out, const AudioFormat &in,
		Resampler::ChannelsMap channelsMap)
{
	vector<double> mixMap;
	mixMap.resize(in.channelsNo * out.channelsNo);

	for (const auto &ch : channelsMap)
	{
#if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(57, 28, 100)
		AVChannelLayout inLayout = {}, outLayout = {};
		inLayout.order = AV_CHANNEL_ORDER_NATIVE;
		inLayout.nb_channels = in.channelsNo;
		inLayout.u.mask = in.channelLayout;

		outLayout.order = AV_CHANNEL_ORDER_NATIVE;
		outLayout.nb_channels = out.channelsNo;
		outLayout.u.mask = out.channelLayout;

		int i = av_channel_layout_index_from_channel(
				&inLayout, bitmaskToAVChannel(get<0>(ch.first)));
		int o = av_channel_layout_index_from_channel(
				&outLayout, bitmaskToAVChannel(get<1>(ch.first)));
#else
		int i = av_get_channel_layout_channel_index(
				in.channelLayout, get<0>(ch.first));
		int o = av_get_channel_layout_channel_index(
				out.channelLayout, get<1>(ch.first));
#endif
		if (i >= 0 && o >= 0)
			mixMap[i * out.channelsNo + o] = ch.second;
	}

	return mixMap;
}

void Resampler::initSwrContext(const AudioFormat &out, const AudioFormat &in)
{
	initSwrContextFromFrames(m_outFrame, NULL, out, in);
}

void Resampler::initSwrContextFromFrames(const AVFrame *outFrame, const AVFrame *inFrame,
		const AudioFormat &outFmt, const AudioFormat &inFmt)
{
	if (swr_is_initialized(m_swr))
		swr_close(m_swr);

#if LIBSWRESAMPLE_VERSION_INT >= AV_VERSION_INT(4, 7, 100)
	AVChannelLayout outLayout, inLayout;

	if (outFrame && outFrame->ch_layout.order != AV_CHANNEL_ORDER_UNSPEC)
		av_channel_layout_copy(&outLayout, &outFrame->ch_layout);
	else
		av_channel_layout_default(&outLayout, outFmt.channelsNo);

	if (inFrame && inFrame->ch_layout.order != AV_CHANNEL_ORDER_UNSPEC)
		av_channel_layout_copy(&inLayout, &inFrame->ch_layout);
	else
		av_channel_layout_default(&inLayout, inFmt.channelsNo);

	int ret = swr_alloc_set_opts2(&m_swr,
			&outLayout, outFmt.sampleFormat, outFmt.sampleRate,
			&inLayout,  inFmt.sampleFormat,  inFmt.sampleRate,
			0, NULL);

	av_channel_layout_uninit(&outLayout);
	av_channel_layout_uninit(&inLayout);

	if (ret < 0 || m_swr == NULL)
		throw EXCEPTION("can't initialize resampler context")
			.module("Resampler", "swr_alloc_set_opts2")
			.add("input", inFmt.toString())
			.add("output", outFmt.toString());
#else
	m_swr = swr_alloc_set_opts(m_swr,
			outFmt.channelLayout, outFmt.sampleFormat, outFmt.sampleRate,
			inFmt.channelLayout,  inFmt.sampleFormat,  inFmt.sampleRate,
			0, NULL);

	if (m_swr == NULL)
		throw EXCEPTION("can't initialize resampler context")
			.module("Resampler", "swr_alloc_set_opts")
			.add("input", inFmt.toString())
			.add("output", outFmt.toString());
#endif

	if (!m_channelsMap.empty())
	{
		vector<double> mixMap = makeMixMap(outFmt, inFmt, m_channelsMap);
		int res = swr_set_matrix(m_swr, mixMap.data(), outFmt.channelsNo);
		if (res < 0)
			throw EXCEPTION_FFMPEG("can't initialize audio mixer", res)
				.module("Resampler", "swr_set_matrix")
				.add("input", inFmt.toString())
				.add("output", outFmt.toString());
	}

	int res = swr_init(m_swr);
	if (res < 0)
		throw EXCEPTION_FFMPEG("can't initialize audio resampler", res)
			.module("Resampler", "swr_init")
			.add("input", inFmt.toString())
			.add("output", outFmt.toString());
}

void Resampler::feed(const AVFrame *frame)
{
	m_outFrame->nb_samples = m_bufferSize;
	m_outFrame->pts = frame->pts;

	if (!swr_is_initialized(m_swr))
	{
		if (m_formatChangeCb)
			m_formatChangeCb(AudioFormat(frame), AudioFormat(m_outFrame));

		initSwrContextFromFrames(m_outFrame, frame,
				AudioFormat(m_outFrame), AudioFormat(frame));
	}

	int res = swr_convert_frame(m_swr, m_outFrame, frame);
	if (res < 0)
	{
		if (res == AVERROR_INPUT_CHANGED)
		{
			if (m_formatChangeCb)
				m_formatChangeCb(AudioFormat(frame), AudioFormat(m_outFrame));

			swr_close(m_swr);
			initSwrContextFromFrames(m_outFrame, frame,
					AudioFormat(m_outFrame), AudioFormat(frame));
			res = swr_convert_frame(m_swr, m_outFrame, frame);
		}

		if (res < 0)
		{
			throw EXCEPTION_FFMPEG("error during audio conversion", res)
				.module("Resampler", "swr_convert")
				.add("input", AudioFormat(frame).toString())
				.add("output", AudioFormat(m_outFrame).toString())
				.add("pts", frame->pts)
				.time(m_timeBase * frame->pts);
		}
	}

	if (m_output)
		m_output->feed(m_outFrame);
}

void Resampler::flush()
{
	if (swr_is_initialized(m_swr))
	{
		while (true)
		{
			m_outFrame->nb_samples = m_bufferSize;
			int res = swr_convert_frame(m_swr, m_outFrame, NULL);

			if (res < 0)
			{
				throw EXCEPTION_FFMPEG("error during audio conversion", res)
					.module("Resampler", "swr_convert")
					.add("output", AudioFormat(m_outFrame).toString())
					.add("pts", m_outFrame->pts)
					.time(m_timeBase * m_outFrame->pts);
			}

			if (m_outFrame->nb_samples <= 0)
				break;

			if (m_output)
				m_output->feed(m_outFrame);
		}
	}
}

void Resampler::discontinuity()
{
	if (m_output)
		m_output->discontinuity();
}

