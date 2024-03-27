# 🔥 **Spark RPC Generator (rpcgen)**
---

This tool is used for converting FlatBuffers schemas into base classes for implementing services within Ember.

Each service is defined within a single `.fbs` file, which is then used to produce the FlatBuffers code for building messages belonging to that service. An additional `.bfbs` file is produced, which is then given to this tool, allowing it to use reflection to generate the boilerplate for handling and dispatching messages for a given service.