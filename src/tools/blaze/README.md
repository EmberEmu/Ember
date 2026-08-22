# 🐉 Blaze Headless Client

Blaze is a minimal headless client primarily intended for running end-to-end and stress tests on Ember.

### Implementation
Blaze implements an API via an SDK which allows plugins to interact with the emulator without requiring the user to handle networking, encryption, or any of the protocol logic. 

Only one official plugin is provided, `blazeas`, which allows running test scripts written in AngelScript to be executed against Blaze.

This is all very WIP. :)
