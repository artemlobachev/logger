#ifndef LOGGER_HP
#define LOGGER_HPP

#include <iostream>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <string>
#include <unordered_map>
#include <chrono>
#include <iomanip>
#include <sstream>

#include <execinfo.h>
#include <cxxabi.h>
#include <dlfcn.h>
 
// FOR DEBUG
constexpr bool is_debug = 
#ifdef DEBUG
    true;
#else
    false;
#endif

#define AUTO_LOG(level, var) LOG_##level("Auto", #var " = " + HtmlLogger::to_string(var))

#define TIME_BLOCK(name)                                                                                  \
    auto start_##name = std::chrono::high_resolution_clock::now();                                         \
    auto end_##name = std::chrono::high_resolution_clock::now();                                            \
    auto duration_##name = std::chrono::duration_cast<std::chrono::microseconds>(end_##name - start_##name); \
    LOG_DEBUG("Performance", #name " took %ld microseconds", duration_##name.count());

#define LOG_ASSERT(condition, message, ...)                                     \
    do {                                                                         \
        if (!(condition)) {                                                       \
            LOG_ERROR("Assertion Failed", #condition ": " message, ##__VA_ARGS__); \
            assert(condition);                                                      \
        } \
    } while(0)

// macros for automatically receiving information about the call location with VA_ARGS support
#define LOG(head, style, ...) HtmlLogger::log(head, HtmlLogger::format_stream(__VA_ARGS__), style, __FILE__, __FUNCTION__, __LINE__)
#define LOG_ERROR(head, ...) HtmlLogger::log(head, HtmlLogger::format_stream(__VA_ARGS__), CssFmt::ERROR, __FILE__, __FUNCTION__, __LINE__)
#define LOG_WARNING(head, ...) HtmlLogger::log(head, HtmlLogger::format_stream(__VA_ARGS__), CssFmt::WARNING, __FILE__, __FUNCTION__, __LINE__)
#define LOG_INFO(head, ...) HtmlLogger::log(head, HtmlLogger::format_stream(__VA_ARGS__), CssFmt::INFO, __FILE__, __FUNCTION__, __LINE__)
#define LOG_SUCCESS(head, ...) HtmlLogger::log(head, HtmlLogger::format_stream(__VA_ARGS__), CssFmt::SUCCESS, __FILE__, __FUNCTION__, __LINE__)
#define LOG_DEBUG(head, ...) HtmlLogger::log(head, HtmlLogger::format_stream(__VA_ARGS__), CssFmt::DEBUG, __FILE__, __FUNCTION__, __LINE__)
#define LOG_DUMP(head, ...) HtmlLogger::log(head, HtmlLogger::format_stream(__VA_ARGS__), CssFmt::DUMP, __FILE__, __FUNCTION__, __LINE__)
// simplified macros for quick logging with automatic formatting
#define LOGF(level, format, ...) LOG_##level("Auto", format, ##__VA_ARGS__)
#define DUMP_VAR(var) LOG_DUMP("Variable Dump", #var " = %s", HtmlLogger::to_string(var).c_str())
#define TRACE_FUNC() LOG_DEBUG("Function Trace", "Entering function: %s", __FUNCTION__)

namespace CssFmt 
{
    struct HeaderStyle 
	{
        std::string css_class;
        std::string inline_style;
    };
    
    inline const HeaderStyle DEFAULT = {"", ""};
    inline const HeaderStyle ERROR = {"error-header", "color: red; font-weight: bold;"};
    inline const HeaderStyle WARNING = {"warning-header", "color: orange; font-weight: bold;"};
    inline const HeaderStyle INFO = {"info-header", "color: blue; font-style: italic;"};
    inline const HeaderStyle SUCCESS = {"success-header", "color: green; font-weight: bold;"};
    inline const HeaderStyle DEBUG = {"debug-header", "color: gray; font-size: 0.9em;"};
    inline const HeaderStyle DUMP = {"dump-header", "color: #6a0dad; font-family: 'Courier New', monospace; background-color: #f0f0f0; padding: 2px 4px; border-radius: 3px;"};
    
    inline const std::string_view CSS_STYLES = R"(
<style>
    body
    {
        background-color: #fafafa;
        background-image: url(https://sun9-78.userapi.com/s/v1/if2/UVWrdKyXhiRs6kz3MLgrWwErdxU3HBuRRIYry4ng3gBnBcz5uGEapTz62Qky7PA3iXwK_iftmAq6UT3zHrDGhHju.jpg?quality=95&as=32x19,48x29,72x43,108x65,160x96,240x144,360x216,480x288,500x300&from=bu&cs=500x0);
        background-size: 200x200;   
        background-position: bottom; 
        background-repeat: no-repeat;
        background-attachment: fixed;  
    
    }
    table 
    { 
        border-collapse: collapse; 
        width: 100%; 
        font-family: Arial, sans-serif; 
        margin: 10px 0;
    }
    th, td 
    { 
        border: 1px solid #ddd; 
        padding: 8px; 
        text-align: left; 
        vertical-align: top;
    }
    th 
    {
        background-color: #f2f2f2;
        font-weight: bold;
    }
    .timestamp 
    {
        font-family: 'Courier New', monospace;
        font-size: 0.9em;
        color: #555;
        white-space: nowrap;
    }
    .source-info 
    {
        font-family: 'Courier New', monospace;
        font-size: 0.8em;
        color: #666;
        background-color: #f8f8f8;
        padding: 2px 4px;
        border-radius: 2px;
    }
    .log-content 
    {
        word-wrap: break-word;
        max-width: 400px;
    }
    .error-header { color: red; font-weight: bold; }
    .warning-header { color: orange; font-weight: bold; }
    .info-header { color: blue; font-style: italic; }
    .success-header { color: green; font-weight: bold; }
    .debug-header { color: gray; font-size: 0.9em; }
    .dump-header 
    { 
        color: #6a0dad; 
        font-family: 'Courier New', monospace; 
        background-color: #f0f0f0; 
        padding: 2px 4px; 
        border-radius: 3px;
        font-weight: bold;
    }
</style>
)";
}

namespace HtmlFmt 
{
    constexpr std::string_view opener_start = "<html><head><meta charset=\"UTF-8\">";
    constexpr std::string_view opener_end = "</head><body>\n<h1>LOG JOURNAL</h1>\n<table>\n<thead>\n<tr><th>Timestamp (MSK)</th><th>Level</th><th>Message</th><th>Source</th></tr>\n</thead>\n<tbody>\n";
    
    constexpr std::string_view row_open = "<tr>";
    constexpr std::string_view row_close = "</tr>\n";
    
    constexpr std::string_view data_open = "<td>";
    constexpr std::string_view data_close = "</td>";
    
    constexpr std::string_view closer = "</tbody>\n</table>\n</body></html>\n";
}

class HtmlLogger 
{
private:
    inline static std::ofstream file_;
    inline static std::string filename_;
    
public:
    static void init(const std::string &filename) 
	{
        std::filesystem::create_directories("build/logger");
        filename_ = "build/logger/" + filename + ".html";
        
        file_.open(filename_, std::ios::out | std::ios::trunc);
        if (!file_.is_open()) 
		{
            std::cerr << "Failed to open logger file: " << filename_ << std::endl;
            return;
        }
        
        file_ << HtmlFmt::opener_start 
              << CssFmt::CSS_STYLES 
              << HtmlFmt::opener_end;
    }
    
    static void log(const std::string &head, const std::string &text, 
                   const CssFmt::HeaderStyle &style,
                   const std::string &filename, const std::string &function, int line) 
	{

		// reopen file
        if (!file_.is_open()) 
		{
            file_.open(filename_, std::ios::out | std::ios::app);
            if (!file_.is_open()) 
			{
                std::cerr << "Logger file is not open: " << filename_ << std::endl;
                return;
            }
        }
        
        std::string timestamp = get_moscow_time();
        std::string formatted_header = create_header(head, style);
        std::string source_info = create_source_info(filename, function, line);
        
        file_ << HtmlFmt::row_open
              << HtmlFmt::data_open << "<span class=\"timestamp\">" << timestamp << "</span>" << HtmlFmt::data_close
              << HtmlFmt::data_open << formatted_header << HtmlFmt::data_close
              << HtmlFmt::data_open << "<div class=\"log-content\">" << text << "</div>" << HtmlFmt::data_close
              << HtmlFmt::data_open << "<span class=\"source-info\">" << source_info << "</span>" << HtmlFmt::data_close
              << HtmlFmt::row_close;
        
        file_.flush();
    }
    
    // overload func log, if style name in string format
    static void log(const std::string &head, const std::string &text, 
                   const std::string &style_name,
                   const std::string &filename, const std::string &function, int line) 
	{
        static const std::unordered_map<std::string, CssFmt::HeaderStyle> style_map = 
		{
            {"default", CssFmt::DEFAULT},
            {"error", CssFmt::ERROR},
            {"warning", CssFmt::WARNING},
            {"info", CssFmt::INFO},
            {"success", CssFmt::SUCCESS},
            {"debug", CssFmt::DEBUG},
            {"dump", CssFmt::DUMP}
        };
        
        auto it = style_map.find(style_name);
        const CssFmt::HeaderStyle &style = (it != style_map.end()) ? it->second : CssFmt::DEFAULT;
        
        log(head, text, style, filename, function, line);
    }
    
    static void close() 
	{
        if (file_.is_open()) 
		{
            file_ << HtmlFmt::closer;
            file_.close();
        }
    }

    template<typename... Args>
    static std::string format_stream(Args... args) 
    {
        std::stringstream ss;
        (ss << ... << args);  
        return ss.str();
    }

private:
    static std::string get_moscow_time() 
	{
        // current utc time
        auto now = std::chrono::system_clock::now();
    
        auto moscow_time = now + std::chrono::hours(3);
        auto time_t = std::chrono::system_clock::to_time_t(moscow_time);
        
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            moscow_time.time_since_epoch()) % 1000;
        
        std::stringstream ss;
        ss << std::put_time(std::gmtime(&time_t), "%Y-%m-%d %H:%M:%S");
        ss << "." << std::setfill('0') << std::setw(3) << ms.count();
        
        return ss.str();
    }
    
    static std::string create_header(const std::string &head, const CssFmt::HeaderStyle &style) 
	{
        std::string result = head;
        
        if (!style.css_class.empty() || !style.inline_style.empty()) 
		{
            result = "<span";
            
            if (!style.css_class.empty())
                result += " class=\"" + style.css_class + "\"";
            
            if (!style.inline_style.empty()) 
                result += " style=\"" + style.inline_style + "\"";
            
            result += ">" + head + "</span>";
        }
        
        return result;
    }
    
    static std::string create_source_info(const std::string &filename, 
                                         const std::string &function, int line) 
	{
        std::string file_name = std::filesystem::path(filename).filename().string();
        
        std::stringstream ss;
        ss << file_name << ":" << function << "():" << line;
        return ss.str();
    }
};

#endif
