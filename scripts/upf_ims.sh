#!/bin/bash

# Parameters (you can change these)
IF_NAME="ogstun2"
IPV4_CIDR="10.47.0.1/16"
IPV6_CIDR="2001:db8:cafe::1/64"
MTU="1400"

# Create the interface if it doesn't exist
ip tuntap add $IF_NAME mode tun || {
    echo "Failed to create $IF_NAME (maybe already exists?)"
}

# Assign IP addresses
ip addr add $IPV4_CIDR dev $IF_NAME
ip addr add $IPV6_CIDR dev $IF_NAME

# Set MTU
# Set MTU
ip link set $IF_NAME mtu $MTU

# Enable the interface
ip link set $IF_NAME up

# Enable forwarding
sysctl -w net.ipv4.ip_forward=1
sysctl -w net.ipv6.conf.all.forwarding=1

echo "[✔] Interface $IF_NAME created with:"
echo "     IPv4: $IPV4_CIDR"
echo "     IPv6: $IPV6_CIDR"
echo "     MTU:  $MTU"