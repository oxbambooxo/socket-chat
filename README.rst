.. image:: https://travis-ci.org/oxbambooxo/gtk3-socket-chat.svg?branch=master
    :target: https://travis-ci.org/oxbambooxo/gtk3-socket-chat

Requirement
===========

* cmake >= 3.4
* gtk+3
* json-glib-1.0

Install Submodule
==================

::

    git submodule update --init --recursive

Build Client
============

::

    cd build
    cmake ..
    make client
    ./client/client

Build Server
============

server temporary provior Python server(C version will coming soon),
that need python2.7 and *gevent*::

    cd server
    python main.py <port>

the port argument default is 56789

C-S Protocol
============

package data is serialize's json value,
package send between client and server illustrate like::

    +----------------------------------------------
    | type[4 byte] | length[4 byte] | data[N byte]
    +----------------------------------------------

the data json format depend on difference type value

* Type 1, client send boardcast message

    - client ::

        {
            "message": "hello"
        }

    - server ::

        {
            "message":   "hello",           // message content
            "timestamp": 1465720314.344066, // message timestamp
            "id":        "IVPJu99yiTF",     // sender id
        }

* Type 2, client modify his own information

    - client ::

        {
            "name": "Jack", // user's name
        }

* Type 3, client query some one info

    - client ::

        {
            "id": "IVPJu99yiTF", // ask some one id
        }

    - server ::

        {
            "id":   "IVPJu99yiTF", // user id
            "name": "Jack",        // user's name
        }
