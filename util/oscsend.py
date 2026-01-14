#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright (C) Kushview, LLC.
# SPDX-License-Identifier: GPL-3.0-or-later

"""
Send OSC messages to Element audio engine.

Element listens on:
  /element/command       - for application commands
  /element/engine        - for engine parameters (e.g., samplerate)

Default port: 9000

Note: UDP is connectionless, so there's no way to verify if the server
received the message. Make sure Element is running and OSC is enabled.
"""

import argparse
import socket
import struct
import sys


def osc_string_encode(s):
    """Encode a string for OSC format (null-terminated, 4-byte aligned)."""
    data = s.encode('utf-8') + b'\x00'
    # Pad to 4-byte boundary
    padding = (4 - len(data) % 4) % 4
    return data + (b'\x00' * padding)


def osc_int_encode(value):
    """Encode a 32-bit integer for OSC."""
    return struct.pack('>i', value)


def osc_float_encode(value):
    """Encode a 32-bit float for OSC."""
    return struct.pack('>f', value)


def build_osc_message(address, args=None):
    """
    Build an OSC message.
    
    Args:
        address: OSC address (e.g., '/element/command')
        args: List of tuples (value, type) where type is 'i', 'f', or 's'
    
    Returns:
        Bytes of the complete OSC message
    """
    msg = osc_string_encode(address)
    
    if args is None:
        args = []
    
    # Build type tag string
    type_tag = ','
    data = b''
    
    for value, arg_type in args:
        type_tag += arg_type
        if arg_type == 'i':
            data += osc_int_encode(value)
        elif arg_type == 'f':
            data += osc_float_encode(value)
        elif arg_type == 's':
            data += osc_string_encode(value)
    
    msg += osc_string_encode(type_tag)
    msg += data
    
    return msg


def send_osc_message(host, port, address, args=None, verbose=False):
    """Send an OSC message to the specified host:port."""
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        osc_msg = build_osc_message(address, args)
        
        if verbose:
            print(f"Sending to {host}:{port}")
            print(f"  Address: {address}")
            if args:
                for val, typ in args:
                    print(f"  Arg ({typ}): {val}")
        
        sock.sendto(osc_msg, (host, port))
        sock.close()
        return True
    except socket.gaierror:
        print(f"✗ Hostname resolution failed: {host}")
        return False
    except OSError as e:
        print(f"✗ Send failed: {e}")
        return False
    except Exception as e:
        print(f"✗ Error: {e}")
        return False


def main():
    parser = argparse.ArgumentParser(
        description='Send OSC messages to Element audio engine',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Element OSC Addresses:
  /element/command       - Application commands (expects string argument)
  /element/engine        - Engine parameters (samplerate: "samplerate" <int>)

Note: UDP is connectionless, so there is no way to verify if the server
received the message. Make sure Element is running and OSC is enabled.

Examples:
  # Send samplerate change
  %(prog)s localhost 9000 --samplerate 44100
  
  # Verbose output
  %(prog)s 127.0.0.1 9000 --samplerate 48000 -v
  
  # Custom port
  %(prog)s myhost.local 8000 --samplerate 44100
        """
    )
    
    parser.add_argument('host', nargs='?', default='localhost',
                       help='Element host address (default: localhost)')
    parser.add_argument('port', nargs='?', type=int, default=9000,
                       help='OSC server port (default: 9000)')
    parser.add_argument('-v', '--verbose', action='store_true',
                       help='Print verbose output')
    parser.add_argument('--samplerate', type=int,
                       help='Set engine samplerate (e.g., 48000)')
    parser.add_argument('--command', type=str,
                       help='Send a command (e.g., quit, save, load)')
    
    args = parser.parse_args()
    
    print(f"Element OSC Sender")
    print(f"Target: {args.host}:{args.port}")
    print()
    
    sent_any = False
    success = True
    
    # Send engine samplerate if requested
    if args.samplerate is not None:
        sent_any = True
        if send_osc_message(args.host, args.port, '/element/engine',
                           [('samplerate', 's'), (args.samplerate, 'i')], args.verbose):
            print(f"✓ Sent samplerate={args.samplerate}")
        else:
            success = False
    
    # Send command if requested
    if args.command is not None:
        sent_any = True
        if send_osc_message(args.host, args.port, '/element/command',
                           [(args.command, 's')], args.verbose):
            print(f"✓ Sent command={args.command}")
        else:
            success = False
    
    if not sent_any:
        print("No message sent. Use --samplerate or --command to send OSC messages.")
        print("Run with --help for usage information.")
        return 0
    
    print()
    return 0 if success else 1


if __name__ == '__main__':
    sys.exit(main())
