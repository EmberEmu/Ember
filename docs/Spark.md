# 🔥 **Ember Spark**
---

# Overview
*Note: For brevity, `es` is an alias of the namespace `ember::spark`. This alias will be used in all code examples.*

As Ember aims to be a modular architecture, there is a requirement for a robust networking library that can allow for various game services to be networked together with mininal effort.

Spark is a work-in-progress library intended to be used exclusively for Ember's low-volume inter-server control protocol.

Under the hood, Spark uses Boost's ASIO for networking and Google's FlatBuffers for serialisation.

Spark is divided into two primary components; the core service and service discovery.

# Usage tutorial
A single Spark service instance should be created for each application - all services that the application wishes to expose/consume will go through this instance.

Example instansiation:
```cpp
// std::string description, boost::asio::io_service& service, const std::string& interface, std::uint16_t port, log::Logger* logger, log::Filter filter

es::Service spark(
	"name",      // an identifier for this application (e.g., login)
	io_service,  // ASIO io_service instance
	"127.0.0.1", // IPv4/6 interface to bind to
    6000,        // port to listen on
    logger,      // logger object
    filter       // log filter ID
);
```

Once you have a Spark Service object, you can then register a service event handler.
```cpp
spark.dispatcher()->register_handler(
	this, // object implementing the EventHandler interface
	messaging::Service::RealmStatus, // service type
	es::EventDispatcher::Mode::SERVER // are we a SERVER or CLIENT?
);
```

All service event handlers must inherit from `spark::EventHandler` and provide definitions of its pure-virtual member functions.
```cpp
#include <spark/Service.h>

class TestService : public es::EventHandler {
public:
	void handle_message(const es::Link& link, const messaging::MessageRoot* root) {
    	// implement
    }
    
	void handle_link_event(const spark::Link& link, spark::LinkState event) {
        // implement
    }
};
```

Let's take a look at the member functions `handle_message` and `handle_link_event`.

#### `handle_link_event`
When a remote peer that your service is capable of communicating with goes offline or comes online, this handler will be called with two arguments; a link object representing the connection to the peer and an enum indicating the event (currently only LINK_UP or LINK_DOWN).

When a link comes online, your service may wish to initiate communication with the remote peer and when the link goes offline, you may wish to perform clean-up.

#### `handle_message`
When a remote peer sends your service a message, this handler will called with a reference
