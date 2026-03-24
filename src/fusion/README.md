# Fusion
Fusion is a service primarily aimed at reducing development friction by allowing for multiple services to be run within a single process rather than having to manually start/stop multiple processes.

When the project is using dynamic linking, Fusion can used to reload the binaries of changed services while keeping the rest of the service stack running. If statically linked, Fusion itself will need to be restarted in order for changes in the services to be reflected.

## Configuration
Configuration via the config file is fairly straightforward and allows the developer to specify which services should initially launch (typically all of them), along with basic logging options.

## Commands
Various commands are available to control which services are active.

> service start [name]

> service stop [name]

> service restart [name]

Stopping/starting or restarting a service when the services have been built as shared libraries will cause the new binaries to be loaded into Fusion.

