#
# Copyright (C) 2006 SIPfoundry Inc.
# Licensed by SIPfoundry under the LGPL license.
# 
# Copyright (C) 2006 Pingtel Corp.
# Licensed to SIPfoundry under a Contributor Agreement.
#
##############################################################################

# system requires
require 'logger'

# application requires
require 'process_config'

=begin
:TODO: Start, stop, report status
:TODO: Don't start a process if it's already running
:TODO: Monitor processes and try to restart them if they go down

:TODO: Honor process config:
:TODO: "manage" flag -- if false then don't do anything with the process
:TODO: "stop" flag -- if false then have to kill the process to stop it
:TODO: "restart" flag -- if false then have to stop-start the process to restart it
:TODO: "max_restarts" setting
:TODO: "report", "max_reports" settings
:TODO: "failure_contact" setting
:TODO: "stdout", "stderr", "stdin" settings

:TODO: Remote service to replicate files
:TODO: Remote service to replicate sipdb data
:LATER: Create the process schema referenced by process config files
=end

# ProcessManager features:
# * Start, monitor, stop, report status on sipX processes
#   * Replaces watchdog, processcgi, WatchDog.xml, ProcessDefinitions.xml, ...
# * Offer network services:
#   * Replicate sipdb databases and arbitrary files (replace replicationcgi)
# * Configured via files that are installed into /etc/sipxpbx/process
class ProcessManager
  
  # If set, then this becomes a prefix to the default sipX directories
  SIPX_PREFIX = 'SIPX_PREFIX'

  # Default directory in which to store PID files for the processes we are managing
  PID_DIR_DEFAULT = '/var/run/sipxpbx'
  
  PID_FILE_EXT = '.pid'

  CONFIG_FILE_PATTERN = Regexp.new('.*\.xml')
  
public

  def initialize(config_dir)
    @config_dir = config_dir
    @config_map = init_process_config
    init_logging
    init_pid_dir
  end

  # Start the specified process.
  # If process_name is not given, then start all configured processes.
  def start(process_name = nil)
    # :NOW:
  end

  # These accessors are used primarily for testing
  attr_accessor :pid_dir
  attr_reader :config_dir, :config_map, :log
  
private
  
  # Each config file in the config dir sets up a sipX process.
  # Read config info from those files and build a process map.
  def init_process_config
    config_map = {}
    get_process_config_files.each do |file|
      config = ProcessConfig.new(File.new(file))
      config_map[config.name] = config
    end
    config_map
  end
  
  # Return an array containing the paths of process config files.
  def get_process_config_files
    config_files = []
    Dir.foreach(@config_dir) do |file|
      config_files << File.join(@config_dir, file) if file =~ CONFIG_FILE_PATTERN
    end
    config_files
  end
  
  def init_logging
    @log = Logger.new(STDOUT)
    @log.level = Logger::DEBUG
  end
  
  # Set @pid_dir to be the directory in which to store PID files for the
  # processes we are managing.
  def init_pid_dir
    @pid_dir = PID_DIR_DEFAULT
      
    # Prepend the prefix dir if $SIPX_PREFIX is defined
    prefix = ENV[SIPX_PREFIX]
    if prefix
      @pid_dir = File.join(prefix, @pid_dir)
    end
  end

  # Start the named process. Raise an exception if no such process is configured.
  def start_process_by_name(process_name)
    config = @config_map[process_name]
    if !config
      raise("Cannot start \"#{process_name}\", no such process is configured")
    end
    start_process(config)
  end

  # Start the named process. Raise an exception if no such process is configured.
  def start_process(process_config)
    # Get info on how to run the process. Assume that the config has been validated already.
    run = process_config.run
    command = run.command
    parameters = run.parameters
    defaultdir = run.defaultdir
    
    # Start the process
    pid = fork do
      log.debug("start_process: command = \"#{command}\", parameters = " +
                "\"#{parameters}\", defaultdir = \"#{defaultdir}\"")
      Dir.chdir(defaultdir) if defaultdir
      exec("#{command} #{parameters}")
    end

    # Remember the process
    pid_file_path = create_process_pid_file(process_config.name, pid)
    log.debug("start_process: PID file = \"#{pid_file_path}\"")
  end

  # Create a PID file for the named process.  Return the path to the file.
  def create_process_pid_file(process_name, pid)
    pid_file_path = File.join(@pid_dir, process_name + PID_FILE_EXT)
    File.open(pid_file_path, 'w') do |file|
      file.puts("#{pid}")
    end
    
    pid_file_path
  end

end






