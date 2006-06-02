#
# Copyright (C) 2006 SIPfoundry Inc.
# Licensed by SIPfoundry under the LGPL license.
# 
# Copyright (C) 2006 Pingtel Corp.
# Licensed to SIPfoundry under a Contributor Agreement.
#
##############################################################################

# set up the load path
$PROCESS_MANAGER_TEST_DIR = File.dirname(__FILE__)
$:.unshift File.join($PROCESS_MANAGER_TEST_DIR, "..", "lib")

# system requires
require 'test/unit'
require 'net/http'
require 'soap/rpc/driver'

# application requires
require 'process_manager'
require 'process_manager_server'


class ProcessManagerServerTest < Test::Unit::TestCase

  TEST_PORT = 2000

  def start_server
    # not starting each time, port doesn't get freed in time
    return if defined? @@server

    puts "Starting"
    httpd = ProcessManagerServer.new(ProcessManager.new(:ProcessConfigDir => '/tmp'), :Port => TEST_PORT)

    trap(:INT) do
      httpd.shutdown
    end

    @@server = Thread.new do
      httpd.start
    end
    puts "Started"
  end
  
  def setup
    super
    start_server
    @pm = SOAP::RPC::Driver.new("http://localhost:#{TEST_PORT}",
                                 ProcessManagerServer::SOAP_NAMESPACE)
    @pm.wiredump_dev = STDERR if $DEBUG
    @pm.add_method('manageProcesses', 'input')
  end
  
  def teardown
  end

  def test_manageProcesses
    input = ProcessManagerServer::ManageProcessesInput.new
    input.processes = ProcessManagerServer::Array['p1', 'p2']
    input.verb = 'start'
    @pm.manageProcesses(input)
  end

end
