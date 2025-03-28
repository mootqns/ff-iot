#!/bin/bash

# Update package lists
apt update

# Install MySQL Server
apt install mysql-server

# Start and enable MySQL service
systemctl start mysql
systemctl enable mysql
systemctl daemon-reload
