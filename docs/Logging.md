# 🔥 **Ember Log**
---

# Basic overview
*Note: For brevity, `el` is an alias of the namespace `ember::log`. This alias will be used in all code examples.*

Ember uses its own lightweight, high-performance logging library built to meet the exact needs of the project. Existing libraries were evaluated but they either carried too much baggage and complexity, had performance short-comings or lacked a desired feature.

Logging in Ember is primarily intended to help server operators assess the health of their server, as well as providing a basic form of debugging for developers. It is *not* intended to be used to generate logs required for administrative tasks such as checking login history, player trades, chat logs and character recovery; such information should be stored in a database with transactional capabilities.

The logger consists of two main components that the user must deal with: the logger object and sinks.

# Logger Creation
To create a logger, simply do the following:
```cpp
#include <logger/Logging.h>

auto logger = std::make_unique<el::Logger>();
```

That's all there is to it.

# Sinks
In order to actually send anything to the logger, sinks must be created and assigned to the logger object. Sinks simply provide an output for the logging messages.

Ember provides three sinks; console, file and remote (syslog). Each sink requires different arguments in order to construct it.

### Console Sink
To create a console sink, simply do:
```cpp
#include <logger/ConsoleSink.h>

auto sink = std::make_unique<el::ConsoleSink>(el::Severity::INFO, el::Filter(0));
```

The console sink has the ability to colourise the output. To enable this option, simply do:
```cpp
sink->colourise(true); // false by default
```

### File Sink
The file sink requires three arguments; severity, filename to write the logs to and a mode that indicates, in cases where the file already exists, whether to create a new file for logging or to append to an existing file. If instructed to create a new file, it will rename the existing file, not overwrite it.

Creation example:
```cpp
#include <logger/FileSink.h>

auto sink = std::make_unique<el::FileSink>(el::Severity::DEBUG, el::Filter(0), "my_log.log", el::FileSink::Mode::APPEND);
```

Filenames may include formatters as specified by C++11's put_time function. See http://en.cppreference.com/w/cpp/io/manip/put_time.

After creation, the sink may be further configured:
```cpp
sink->size_limit(5); // maximum allowed log size in MB - log will be rotated before hitting this limit
sink->log_severity(true); // whether to write the log message severity to the file
sink->log_date(true); // whether to write the timestamp to the file
sink->time_format("%Y"); // format used for writing timestamps - uses C++11's put_time formatters
sink->midnight_rotate(true); // whether to rotate the file at midnight
```

### Remote Sink
The remote sink implements the UDP-based syslog protocol to allow log messages to be sent to remote machines. This could be useful when used in conjunction with third-party remote log viewers; for example, setting up email/text-message notifications if an error is encountered.

The remote sink requires five arguments; severity, the host to send the logs to, the remote port, facility and service name. 

In Ember, the facility is always set to LOCAL_USE_0 (although it can be changed) and the service name is usually the name of the application sending the log messages. To learn more, see: http://en.wikipedia.org/wiki/Syslog

Creation example:
```cpp
#include <logger/SyslogSink.h>

auto sink = std::make_unique<el::SyslogSink>(el::Severity::ERROR, el::Filter(0), "localhost", 514, el::SyslogSink::Facility::LOCAL_USE_0, "login");
```

# Registering Sinks
After creating the desired sinks, they must be registered with the logger. This requires giving the logger object ownership of the unique_ptr holding the sink.

For example:
```cpp
auto sink = std::make_unique<el::ConsoleSink>(el::Severity::INFO, el::Filter(0));
auto logger = std::make_unique<el::Logger>();
logger->add_sink(std::move(sink));
```

# Outputting Messages
Now that we've created a logger and provided it with sinks, we can use it for logging. To log a message, simply follow the form:
```cpp
LOG_DEBUG(logger) << "Hello, world!" << LOG_ASYNC; // async log entry
LOG_INFO(logger) << "This is a blocking entry!" << LOG_SYNC; // blocking log entry
```

### LOG_SYNC vs LOG_ASYNC
LOG_ASYNC should be the preferred method of logging. It will always return immediately, without blocking the thread while the message is flushed by the sinks. In contrast, LOG_SYNC will block the thread's execution until the message has been flushed by the sinks. This could be useful when attempting to insert log entries for crash debugging in the absence of a crash handler.

#### Formatted Messages
An alternative to the stream interface is to use string formatting. The underlying mechanism uses C++20's std::format library and so the logger follows the same rules with the exception that a formatted string with no arguments passed will be logged without being parsed.
```cpp
LOG_DEBUG_FMT(logger, "They're taking the {} to {}", "Hobbits", "Isengard"); // They're taking the Hobbits to Isengard
LOG_DEBUG_FMT(logger, "The quick brown {1} jumps over the lazy {0}", "dog", "fox"); // The quick brown fox jumps over the lazy dog
LOG_DEBUG_FMT(logger, "This won't be parsed for arguments {0}{1}"); // This won't be parsed for arguments {0}{1}
```

Note that formatted logging is async only.

# Global Logging
Although it is not recommended for normal development, it may be useful to perform logging in a section of the code that doesn't have access to a logger object. For those situations, you should set a logger object as the global logger:
```cpp
el::set_global_logger(logger.get()); // where logger is a unique_ptr
```

You can then use the LOG_SEVERITY_GLOB macros to perform logging:
```cpp
#include <logger/Logging.h>

LOG_WARN_GLOB << "Hello, world! This is the global logger!" << LOG_ASYNC;
```

# Filtering
Each logging sink can have a filter. A filter allows the end-user to configure which categories of messages they'd like to include/exclude. The filter mask works as a blacklist - that is, a filter value of zero will allow all messages through and any set bits will block the corresponding message category.

The filter is a 32-bit integer, with each bit representing a message category.

Example usage:
```cpp
// define the filter values
enum Filter {
    RESERVED         = 1, // do not use
    PLAYER_MOVE      = 2,
    PLAYER_TRADE     = 4,
    TRANSPORT_ARRIVE = 8,
    TRANSPORT_DEPART = 16,
    NPC_ACTION       = 32,
    SERVER_ALERT     = 64,
    ...
};

// set filter and create sink
el::Filter(PLAYER_MOVE | SERVER_ALERT); // filter value is 66
auto sink = std::make_unique<el::SyslogSink>(severity, filter, host, port, facility, service);
```
Note that this is only one possible way of defining and using filters. You may use other techniques (defines, bitfields, enum class, etc) if you prefer.

To allow uncategorised messages to be filtered, all uncategorised log messages are implicitly given the category value of '1'. As such, '1' should be considered a reserved value (as shown above).

To a log a message with a specified category:
```cpp
LOG_DEBUG_FILTER(logger, PLAYER_TRADE) << "Example message" << LOG_ASYNC;
```

With a global logger:
```cpp
LOG_DEBUG_FILTER_GLOB(PLAYER_TRADE) << "Example message" << LOG_ASYNC;
```

# Compile-Time Disabling
If desired, logging statements can be removed from the produced binary entirely via defines. To remove *all* logging statements, define "NO_LOGGING". To selectively remove statements for a severity, define "NO_SEVERITY_LOGGING" where SEVERITY is replaced with the severity to remove.

Note that the disabling macros prepend the logging statements with an *if(false)* conditional to allow for the compiler to optimise-out. If optimisations are disabled, the compiler may not remove all traces of the logging statements.

# Thread Safety
Logger objects and all sinks are entirely thread safe. It is also safe to create multiple logger instances within a single process, even when using console sinks (assuming C++11).

# Performance Notes
* When the message queue hits a certain size, the logger will begin writing log entries in batches rather than individually. This does not apply to the remote sink.
* File entries are written in binary mode rather than ASCII. This means that newline characters will not be converted to \r\n on Windows but this should not pose a problem for most text viewers.
* The logger can only log as fast as its slowest sink, with the slowest sink being console output and the fastest being the file sink. This shouldn't be an issue unless you're doing a copious amount of logging (e.g. writing every packet on a busy realm, for some reason).
* The LOG_SEVERITY macros will prevent any evaluation taking place if none of the registered sinks are interested in receiving messages of the specified severity. This means it's okay to have expensive function calls inside logging statements as they won't be evaluated unless needed. The overhead of a log message that doesn't need to be sent to any sinks is a single integer comparison (regardless of number of sinks).
* Formatting timestamps for file entries will significantly decrease throughput.
* For maximum performance, use only a file sink with timestamp and severity logging disabled. In testing, this allowed logging at a rate that appeared to be limited by the throughput of the SSD being used to save the entries to.

# Limitations & Future Work
* If the application crashes and there are LOG_ASYNC entries that are yet to be written by the sinks, they may be lost. This should be possible to solve after crash handlers have been added to Ember.
* Memory exhaustion is possible, albeit highly unlikely, when using LOG_ASYNC if the logger is unable to keep up with the rate of message receival. This could be solved by adding a back-pressure mechanism to the queue.
* Log entries from different threads may not retain their original ordering. That is, for example, if thread #1 logs "Hello" and thread #2 logs ", world!" shortly after, the logger could output the entries as ", world!" and "Hello". Log entries will retain their original ordering on a per-thread basis. This is a result of optimisations in the multi-producer, multi-consumer queue implementation used by the logger. 
* For performance purposes, timestamps are generated by each sink (as timestamps are a configurable option for each sink) when the messages are flushed by the logger's worker thread, not when the message is queued. The implication is that when messages are written in batches, all messages of the same batch will be given the same timestamp on a per-sink basis, meaning there may be small variations in time for the same message but sent to different sinks. This could be partially rectified by generating a single timestamp and passing it to all registered sinks rather than having each sink generate its own timestamp.
