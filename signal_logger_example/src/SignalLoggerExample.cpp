/*
 * SignalLoggerExample.cpp
 *
 *  Created on: Feb 29, 2016
 *      Author: Gabriel Hottiger
 */

#include <signal_logger_example/SignalLoggerExample.hpp>

namespace signal_logger_example {

SignalLoggerExample::SignalLoggerExample(NodeHandlePtr nh):
  any_node::Node(nh),
  publishThread_(),
  shouldPublish_(true),
  shouldRead_(true),
  logVar_(0.0),
  time_(ros::TIME_MIN)
{
  ros::Time::setNow(time_);
}

void SignalLoggerExample::init()
{
  // Initialize logger
  signal_logger::setSignalLoggerRos(&getNodeHandle());
  signal_logger::logger->initLogger(signal_logger::SignalLoggerOptions(10, 2, "loggerScript.yaml", "/log"));

  // Add logger vars and update logger
  signal_logger::add(logVar_, "logVar1", "ns", "[m]", 1, signal_logger::LogElementAction::SAVE,
   100, signal_logger::BufferType::LOOPING);
  signal_logger::add(logVar_, "logVar2", "ns", "[m]", 2, signal_logger::LogElementAction::SAVE_AND_PUBLISH,
   200, signal_logger::BufferType::FIXED_SIZE);
  signal_logger::add(logVar_, "logVar3", "ns", "[m]", 1, signal_logger::LogElementAction::PUBLISH,
   100, signal_logger::BufferType::LOOPING);
  signal_logger::add(logVar_, "logVar4", "ns", "[m]", 3, signal_logger::LogElementAction::SAVE_AND_PUBLISH ,
   10, signal_logger::BufferType::EXPONENTIALLY_GROWING);
  signal_logger::add(logVar_, "logVar5", "ns", "[m]", 5, signal_logger::LogElementAction::SAVE ,
   1000, signal_logger::BufferType::LOOPING);

  // Update and save script
  signal_logger::logger->updateLogger();
  signal_logger::logger->saveLoggerScript();

  // Spawn a publishing thread
  publishThread_ = std::thread(&SignalLoggerExample::publishWorker, this);
  readThread_ = std::thread(&SignalLoggerExample::readWorker, this);

//  signal_logger::logger->setNamespaceBufferSize("ns", 5);
//  signal_logger::logger->disableElement("/log/ns/logVar1");
  signal_logger::logger->disableNamespace("ns");

  signal_logger::logger->setMaxLoggingTime(10);

  // Start logger
  signal_logger::logger->startLogger();
}

void SignalLoggerExample::cleanup()
{
  // Stop and save logger data
  signal_logger::logger->saveLoggerData( { signal_logger::LogFileType::CSV, signal_logger::LogFileType::BAG, signal_logger::LogFileType::BINARY } );
  signal_logger::logger->stopLogger();

  // Let logger save data
  sleep(3.0);

  // Joint publisher thread
  shouldPublish_.store(false);
  shouldRead_.store(false);
  publishThread_.join();
  readThread_.join();

  // Cleanup
  signal_logger::logger->cleanup();
}

bool SignalLoggerExample::update(const any_worker::WorkerEvent& event) {

  // Collect data
  signal_logger::logger->collectLoggerData();
  //ROS_INFO_STREAM("Time " << ros::Time::now() << " logVar " << logVar_);

  // Update logger vars and time
  logVar_ += 0.1;
  time_ += ros::Duration(1.0/10);
  ros::Time::setNow(time_);

  static unsigned int counter = 0;
  counter++;

  if(counter ==  15) {
//    signal_logger::logger->enableNamespace("ns");
    signal_logger::logger->enableElement("/log/ns/logVar1");
    signal_logger::logger->setElementDivider("/log/ns/logVar1", 6);
  } else if(counter == 25) {
//    signal_logger::logger->disableElement("/log/ns/logVar2");
  }

  return true;
}

void SignalLoggerExample::publishWorker() {
  while(shouldPublish_) {
    signal_logger::logger->publishData();
    usleep(100);
  }
}

void SignalLoggerExample::readWorker() {
  while(shouldRead_) {
    signal_logger::vector_type<double> v = signal_logger::logger->readNewValues<double>("/log/ns/logVar1");
    std::cout << "Got new values: " << std::endl;
    for(unsigned int i = 0; i < v.size(); ++i) {
      std::cout << v[i] << std::endl;
    }
    sleep(1);
  }
}

} /* namespace signal_logger_example */
