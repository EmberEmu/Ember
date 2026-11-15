# Fusion
Fusion is a service primarily aimed at reducing development friction by allowing for multiple services to be run and managed within a single process.

When the services are built as shared libraries, Fusion can used to reload the service binaries while keeping the rest of the service stack running. If statically linked, Fusion itself will need to be restarted in order for changes to be reflected, although services can still be individually controlled.

## Configuration
Configuration is provided through a configuration file (fusion.conf), allowing the developer to specify which services should initially launch (typically all of them), along with basic logging options.

## Commands
Various commands are available to control which services are active.

> service start [name]

> service stop [name]

> service restart [name]

Stopping/starting or restarting a service when the services have been built as shared libraries will cause the new binaries to be loaded into Fusion.