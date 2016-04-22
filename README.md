# Twilio-CNAM #

This is a simple little project I came up with, its sole purpose is using the [Twilio Lookup API](https://www.twilio.com/lookup)
to perform CNAM lookups.

### Why is this useful? ###

If you have an Asterisk server, like I do, and are using Twilio as your SIP provider, you might know that Twilio does not provide caller names
with the caller ID sent to Asterisk. I was using [OpenCNAM](http://www.opencnam.com/), which provides an even nicer API, however they seem
to be deprecating their old free hobbyist tier. They are cheaper than Twilio for lookups, but I already have a Twilio account ;p

### So how do I use it ###

Well, it's simple, you put your Twilio AccountSID and AuthToken inside of tokens.hpp and run build.sh, well, provided you have curl and sqlite3
development headers installed on your system somewhere.

And then do something like this in your extensions.conf where your calls come in from Twilio (or any other SIP provider for that matter 
if they don't provide caller names). 


~~~~
[pstn-incoming]
exten = _[+]1NXXNXXXXXX,1,NoOp(${EXTEN:2})
same = n,Set(CALLERID(name)=${SHELL(/usr/bin/twilio-cnam ${CALLERID(num)})})
same = n,NoOp(This caller's name is: ${CALLERID(name)})
same = n,NoOp(This caller's number is: ${CALLERID(num)})
same = n,Set(CID=${CALLERID(num)})
same = n,NoOp(This caller's fixed number is: ${STRREPLACE(CID,"+1")})
same = n,Set(CALLERID(num)=${STRREPLACE(CID,"+1")})
same = n,Dial(SIP/${EXTEN:2}&Local/incoming@from-internal,30)
same = n,VoiceMail(${EXTEN:2},u)
same = n,Hangup()
~~~~

You can also change where the SQLite cache is located inside of twilio-cnam.cpp and the number of seconds the cached
record is valid for the default is 1 week (604800 seconds).


### Credits ###

nlohmann for his [excellently simple single file json c++ library](https://github.com/nlohmann/json)

\#asterisk on freenode

