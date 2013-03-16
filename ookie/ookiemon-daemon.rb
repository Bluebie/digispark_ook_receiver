# This script works like a linux system daemon. You can run:
#   sudo ruby ookiemon-daemon.rb start
# to start it up in the background on a linux computer like
# a raspberry pi - my setup at home!             <3 Bluebie
require 'rubygems'
require 'daemons'

Daemons.run("#{File.dirname(__FILE__)}/ookiemon.rb")
