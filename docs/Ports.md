# 🔥 **Ember Ports**
---

# Basic overview

The Ports library offers Ember's services the ability, when configured by the user, to automatically open ports when behind a NAT (network address translation) gateway. The vast majority of residential IPv4 connections are behind NAT gateways, which allow multiple devices to share a single external IPv4 address. 

Although servers, particularly MMO servers, are largely run on enterprise networks which do not deploy NAT, users do run open source servers on their residential connections for development and testing, or to just play with a small group of friends.

When behind a NAT gateway, port forwarding is required to make the service available across the Internet. This can often be a source of frustration for users, particularly for those not technically minded or familiar with NAT. The user must be aware of which ports are needed, the internal address of their machine, as well as their (external) IP address in order to get the software working. This library aims to provide a solution by allowing the services to automatically open ports and configure the server as necessary.

# UPnP, NAT-PMP, Port Control Protocol. Huh?
These are three main technical standards that allow for automatically opening/forwarding ports on a NAT gateway. As a residential router may implement any and all of these protocols, or none, Ports provides support for all of them.

## UPnP
UPnP (Universal Plug and Play) is the most complex of the three, largely because it offers functionality that goes well beyond simply managing ports. The general jist is this:
1) A service uses multicast UDP to broadcast a request to the entire local network, requesting devices or services that it is interested in using. In this case, we're interested in Internet gateway devices (a.k.a routers that implement NAT).
2) A UPnP-capable device receives the request and checks to see whether it matches. If it does, it sends a response containing its details directly to the server that sent the broadcast.
3) The service receives the response, parses it, then opens a HTTP connection to request further information about the device and the services it offers. It'll request/receive multiple XML files containing available servers and the functions exposed by those services.
4) Once the service has all of the information it needs, it can effectively call a function on the device and receive a response, using an XML-based protocol known as SOAP.

## NAT-PMP
NAT-PMP (NAT Port Mapping Protocol), defined by RFC6886, is the simplest of the three. It is a binary protocol whose solution problem space is how to open ports on residential gateways. Aside from allowing users to request their external IP address, it does nothing more.

## PCP
PCP (Port Control Protocol), defined by RFC6887, is the successor to NAT-PMP. It offers some additional functionality and flexibility; IPv6 support, the ability to open ports on machines other than the one sending the request, and other options suited to a more complex operating environment (e.g. an enterprise network).

# Library Usage
Using the library is fairly straightforward but differs based on which protocol you wish to use.

## Note
All examples use `using namespace ember;` for brevity.
## UPnP
Here's an example of opening a port on a gateway that supports UPnP.
```cpp
#include <boost/asio/io_context.hpp>
#include <ports/upnp/ssdp.h>

// ... function preamble...

boost::asio::io_context ctx;            // the library requires an ASIO io_context to run
ports::upnp::SSDP ssdp(interface, ctx); // interface is a string containing the machine's LAN IP address

ssdp.locate_gateways([&](const ports::upnp::LocateResult& result) {
    if(!result) {
        std::cout << std::to_underlying(result.error()) << '\n';
        return true;
    }

	// Build our port mapping request to send to the device
    ports::upnp::Mapping map {
        .external = 3724, // the port to open to the world
        .internal = 3724, // the port that all traffic to the external port should be forwarded to
        .ttl      = 7200, // time-to-live (a.k.a lease) in seconds - port will be closed after this elapses
        .protocol = ports::Protocol::TCP // can be TCP or UDP
    };

	// use the device found during SSDP to request it to add a port mapping
    result->device->add_port_mapping(map, [&, map](ports::upnp::ErrorCode result) {
        if(result == ports::upnp::ErrorCode::SUCCESS) {
            std::print("Port {} mapped successfully", mapping.external);
        } else {
            std::print("Port mapping failed, error {}", std::to_underlying(result));
        }
    });

    return false;
});

ctx.run();
```
Here's a breakdown of what's going on in this example.
1) We create an ASIO `io_context`, which is used to run the networking functionality in the library. The user must provide it as it offers greater configuration flexibility than if the library were to simply create its own. For example, you could have the service run on your own thread pool or within the context of your main thread rather than requiring the service to create its own threads.
2) We create `SSDP` instance, passing it the context and a string representing the machine's LAN IP.
3) We request the SSDP service to send a multicast on the LAN to search for Internet gateway devices (in short, your router). The callback is invoked each time it receives a response from a device claiming to be compatible.
4) Whenever we get a result, we need to ensure that it was successful. This shouldn't fail frequently but the error code is provided for diagnostic purposes.
5) We initialise a structure, filling it with the details of the port mapping to be created.
6) We call `add_port_mapping`, passing it our filled structure and a callback to be invoked when it gets a response from the device.
7) Before leaving the scope of our SSDP callback, we `return false`. This signals to the SSDP service that we are not interested in receiving further callbacks if multiple compatible devices respond to our search.

Here's the same example, using coroutines instead of callbacks.
```cpp
int main() {
    ports::upnp::Mapping map {
        .external = 3724,
        .internal = 3724,
        .ttl      = 7200,
        .protocol = ports::Protocol::TCP
    };

	boost::asio::io_context ctx;
	
	// Run io_context on another thread
	auto worker = std::jthread(
		static_cast<size_t(boost::asio::io_context::*)()>(&boost::asio::io_context::run), &ctx
	);

	ba::co_spawn(ctx_, add_port_mapping(ctx, mapping), ba::detached);
}

boost::asio::awaitable<void> add_port_mapping(boost::asio::io_context& ctx,
                                              ports::upnp::Mapping map) {
	using awaitable = ports::upnp::use_awaitable;

	ports::upnp::SSDP ssdp(interface, ctx, awaitable);
    auto locate_res = co_await ssdp.locate_gateways(awaitable);

    if(!locate_res) {
        return;
    }

    auto result = co_await locate_res->device->add_port_mapping(map, awaitable);
}
```

Here's the same again but with `std::future`.

```cpp
void add_port_mapping(boost::asio::io_context& ctx, ports::upnp::Mapping map) {
	using future = ports::upnp::use_future;

	ports::upnp::SSDP ssdp(interface, ctx);
    auto lg_future = ssdp.locate_gateways(future);
	auto result = lg_future.get();

    if(!result) {
        return;
    }

    auto apm_future = result->device->add_port_mapping(map, future);
	auto ec = apm_future.get();

    if(!ec) {
	    std::cout << "Great success!";
    }
}
```

Note that the coroutine and future versions will only return a single device result. The SSDP class needs to be redesigned to queue devices for retrieval to allow for these versions to return multiple devices.

### Deletion
Deletion (`delete_port_mapping`) is virtually the same as adding a port mapping. Simply omit the internal port and TTL in the mapping request structure. Fields other than `external` and `protocol`  will be ignored as the protocol unmaps by matching these two values.

### Thread Safety
The UPnP facilities are fully thread-safe. You may issue parallel requests to a single returned device on multiple threads, as may be the case if your `io_context` is running on a thread pool.

## NAT-PMP & Port Control Protocol
Because NAT-PMP and PCP are so closely related, the library implements them as a single service. As recommended by their specifications, clients should default to using PCP and only downgrading to NAT-PMP if a device indicates that it does not support PCP. This means that the library will first attempt to use PCP and will automatically downgrade and resend the request in NAT-PMP format if required. If the remote device does not understand either protocol, the library will return a `SERVER_INCOMPATIBLE` error.

While implementing both protocols in a single service makes life easier for the user through automatically downgrading and retrying failed requests, it means that functionality is largely restricted to the subset supported by NAT-PMP. Some PCP-only functionality is supported but `disable_natpmp(true)` should be called beforehand.

Here's an example of mapping a port with NAT-PMP/PCP.

```cpp
const ports::MapRequest request {
	.protocol      = ports::Protocol::TCP,
	.internal_port = 8085,
	.external_port = 8085,
	.lifetime      = 7200u
};

boost::asio::io_context ctx;
ports::Client client(interface, gateway, ctx);

// Run io_context on another thread
auto worker = std::jthread(
	static_cast<size_t(boost::asio::io_context::*)()>(&boost::asio::io_context::run), &ctx
);

std::future<ports::Result> future = client.add_mapping(request);
auto result = future.get();

if(result) {
	std::print("Success! Mapped {} -> {} for {} seconds\n",
	           result->external_port,
	           result->internal_port,
	           result->lifetime);
} else {
	std::cout << "Error: could not map port";
}
```

Here's the same but with the `std::future` is replaced by a callback.
```cpp
bool strict = false; // more on this below

client.add_mapping(request, strict, [&](const ports::Result& result) {
	if(result) {
		std::print("Success! Mapped {} -> {} for {} seconds\n",
			       result->external_port,
				   result->internal_port,
		           result->lifetime);
	} else {
		std::cout << "Error: could not map port";
	}
});
```
### Strict Mode
When the requested external port is unavailable, NAT-PMP and PCP will choose an available port, likely the nearest available number. Although NAT-PMP offers no way to disable this behaviour, PCP does through a mode called `prefer failure`. Setting the strict argument to true when calling `add_mapping` will enforce this mode with PCP. If the client downgrades to NAT-PMP, it will lose this behaviour. Call `disable_natpmp(true)` if you want to guarantee no mapping is created if the requested port is unavailable.

Because the port that was actually assigned is returned with the result, you should usually be able to compensate for not getting your first choice. Simply advertise your Internet-facing service on this port instead.

### Wildcard Ports
If you don't care about the port you're given, you can simply assign `0` to the `external_port` field in the mapping request structure. You can check the result to discover which port was assigned to you.

### Refreshing Mappings
The remote device may place limits on how long a port mapping is valid before it will clear it, so always check the result to discover how long the device has promised to keep the mapping valid for. It may not be the same value as you requested. To refresh mapping duration, simply repeat the request but ensure that the `external_port` field is set to the value that was returned by the remote device, in case it differs from your request.

Additionally, for PCP, the `nonce` field in the mapping request should match that of the original request. So, if you wish to refresh a PCP mapping, ensure you set it to the same value as was originally used for the initial request. If did not set a value initially, the client handles it internally and will return the actual used value with the mapping request result.

If a mapping is not refreshed before it expires, the remote device will *may* close all connections that are using the port.

### Deletion
Deleting a mapping with `delete_mapping` only requires the internal port and protocol. For example:
```cpp
auto result = client.delete_mapping(3724, ports::Protocol::TCP);

if(result) {
	std::cout << "Wahoo!";
}
```

### External Address
If you want to get your external IP address, call `external_address`. For example:
```cpp
auto future = client.external_address();
const auto result = future.get();

if(result) {
	const auto v6 = boost::asio::ip::address_v6(result->external_ip);
	std::print("External address: {}", v6.to_string());
} else {
	std::cout << "Error: could not retrieve external address";
}
```
Addresses are returned in IPv6 format. If your address is IPv4, it will be converted to an IPv4 mapped IPv6 address. Such an address is represented as `::FFFF:a.b.c.d`, where the last four bytes are the same as they would be an IPv4 address stored as an integer, with the two prior bytes indicating that this is a mapped address.

As NAT-PMP does not fully support IPv6, the results of requesting an external IPv6 address through NAT-PMP are not clear. To be certain that you won't run foul of any odd behaviour, call `disable_natpmp(true)`.

### Usage Notes
It's important that you do not make simultaneous mapping requests. This is because NAT-PMP does include any form of transaction ID to allow for uniquely identifying in-flight requests. As the protocol runs over UDP, this could lead to multiple requests being handled incorrectly. Although PCP does include identifiers and the library validates that the response matches the request, it does not allow for more than one identifier to be tracked at a time, given the additional complexity induced by supporting both protocols. If you *really* need to issue simultaneous requests, do so on different `Client` instances, although this is not recommended. Additionally, the RFCs advise against issuing multiple requests to avoid potentially overloading the remote service given that it's likely a low-power home device, although this is not likely to be an issue in reality.

### Thread Safety
`Client` is internally thread-safe, which means it's safe to provide it with an `io_context` running in a thread pool.

## NAT-PMP/PCP Daemon
As manually managing mappings, particularly short-lived ones, can be ardous, a helper service is provided to automatically refresh mappings.

Here's an example of using the service to create a mapping that will live for at least the lifetime of the process. The arguments and return values are identical to those of `Client`.
```cpp
const ports::MapRequest request {
	.protocol      = ports::Protocol::TCP,
	.internal_port = 8085,
	.external_port = 8085,
	.lifetime      = 7200u
};

boost::io_context ctx;
ports::Client client(interface, gateway, ctx);
ports::Daemon daemon(client, ctx);

daemon.add_mapping(MapRequest request, bool strict, [&](const ports::Result& result) {
	if(result) {
		std::print("Success! Mapped {} -> {} for {} seconds\n",
			       result->external_port,
				   result->internal_port,
		           result->lifetime);
	} else {
		std::cout << "Error: could not map port";
	}
});
```

Once you've successfully mapped a port via the daemon, it will attempt to keep it alive for the entire duration of the process. It can also detect if the remote device has potentially lost its mapping (power loss, reboot, etc) and will attempt to re-request all lost mappings.

### Event Callback
Because it's possible for mapping refreshes to fail, you may wish to register a callback to receive key events from the daemon. Alternatively, you could use it for logging purposes.

```cpp
daemon.event_handler([](ports::Daemon::Event event, const MapRequest& request) {
	// handle the event here
});
```

The `event`  argument is one of several possible events (add, delete, renew failure, mapping expired) and the `request` argument is the mapping that the event is applicable to. This is same structure as used for adding/deleting a mapping.

## Conclusion
The library provides easy interfaces for managing ports with the three major protocols. However, it does not support the full suite of protocol features, particularly with UPnP, since they aren't required in Ember's usecase. However, expanding the supported features would be fairly straightforward if necessary.