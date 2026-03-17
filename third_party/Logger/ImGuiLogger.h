
#ifndef IMGUILOGGER_H
#define	IMGUILOGGER_H

#include <type_traits>
#include <iostream>
#include <chrono>
#include <ctime>
#include <string>
#include <iostream>
#include "imgui/imgui.h"
#include <vector>
#include "Singleton.h"
using std::chrono::system_clock;
enum class Comm_Type { WARNING, INFO, ERROR,TRACE };

class _msg
{
public:
	_msg(Comm_Type T, std::string s)
	{
		m_Type = T;
		m_comm = s;
	}
	void print_to_log(ImColor col)
	{
		ImGui::TextColored(col, "%s",m_comm.c_str());
	};
	Comm_Type m_Type;
private:
	std::string m_comm;
	
};

class Logger :public Singleton<Logger> {
	friend class Singleton<Logger>; 
private:
	ImGuiTextBuffer     Buf;
	std::vector<_msg> loggedMsg;
	bool ScrollToBottom;
	bool disp_error;
	bool disp_warning;
	bool disp_info;
	bool disp_trace;
	Comm_Type current_type;
private: 
	Logger();
public:
	void Clear();
	bool show_interface;
	Logger &operator <<(const char* value);
		template<typename T>
		Logger &operator <<(const T value)
		{
			Buf.append("%s",std::to_string(value).c_str());
			if (current_type != Comm_Type::TRACE)
				std::cout << value;
			return *this;
		};
		Logger &operator <<(const std::string value);
		Logger &operator <<(std::ostream& (*os)(std::ostream&));
		void Draw(const char* title);
};
#ifndef LOG_DEFINITION
#define LOG_DEFINITION
#define LOG_TRACE *(Logger::getInstance()) << "[TRACE]"
#define LOG_INFO *(Logger::getInstance()) << "[INFO]"
#define LOG_WARNING *(Logger::getInstance()) << "[WARNING]"
#define LOG_ERROR *(Logger::getInstance()) << "[ERROR]"
#endif

#endif