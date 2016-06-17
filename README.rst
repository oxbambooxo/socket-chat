Requirement
===========

* cmake
* gtk+3
* json-glib


Build Client
============

::

    mkdir build
    cd build
    cmake ..
    make client
    ./client

Build Server
============

server temporary provior Python server(C version will coming soon), that need python2.7 and *gevent*::

    cd server
    python main.py <port>

the port argument default is 56789

C-S Protocol
============

package data is serialize's json value,
package send between client and server illustrate like::

    +----------------------------------------------
    | length[4 byte] | data[N byte]
    +----------------------------------------------

the data json format are the following:

* client send message ::

    {
        "type": 1,
        "message": "hello",  // message content
    }

* server broadcast message ::

    {
        "type": 1,
        "message": "hello",  // message content
        "timestamp": 1465720314.344066,  // message timestamp
        "sender": "IVPJu99yiTF",  // sender id
    }

* client setting self name ::

    {
        "type": 2,
        "name": "Jack",  // user's name
    }

* client require the name ::

    {
        "type": 3,
        "sender": "IVPJu99yiTF",  // ask sender id
    }

* server return client the name ::

    {
        "type": 3,
        "sender": "IVPJu99yiTF",  // sender id
        "name": "Jack",  // user's name
    }





