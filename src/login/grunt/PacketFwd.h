/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

namespace ember::grunt {

struct Packet;

namespace server {

class LoginChallenge;
class LoginProof;
class RealmList;
class ReconnectChallenge;
class ReconnectProof;
class TransferData;
class TransferInitiate;

} // server

namespace client {

class LoginChallenge;
class LoginProof;
class ReconnectProof;
class RequestRealmList;
class SurveyResult;
class TransferAccept;
class TransferCancel;
class TransferResume;

} // client

} // grunt, ember