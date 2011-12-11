#!/usr/bin/env python

import socket
import sys
import time

QUIT_SIGNATURE = "__pwfarm_quit"
STATE_PATH = None

def main():
    if sys.argv[1] == "quit_signature":
        print QUIT_SIGNATURE
        sys.exit( 0 )

    global STATE_PATH
    STATE_PATH = sys.argv[1]

    mode=sys.argv[2]

    if mode == "server":
        server()
    elif mode == "quit":
        send_quit()
    elif mode == "set":
        send_set( sys.argv[3] )
    elif mode == "get":
        send_get()
    elif mode == "wait":
        send_wait()
    else:
        print "illegal mode", mode
        sys.exit( 1 )

    sys.exit( 0 )

def init_send( cmd ):
    port = None

    while port == None:
        try:
            f = open( STATE_PATH, 'r' )
            content = f.read()
        except:
            time.sleep( 1 )
            continue

        if content == QUIT_SIGNATURE:
            print QUIT_SIGNATURE
            sys.exit( 0 )
        else:
            try:
                port = int(content)
            except:
                time.sleep( 1 )
                continue

    s = socket.socket( socket.AF_INET, socket.SOCK_STREAM )
    try:
        s.connect( ('localhost', port))
    except:
        print QUIT_SIGNATURE
        sys.exit( 0 )
    s.send( cmd )
    return s

def send_quit():
    s = init_send( 'Q' )
    s.close()

def send_set( status ):
    s = init_send( 'S' )
    s.send( status )
    s.close()

def send_get():
    s = init_send( 'G' )
    status = s.recv(1024)
    print status
    s.close()

def send_wait():
    s = init_send( 'W' )
    status = s.recv(1024)
    print status
    s.close()

def server():
    status="???"
    status_dirty = False
    pending_wait = None

    port = 11234
    s = socket.socket( socket.AF_INET, socket.SOCK_STREAM )

    while True:
        try:
            s.bind( ('', port) )
            break
        except:
            port += 1
            continue

    f = open( STATE_PATH, 'w' )
    f.write( str(port) )
    f.close()

    s.listen( 5 )

    while True:
        conn, addr = s.accept()
        command = conn.recv(1)

        if command == 'Q':
            break
        elif command == 'S':
            status = conn.recv(1024)
            conn.close()
            if pending_wait:
                status_dirty = False
                try:
                    pending_wait.send( status )
                except:
                    pass
                pending_wait = None
            else:
                status_dirty = True
        elif command == 'G':
            status_dirty = False
            conn.send( status )
        elif command == 'W':
            if pending_wait:
                try:
                    pending_wait.send( status )
                except:
                    pass

            if status_dirty:
                conn.send( status )
                pending_wait = None
                status_dirty = False
            else:
                pending_wait = conn

    if pending_wait != None:
        pending_wait.send(QUIT_SIGNATURE)

    s.close()

    f = open( STATE_PATH, 'w' )
    f.write( QUIT_SIGNATURE )
    f.close()

main()
