#include "speechrec.h"
#include "text/utf8.h"
#include "general/exception.h"
#include <cstring>
#include <cstdint>
#include <cmath>

#ifdef USE_VOSK
#include <vosk_api.h>
#include <string>
#include <sstream>
#include "general/logger.h"
#else
#include <pocketsphinx.h>
#endif

using namespace std;


SpeechRecognition::SpeechRecognition() :
#ifdef USE_VOSK
	m_model(nullptr),
	m_recognizer(nullptr),
#else
	m_ps(NULL),
	m_config(NULL),
#endif
	m_utteranceStarted(false),
	m_framePeriod(0.0),
	m_deltaTime(-1.0),
	m_timeBase(0.0),
	m_sampleRate(16000),
	m_minProb(1.0f),
	m_minLen(0)
{
#ifndef USE_VOSK
	m_config = cmd_ln_parse_r(NULL, ps_args(), 0, NULL, TRUE);
	if (m_config == NULL)
		throw EXCEPTION("can't init Sphinx configuration")
			.module("SpeechRecognition", "cmd_ln_parse_r");
#endif
}

SpeechRecognition::~SpeechRecognition()
{
#ifdef USE_VOSK
	if (m_recognizer)
		vosk_recognizer_free(m_recognizer);
	if (m_model)
		vosk_model_free(m_model);
#else
	cmd_ln_free_r(m_config);
#endif
}

void SpeechRecognition::setParam(const string &key, const string &val)
{
#ifdef USE_VOSK
	if (key == "-hmm" || key == "model-path")
		m_modelPath = val;
	else if (key == "-frate")
		m_sampleRate = atoi(val.c_str());
	else
		m_voskParams[key] = val;
#else
	arg_t const *args = ps_args();

	for (size_t i = 0; args[i].name != NULL; i++)
	{
		if (key == args[i].name)
		{
			int type = args[i].type;
			if (type & ARG_INTEGER)
				cmd_ln_set_int_r(m_config, key.c_str(), atol(val.c_str()));
			else if (type & ARG_FLOATING)
				cmd_ln_set_float_r(m_config, key.c_str(), atof(val.c_str()));
			else if (type & ARG_STRING)
				cmd_ln_set_str_r(m_config, key.c_str(), val.c_str());
			else if (type & ARG_BOOLEAN)
				cmd_ln_set_boolean_r(m_config, key.c_str(),
						!(val.empty() || val == "0"));
			else
				throw EXCEPTION("invalid parameter type")
					.module("SpeechRecognition", "setParameter")
					.add("parameter", key)
					.add("value", val)
					.add("type", type);

			return;
		}
	}

	throw EXCEPTION("parameter not supported")
		.module("SpeechRecognition", "setParameter")
		.add("parameter", key)
		.add("value", val);
#endif
}

void SpeechRecognition::addWordsListener(WordsListener listener)
{
	m_wordsNotifier.addListener(listener);
}

bool SpeechRecognition::removeWordsListener(WordsListener listener)
{
	return m_wordsNotifier.removeListener(listener);
}

void SpeechRecognition::setMinWordProb(float minProb)
{
	m_minProb = minProb;
}

void SpeechRecognition::setMinWordLen(unsigned minLen)
{
	m_minLen = minLen;
}

void SpeechRecognition::start(const AVStream *stream)
{
#ifdef USE_VOSK
	if (m_modelPath.empty())
		throw EXCEPTION("Vosk model path not set")
			.module("SpeechRecognition");

	m_model = vosk_model_new(m_modelPath.c_str());
	if (!m_model)
		throw EXCEPTION("can't load Vosk model")
			.module("SpeechRecognition", "vosk_model_new")
			.add("path", m_modelPath);

	float sampleRate = static_cast<float>(m_sampleRate);
	m_recognizer = vosk_recognizer_new(m_model, sampleRate);
	if (!m_recognizer)
		throw EXCEPTION("can't create Vosk recognizer")
			.module("SpeechRecognition", "vosk_recognizer_new");

	vosk_recognizer_set_words(m_recognizer, 1);
	m_framePeriod = 1.0 / static_cast<double>(m_sampleRate);
	logger::info("SpeechRec", "Vosk initialized: model='%s' sampleRate=%d", m_modelPath.c_str(), m_sampleRate);
#else
	if ((m_ps = ps_init(m_config)) == NULL)
		throw EXCEPTION("can't init Sphinx engine")
			.module("SpeechRecognition", "ps_init");

	int32_t frate = cmd_ln_int32_r(m_config, "-frate");
	if (frate == 0)
		throw EXCEPTION("can't get frame rate value")
			.module("SpeechRecognition", "cmd_ln_int32_r");

	m_framePeriod = 1.0 / static_cast<double>(frate);

	if (ps_start_utt(m_ps))
		throw EXCEPTION("can't start speech recognition")
			.module("SpeechRecognition", "ps_start_utt");
#endif

	m_utteranceStarted = false;
	m_timeBase = av_q2d(stream->time_base);
}

void SpeechRecognition::stop()
{
#ifdef USE_VOSK
	if (m_recognizer)
	{
		const char *finalResult = vosk_recognizer_final_result(m_recognizer);
		if (finalResult)
		{
			logger::debug("SpeechRec", "vosk final result: %.200s", finalResult);
			parseVoskResult(finalResult);
		}

		vosk_recognizer_free(m_recognizer);
		m_recognizer = nullptr;
	}
#else
	if (m_ps)
	{
		if (ps_end_utt(m_ps))
			throw EXCEPTION("can't stop speech recognition")
				.module("SpeechRecognition", "ps_end_utt");

		if (m_utteranceStarted)
		{
			parseUtterance();
			m_utteranceStarted = false;
		}

		ps_free(m_ps);
		m_ps = NULL;
	}
#endif
}

void SpeechRecognition::feed(const AVFrame *frame)
{
	if (m_deltaTime < 0.0)
		m_deltaTime = m_timeBase * frame->pts;

	const int16_t *data = reinterpret_cast<const int16_t*>(frame->data[0]);
	int size = frame->nb_samples;

#ifdef USE_VOSK
	const char *rawData = reinterpret_cast<const char*>(data);
	int byteLen = size * sizeof(int16_t);

	int accepted = vosk_recognizer_accept_waveform(m_recognizer, rawData, byteLen);
	if (accepted)
	{
		const char *result = vosk_recognizer_result(m_recognizer);
		if (result)
		{
			logger::debug("SpeechRec", "vosk result: %.200s", result);
			parseVoskResult(result);
		}
		else
		{
			logger::debug("SpeechRec", "vosk returned null result");
		}
	}
#else
	int no = ps_process_raw(m_ps, data, size, FALSE, FALSE);
	if (no < 0)
		throw EXCEPTION("speech recognition error")
			.module("SpeechRecognition", "ps_process_raw");

	uint8_t inSpeech = ps_get_in_speech(m_ps);
	if (inSpeech && !m_utteranceStarted)
	{
		m_utteranceStarted = true;
	}
	if (!inSpeech && m_utteranceStarted)
	{
		if (ps_end_utt(m_ps))
			throw EXCEPTION("can't end utterance")
				.module("SpeechRecognition", "ps_end_utt");

		parseUtterance();

		if (ps_start_utt(m_ps))
			throw EXCEPTION("can't start utterance")
				.module("SpeechRecognition", "ps_start_utt");

		m_utteranceStarted = false;
	}
#endif
}

void SpeechRecognition::flush()
{
#ifdef USE_VOSK
	if (m_recognizer)
	{
		const char *finalResult = vosk_recognizer_final_result(m_recognizer);
		if (finalResult)
			parseVoskResult(finalResult);
	}
#else
	if (m_ps && m_utteranceStarted)
	{
		if (ps_end_utt(m_ps) == 0)
		{
			parseUtterance();
			m_utteranceStarted = false;

			ps_start_utt(m_ps);
		}
	}
#endif
}

void SpeechRecognition::discontinuity()
{
#ifdef USE_VOSK
	if (m_recognizer)
	{
		const char *finalResult = vosk_recognizer_final_result(m_recognizer);
		if (finalResult)
			parseVoskResult(finalResult);

		vosk_recognizer_reset(m_recognizer);
	}
	m_deltaTime = -1.0;
#else
	if (ps_end_utt(m_ps))
		throw EXCEPTION("can't stop speech recognition")
			.module("SpeechRecognition", "ps_end_utt");

	if (m_utteranceStarted)
		parseUtterance();

	m_deltaTime = -1.0;
	if (ps_start_stream(m_ps))
	{
		throw EXCEPTION("can't reset speech recognition engine")
			.module("SpeechRecognition", "sphinx", "ps_start_stream");
	}

	if (ps_start_utt(m_ps))
		throw EXCEPTION("can't start speech recognition")
			.module("SpeechRecognition", "ps_start_utt");

	m_utteranceStarted = false;
#endif
}

#ifdef USE_VOSK
void SpeechRecognition::parseVoskResult(const char *json)
{
	string s(json);

	size_t resultPos = s.find("\"result\"");
	if (resultPos == string::npos)
		return;

	size_t arrStart = s.find('[', resultPos);
	if (arrStart == string::npos)
		return;

	size_t pos = arrStart;
	while (pos < s.size())
	{
		size_t objStart = s.find('{', pos);
		if (objStart == string::npos)
			break;

		size_t objEnd = s.find('}', objStart);
		if (objEnd == string::npos)
			break;

		string obj = s.substr(objStart, objEnd - objStart + 1);

		string word;
		double startTime = 0.0, endTime = 0.0;
		float conf = 0.0f;

		auto extractStr = [&obj](const string &key) -> string {
			size_t kp = obj.find("\"" + key + "\"");
			if (kp == string::npos) return "";
			size_t colon = obj.find(':', kp);
			if (colon == string::npos) return "";
			size_t q1 = obj.find('"', colon + 1);
			if (q1 == string::npos) return "";
			size_t q2 = obj.find('"', q1 + 1);
			if (q2 == string::npos) return "";
			return obj.substr(q1 + 1, q2 - q1 - 1);
		};

		auto extractNum = [&obj](const string &key) -> double {
			size_t kp = obj.find("\"" + key + "\"");
			if (kp == string::npos) return 0.0;
			size_t colon = obj.find(':', kp);
			if (colon == string::npos) return 0.0;
			size_t numStart = colon + 1;
			while (numStart < obj.size() && (obj[numStart] == ' ' || obj[numStart] == '\t'))
				numStart++;
			return atof(obj.c_str() + numStart);
		};

		word = extractStr("word");
		startTime = extractNum("start");
		endTime = extractNum("end");
		conf = static_cast<float>(extractNum("conf"));

		if (!word.empty() && word[0] != '<' && word[0] != '[')
		{
			double midTime = (startTime + endTime) / 2.0;
			float duration = static_cast<float>(endTime - startTime);

			logger::debug("SpeechRec", "word='%s' conf=%.3f start=%.3f end=%.3f minLen=%u minProb=%.3f",
				word.c_str(), conf, startTime, endTime, m_minLen, m_minProb);

			if (Utf8::size(word) >= m_minLen && conf >= m_minProb)
			{
				logger::debug("SpeechRec", "EMITTING word='%s' time=%.3f", word.c_str(), midTime + m_deltaTime);
				m_wordsNotifier.notify(Word(word, midTime + m_deltaTime, duration, conf));
			}
		}

		pos = objEnd + 1;
	}
}
#else
void SpeechRecognition::parseUtterance()
{
	for (ps_seg_t *it=ps_seg_iter(m_ps); it!=NULL; it=ps_seg_next(it))
	{
		const char *text = ps_seg_word(it);
		if (text && text[0] != '<' && text[0] != '[')
		{
			string word = text;
			if ((word.size() > 3) && (word.back() == ')'))
			{
				size_t pos = word.size() - 2;
				while (word[pos] >= '0' && word[pos] <= '9' && pos > 0)
					pos--;

				if (pos <= word.size()-3 && word[pos] == '(')
					word.resize(pos);
			}

			int begin, end;
			ps_seg_frames(it, &begin, &end);
			const double time = ((double)begin+(double)end) * m_framePeriod / 2.0;

			const int pprob = ps_seg_prob(it, NULL, NULL, NULL);
			const float prob = logmath_exp(ps_get_logmath(m_ps), pprob);

			if (Utf8::size(word) >= m_minLen && prob >= m_minProb)
				m_wordsNotifier.notify(Word(word, time + m_deltaTime, 0.0f, prob));
		}
	}
}
#endif

