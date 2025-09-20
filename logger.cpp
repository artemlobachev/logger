#include "logger.hpp"


void test_function() 
{
    HtmlLogger::init("test_log");
    
    LOG_ERROR("Database Error", "Failed to connect to database server");
    LOG_WARNING("Memory Warning", "Memory usage is above 80%");
    LOG_INFO("User Action", "User logged in successfully");
    LOG_SUCCESS("Operation Complete", "File uploaded successfully");
    LOG_DEBUG("Debug Info", "Loop iteration: 42");
    LOG_DUMP("Memory Dump", "0x7fff5fbff7a0: 48 65 6c 6c 6f 20 57 6f 72 6c 64");
    
    LOG("Custom Message", "This is a custom styled message", CssFmt::INFO);
    
    HtmlLogger::close();
}

int main() {
    test_function();
    return 0;
}