#include <sserialize/utility/log.h>
#ifdef __ANDROID__
#include <android/log.h>
#else
#include <iostream>
#endif

namespace sserialize {

Log info(sserialize::Log::LL_INFO);
Log warn(sserialize::Log::LL_WARNING);
Log debug(sserialize::Log::LL_DEBUG);
Log err(sserialize::Log::LL_DEBUG);

Log::Log() : m_defLogLevel(LL_INFO) {
#ifdef __ANDROID__
	m_androidLogLevel = ANDROID_LOG_INFO;
#endif
}

Log::Log(Log::LogLevel defLevel) : m_defLogLevel(defLevel) {
#ifdef __ANDROID__
	switch (defLevel) {
	case (sserialize::Log::LL_INFO):
	case (sserialize::Log::LL_WARNING):
		m_androidLogLevel = ANDROID_LOG_INFO;
		break;
	case (sserialize::Log::LL_DEBUG):
		m_androidLogLevel = ANDROID_LOG_DEBUG;
		break;
	case (sserialize::Log::LL_ERROR):
		m_androidLogLevel = ANDROID_LOG_ERROR;
		break;
	default:
		m_androidLogLevel = ANDROID_LOG_VERBOSE;
		break;
	}
#endif
}

Log::~Log() {}

Log& Log::operator()(const std::string& logTag, const std::string& msg) {
	if (m_sbuf.tellp() > 0) {
		(*this)("", m_sbuf.str());
		m_sbuf.clear();
		m_sbuf.str(std::string());
	}
#ifdef __ANDROID__
	__android_log_print(m_androidLogLevel, logTag.c_str(), msg.c_str(), 0);
#else
	if (m_defLogLevel != LL_ERROR) {
		std::cout << logTag << ": " << msg << std::endl;
	}
	else {
		std::cerr << logTag << ": " << msg << std::endl;
	}
#endif
	return *this;
}

void Log::sbufCmd(Log::CmdTypes t) {
#ifdef __ANDROID__
	__android_log_print(m_androidLogLevel, "", m_sbuf.str().c_str(), 0);
#else
	if (m_defLogLevel != LL_ERROR) {
		std::cout << m_sbuf.str() << std::flush;
		if (t == sserialize::Log::CmdTypes::endl) {
			std::cout << std::endl;
		}
	}
	else {
		std::cerr << m_sbuf.str() << std::flush;
		if (t == Log::CmdTypes::endl) {
			std::cerr << std::endl;
		}
	}
#endif
	m_sbuf.clear();
	m_sbuf.str(std::string());
}

sserialize::Log& operator<<(sserialize::Log& log, const char * msg) {
	log.sbuf() << msg;
	return log;
}

sserialize::Log& operator<<(sserialize::Log& log, const std::string & msg) {
	log.sbuf() << msg;
	return log;
}
sserialize::Log& operator<<(sserialize::Log& log, int num) {
	log.sbuf().operator<<(num);
	return log;
}

sserialize::Log& operator<<(sserialize::Log& log, unsigned int num) {
	log.sbuf().operator<<(num);
	return log;
}

sserialize::Log& operator<<(sserialize::Log& log, double num) {
	log.sbuf().operator<<(num);
	return log;
}

sserialize::Log& operator<<(sserialize::Log& log, bool value) {
	log.sbuf().operator<<((value ? "true" : "false"));
	return log;
}

sserialize::Log& operator<<(sserialize::Log& log, sserialize::Log::CmdTypes cmdt) {
	log.sbufCmd(cmdt);
	return log;
}

}//End namespace
