/*!
 * @file     SignalLoggerBase.cpp
 * @author   Gabriel Hottiger, Christian Gehring
 * @date     Sep 28, 2016
 * @brief    Base class for signal loggers.
 */

// signal logger
#include "signal_logger_core/SignalLoggerBase.hpp"

// yaml
#include <yaml-cpp/yaml.h>
#include "signal_logger_core/yaml_helper.hpp"

// boost
#include <boost/iterator/counting_iterator.hpp>

// stl
#include "assert.h"
#include <thread>
#include <fstream>
#include <chrono>
#include <ctime>

// system
#include <sys/stat.h>

namespace signal_logger {

SignalLoggerBase::SignalLoggerBase():
                  options_(),
                  noCollectDataCalls_(0u),
                  noCollectDataCallsCopy_(0u),
                  logElements_(),
                  enabledElements_(),
                  logElementsToAdd_(),
                  logTime_(),
                  timeElement_(),
                  isInitialized_(false),
                  isCollectingData_(false),
                  isSavingData_(false),
                  isCopyingBuffer_(false),
                  isStarting_(false),
                  shouldPublish_(false),
                  loggerMutex_(),
                  saveLoggerDataMutex_()
{
}

SignalLoggerBase::~SignalLoggerBase()
{

}

void SignalLoggerBase::initLogger(const SignalLoggerOptions& options)
{
  // Lock the logger (blocking!)
  boost::unique_lock<boost::shared_mutex> initLoggerLock(loggerMutex_);

  // Assert corrupted configuration
  assert(options.updateFrequency_ > 0);

  // Set configuration
  options_ = options;

  // Init time element
  initTimeLogElement();

  // Notify user
  MELO_INFO("[SignalLogger::initLogger] Initialized!");
  isInitialized_ = true;
}

bool SignalLoggerBase::startLogger()
{
  // Delayed logger start if saving prevents lock of mutex
  boost::unique_lock<boost::shared_mutex> tryStartLoggerLock(loggerMutex_, boost::try_to_lock);
  if(!tryStartLoggerLock && isCopyingBuffer_) {
    // Still copying the data from the buffer, Wait in other thread until logger can be started.
    std::thread t1(&SignalLoggerBase::workerStartLogger, this);
    t1.detach();
    return true;
  }

  // Lock the logger if not locked yet (blocking!)
  boost::unique_lock<boost::shared_mutex> startLoggerLock(loggerMutex_, boost::defer_lock);
  if(!tryStartLoggerLock) { startLoggerLock.lock(); }

  // Warn the user on invalid request
  if(!isInitialized_ || isStarting_) {
    MELO_WARN("[SignalLogger::startLogger] Could not start!%s%s", !isInitialized_?" Not initialized!":"",
              isStarting_?" Delayed logger start was already requested!":"");
    return false;
  } else if( isCollectingData_ ) {
    MELO_DEBUG("[SignalLogger::startLogger] Already started!");
    return true;
  }

  // #TODO update elements here?

  // Decide on the time buffer to use ( Init with exponentially growing when max log time is zero, fixed size buffer otherwise)
  const double maxLogTime = (options_.maxLoggingTime_ == 0.0) ? LOGGER_EXP_GROWING_MAXIMUM_LOG_TIME : options_.maxLoggingTime_;
  size_t timeBufferSize =  static_cast<size_t >(maxLogTime * options_.updateFrequency_);
  BufferType timeBufferType =  (options_.maxLoggingTime_ == 0.0) ? BufferType::EXPONENTIALLY_GROWING : BufferType::FIXED_SIZE;

  if(enabledElements_.empty()) {
    // If no elements are logged, log with a looping buffer TODO: keep max log time?
    timeBufferType = BufferType::LOOPING;
  } else {
    // If all elements are looping or all elements are fixed size use a time buffer of the according size/type
    for(auto bufferType : { BufferType::FIXED_SIZE, BufferType::LOOPING } ) {
      auto hasOtherElement = [bufferType] (const LogElementMapIterator& s) { return s->second->getBuffer().getBufferType() != bufferType; };
      auto otherElement = std::find_if(enabledElements_.begin(), enabledElements_.end(), hasOtherElement);
      if( otherElement == enabledElements_.end() ) {
        auto maxElement = std::max_element(enabledElements_.begin(), enabledElements_.end(), maxScaledBufferSize());
        timeBufferSize = ( maxElement != enabledElements_.end() ) ? ( (*maxElement)->second->getOptions().getDivider() *
            (*maxElement)->second->getBuffer().getBufferSize() ) : 0;
        timeBufferType = bufferType;
        MELO_INFO_STREAM("[Signal logger] Use " << ( (bufferType == BufferType::FIXED_SIZE)? "fixed size" : "looping" )
                                                << " Time Buffer of size:" << timeBufferSize);
      }
    }
  }


  // Set time buffer properties to the element
  timeElement_->getBuffer().setBufferType(timeBufferType);
  timeElement_->getBuffer().setBufferSize(timeBufferSize);

  // Reset elements ( Clear buffers )
  timeElement_->reset();
  for(auto & elem : enabledElements_) { elem->second->reset(); }

  // Reset flags and data collection calls
  isCollectingData_ = true;
  shouldPublish_ = true;
  noCollectDataCalls_ = 0;

  return true;
}

bool SignalLoggerBase::stopLogger()
{
  // If publishData is running notify stop
  shouldPublish_ = false;

  // If lock can not be acquired because of saving ignore the call
  boost::unique_lock<boost::shared_mutex> tryStopLoggerLock(loggerMutex_, boost::try_to_lock);
  if(!tryStopLoggerLock && isSavingData_) {
    MELO_WARN("Saving data while trying to stop logger. Do nothing!");
    return false;
  }

  // Lock the logger if not locked yet (blocking!)
  boost::unique_lock<boost::shared_mutex> stopLoggerLock(loggerMutex_, boost::defer_lock);
  if(!tryStopLoggerLock) { stopLoggerLock.lock(); }

  // Those are cases where the logger is already stopped. So "stopping" was successful.
  if(!isInitialized_) {
    MELO_DEBUG("[SignalLogger::stopLogger] Could not stop non-initialized logger!");
  } else if(!isCollectingData_) {
    MELO_DEBUG("[SignalLogger::stopLogger] Logger was already stopped!");
  }

  // Stop the collection of data!
  isCollectingData_ = false;

  return true;
}

bool SignalLoggerBase::restartLogger()
{
  // Mutex is locked internally
  bool stopped = stopLogger();
  return startLogger() && stopped;
}


// #TODO update logger via script, that basically allows every script to be loaded
// #TODO update logger to add variables



bool SignalLoggerBase::updateLogger() {


  // If lock can not be acquired because of saving ignore the call
  boost::unique_lock<boost::shared_mutex> tryUpdateLoggerLock(loggerMutex_, boost::try_to_lock);
  if(!tryUpdateLoggerLock && isSavingData_) {
    MELO_WARN("Saving data while trying to update logger. Do nothing!");
    return false;
  }

  // Lock the logger if not locked yet (blocking!)
  boost::unique_lock<boost::shared_mutex> updateLoggerLock(loggerMutex_, boost::defer_lock);
  if(!tryUpdateLoggerLock) { updateLoggerLock.lock(); }

  // Check if update logger call is valid
  if(!isInitialized_ || isCollectingData_ || isSavingData_)
  {
    MELO_WARN("[Signal logger] Could not update!%s%s%s", !isInitialized_?" Not initialized!":"",
        isSavingData_?" Saving data!":"", isCollectingData_?" Collecting data!":"");
    return false;
  }

  // Disable all log data and reallocate buffer
  for(auto & elem : enabledElements_) { elem->second->setIsEnabled(false); }
  enabledElements_.clear();

  // Add elements to list
  for(auto & elemToAdd : logElementsToAdd_ ) {
    // Transfer ownership to logElements
    logElements_[elemToAdd.first] = std::unique_ptr<LogElementInterface>(std::move(elemToAdd.second));
  }
  // Clear elements
  logElementsToAdd_.clear();

  // Read the script
  if( !readDataCollectScript(options_.collectScriptFileName_) ) {
    MELO_ERROR("[Signal logger] Could not load logger script!");
    return false;
  }

  return true;

}

bool SignalLoggerBase::saveLoggerScript() {
  // If lock can not be acquired because of saving ignore the call
  boost::unique_lock<boost::shared_mutex> trySaveLoggerScriptLock(loggerMutex_, boost::try_to_lock);
  if(!trySaveLoggerScriptLock && isSavingData_) {
    MELO_WARN("Saving data while trying to save logger script. Do nothing!");
    return false;
  }

  // Lock the logger if not locked yet (blocking!)
  boost::unique_lock<boost::shared_mutex> saveLoggerScriptLock(loggerMutex_, boost::defer_lock);
  if(!trySaveLoggerScriptLock) { saveLoggerScriptLock.lock(); }

  if( !saveDataCollectScript( std::string(LOGGER_DEFAULT_SCRIPT_FILENAME) ) ){
    MELO_ERROR("[Signal logger] Could not save logger script!");
    return false;
  }
  return true;
}

bool SignalLoggerBase::collectLoggerData()
{
  // Try lock logger for read (non blocking!)
  boost::shared_lock<boost::shared_mutex> collectLoggerDataLock(loggerMutex_, boost::try_to_lock);

  if(collectLoggerDataLock)
  {
    //! Check if logger is initialized
    if(!isInitialized_) { return false; }

    //! If buffer is copied return -> don't collect
    if(isCopyingBuffer_) { return true; }

    // Is logger started?
    if(isCollectingData_)
    {
      // Set current time (thread safe, since only used in init, start which have unique locks on the logger)
      logTime_ = this->getCurrentTime();

      // Check if time buffer is full when using a fixed size buffer
      if(timeElement_->getBuffer().getBufferType() == BufferType::FIXED_SIZE && timeElement_->getBuffer().noTotalItems() == timeElement_->getBuffer().getBufferSize())
      {
        MELO_WARN("[Signal Logger] Stopped. Time buffer is full!");
        // Free lock and stop logger
        collectLoggerDataLock.unlock();
        this->stopLogger();
        return true;
      }

      // Lock all mutexes for time synchronization
      for(auto & elem : enabledElements_)
      {
        elem->second->acquireMutex().lock();
      }

      // Update time
      {
        std::unique_lock<std::mutex> uniqueTimeElementLock(timeElement_->acquireMutex());
        timeElement_->collectData();
      }

      // Collect element into buffer and unlock element mutexes (time/value pair is valid now)
      for(auto & elem : enabledElements_)
      {
        if((noCollectDataCalls_ % elem->second->getOptions().getDivider()) == 0) {
          elem->second->collectData();
        }
        elem->second->acquireMutex().unlock();
      }

      // Iterate
      ++noCollectDataCalls_;
    }
  }
  else {
    MELO_DEBUG("[Signal Logger] Dropping collect data call!");
  }

  return true;
}

bool SignalLoggerBase::publishData()
{
  // Check if publishing is desired
  if(!shouldPublish_) {
    return true;
  }

  // Try lock logger for read (non blocking!)
  boost::shared_lock<boost::shared_mutex> publishDataLock(loggerMutex_, boost::try_to_lock);

  if(publishDataLock)
  {
    // Publish data from buffer
    for(auto & elem : enabledElements_)
    {
      // Publishing blocks the mutex for quite a long time, allow interference by stopLogger using atomic bool
      if(shouldPublish_) {
        if(elem->second->getOptions().isPublished()) {
          elem->second->publishData(*timeElement_, noCollectDataCalls_);
        }
      }
      else {
        return true;
      }
    }
  }

  return true;
}

bool SignalLoggerBase::saveLoggerData(const LogFileTypeSet & logfileTypes)
{
  // Make sure start stop are not called in the meantime
  boost::shared_lock<boost::shared_mutex> saveLoggerDataLock(loggerMutex_);

  // Allow only single call to save logger data
  std::unique_lock<std::mutex> lockSaveData(saveLoggerDataMutex_, std::try_to_lock);

  if(lockSaveData) {
    if(!isInitialized_ || isSavingData_)
    {
      MELO_WARN("[Signal logger] Could not save data! %s%s", isSavingData_?" Already saving data!":"",
          !isInitialized_?" Not initialized!":"");
      return false;
    }

    // set save flag
    isCopyingBuffer_ = true;
    isSavingData_ = true;

    // Save data in different thread
    std::thread t1(&SignalLoggerBase::workerSaveDataWrapper, this, logfileTypes);
    t1.detach();
  }
  else {
    MELO_WARN("[Signal logger] Already saving to file!");
  }

  return true;
}

bool SignalLoggerBase::stopAndSaveLoggerData(const LogFileTypeSet & logfileTypes)
{
  // Mutex is locked internally
  bool stopped = stopLogger();
  return saveLoggerData(logfileTypes) && stopped;
}

bool SignalLoggerBase::cleanup()
{
  // Lock the logger (blocking!)
  boost::unique_lock<boost::shared_mutex> lockLogger(loggerMutex_);

  // Publish data from buffer
  for(auto & elem : logElements_) { elem.second->cleanup(); }
  for(auto & elem : logElementsToAdd_) { elem.second->cleanup(); }

  // Clear maps
  enabledElements_.clear();
  logElements_.clear();
  logElementsToAdd_.clear();

  return true;
}

void SignalLoggerBase::setMaxLoggingTime(double maxLoggingTime) {
  // The max logging time is only needed in startLogger, no issues it setting it anytime
  boost::shared_lock<boost::shared_mutex> lockLogger(loggerMutex_);
  options_.maxLoggingTime_ = std::max(0.0, maxLoggingTime);
}

bool SignalLoggerBase::hasElement(const std::string & name)
{
  boost::shared_lock<boost::shared_mutex> lockLogger(loggerMutex_);
  return logElements_.find(name) != logElements_.end();
}

const LogElementInterface & SignalLoggerBase::getElement(const std::string & name)
{
  boost::shared_lock<boost::shared_mutex> lockLogger(loggerMutex_);
  if(!hasElement(name)) {
    throw std::out_of_range("[SignalLoggerBase]::getElement(): Element " + name + " was not added to the logger!");
  }
  return *logElements_[name];
}

bool SignalLoggerBase::enableElement(const std::string & name)
{
  boost::upgrade_lock<boost::shared_mutex> lockLogger(loggerMutex_);
  if( !hasElement(name) ) {
    MELO_WARN_STREAM("[SignalLogger::enableElement] Can not enable non-existing element with name " << name << "!");
    return false;
  }
  if( logElements_[name]->isEnabled() ) { return true; }

  if( isCollectingData_ && logElements_[name]->getBuffer().getBufferType() != BufferType::LOOPING ) {
    MELO_WARN_STREAM("[SignalLogger::enableElement]: Can not enable element " << name << " with non-looping buffer type when logger is running!");
    return false;
  }

  if(isCollectingData_ && ( timeElement_->getBuffer().getBufferType() != BufferType::LOOPING ||
      (logElements_[name]->getOptions().getDivider() * logElements_[name]->getBuffer() .getBufferSize()) > timeElement_->getBuffer().getBufferSize() ) ) {
    MELO_WARN_STREAM("[SignalLogger::enableElement]: Can not enable element " << name << " when logger is running and time buffer is too small! "
      << "(Des: " << logElements_[name]->getOptions().getDivider() * logElements_[name]->getBuffer() .getBufferSize() << " / Time: " << timeElement_->getBuffer().getBufferSize() <<")");
    return false;
  }

  boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLockLogger(lockLogger);
  logElements_[name]->setIsEnabled(true);
  enabledElements_.push_back(logElements_.find(name));

  return true;
}

bool SignalLoggerBase::disableElement(const std::string & name)
{
  auto isEnabled = [](const std::unique_ptr<LogElementInterface> & e) { return e->isEnabled(); };
  auto setIsEnabled = [this, name](const std::unique_ptr<LogElementInterface> & e, const bool p) {
    e->setIsEnabled(p);
    auto it =  std::find(enabledElements_.begin(), enabledElements_.end(), logElements_.find(name));
    if( it != enabledElements_.end() ) { enabledElements_.erase(it); }
  };
  return setElementProperty<bool>(name, false, "enabled(false)", isEnabled, setIsEnabled);
}

bool SignalLoggerBase::setElementBufferSize(const std::string & name, const std::size_t size)
{
  auto getBufferSize = [](const std::unique_ptr<LogElementInterface> & e) { return e->getBuffer().getBufferSize(); };
  auto setBufferSize = [](const std::unique_ptr<LogElementInterface> & e, const std::size_t p) { e->getBuffer().setBufferSize(p); };
  return setElementProperty<std::size_t>(name, size, "buffer size", getBufferSize, setBufferSize);
}

bool SignalLoggerBase::setElementBufferType(const std::string & name, const BufferType type)
{
  auto getBufferType = [](const std::unique_ptr<LogElementInterface> & e) { return e->getBuffer().getBufferType(); };
  auto setBufferType = [](const std::unique_ptr<LogElementInterface> & e, const BufferType p) { e->getBuffer().setBufferType(p); };
  return setElementProperty<BufferType>(name, type, "buffer type", getBufferType, setBufferType);
}

bool SignalLoggerBase::setElementDivider(const std::string & name, const std::size_t divider)
{
  auto getDivider = [](const std::unique_ptr<LogElementInterface> & e) { return e->getOptions().getDivider(); };
  auto setDivider = [](const std::unique_ptr<LogElementInterface> & e, const std::size_t p) { e->getOptions().setDivider(p); };
  return setElementProperty<std::size_t>(name, divider, "divider", getDivider, setDivider);
}

bool SignalLoggerBase::setElementAction(const std::string & name, const LogElementAction action)
{
  auto getAction = [](const std::unique_ptr<LogElementInterface> & e) { return e->getOptions().getAction(); };
  auto setAction = [](const std::unique_ptr<LogElementInterface> & e, const LogElementAction p) { e->getOptions().setAction(p); };
  return setElementProperty<LogElementAction>(name, action, "action", getAction, setAction);
}

bool SignalLoggerBase::enableNamespace(const std::string & ns)
{
  auto f = [this](const std::string & name) { return enableElement(name); };
  return setElementPropertyForNamespace(ns, f);
}

bool SignalLoggerBase::disableNamespace(const std::string & ns)
{
  auto f = [this](const std::string & name) { return disableElement(name); };
  return setElementPropertyForNamespace(ns, f);
}

bool SignalLoggerBase::setNamespaceBufferSize(const std::string & ns, const std::size_t size)
{
  auto f = [size, this](const std::string & name) { return setElementBufferSize(name, size); };
  return setElementPropertyForNamespace(ns, f);
}

bool SignalLoggerBase::setNamespaceBufferType(const std::string & ns, const BufferType type)
{
  auto f = [type, this](const std::string & name) { return setElementBufferType(name, type); };
  return setElementPropertyForNamespace(ns, f);
}

bool SignalLoggerBase::setNamespaceDivider(const std::string & ns, const std::size_t divider)
{
  auto f = [divider, this](const std::string & name) { return setElementDivider(name, divider); };
  return setElementPropertyForNamespace(ns, f);
}

bool SignalLoggerBase::setNamespaceAction(const std::string & ns, const LogElementAction action)
{
  auto f = [action, this](const std::string & name) { return setElementAction(name, action); };
  return setElementPropertyForNamespace(ns, f);
}

bool SignalLoggerBase::readDataCollectScript(const std::string & scriptName)
{
  if(!isInitialized_ || isSavingData_)
  {
    MELO_WARN("[Signal logger] Could not read data collect script! %s%s", isCollectingData_?" Collecting data!":"",
        !isInitialized_?" Not initialized!":"");
    return false;
  }

  // Check filename size and ending
  std::string ending = ".yaml";
  if ( ( (ending.size() + 1) > scriptName.size() ) || !std::equal(ending.rbegin(), ending.rend(), scriptName.rbegin()) ) {
    MELO_ERROR_STREAM("[Signal logger] Script must be a yaml file : *.yaml");
    return false;
  }

  // Check file existance
  struct stat buffer;
  if(stat(scriptName.c_str(), &buffer) != 0) {
    std::fstream fs;
    fs.open(scriptName, std::fstream::out);
    fs.close();
  }

  // Get set of numbers from 0...(size-1)
  std::set<unsigned int> iteratorOffsets(boost::counting_iterator<unsigned int>(0),
                                         boost::counting_iterator<unsigned int>(logElements_.size()));

  // Save loading of yaml config file
  try {
    YAML::Node config = YAML::LoadFile(scriptName);
    if (config["log_elements"].IsSequence())
    {
      YAML::Node logElementsNode = config["log_elements"];
      for( size_t i = 0; i < logElementsNode.size(); ++i)
      {
        if (YAML::Node parameter = logElementsNode[i]["name"])
        {
          std::string name = logElementsNode[i]["name"].as<std::string>();
          auto elem = logElements_.find(name);
          if(elem != logElements_.end())
          {
            // Erase the iterator offset that belongs to elem since it was found in the yaml
            iteratorOffsets.erase( std::distance(logElements_.begin(), elem) );

            // Overwrite defaults if specified in yaml file
            if (YAML::Node parameter = logElementsNode[i]["enabled"])
            {
              elem->second->setIsEnabled(parameter.as<bool>());
            }
            else {
              // Disable element if nothing is specified
              elem->second->setIsEnabled(false);
            }
            // Overwrite defaults if specified in yaml file
            if (YAML::Node parameter = logElementsNode[i]["divider"])
            {
              elem->second->getOptions().setDivider(parameter.as<int>());
            }
            // Check for action
            if (YAML::Node parameter = logElementsNode[i]["action"])
            {
              elem->second->getOptions().setAction( static_cast<LogElementAction>(parameter.as<int>()) );
            }
            // Check for buffer size
            if (YAML::Node parameter = logElementsNode[i]["buffer"]["size"])
            {
              elem->second->getBuffer().setBufferSize(parameter.as<int>());
            }
            // Check for buffer looping
            if (YAML::Node parameter = logElementsNode[i]["buffer"]["type"])
            {
              elem->second->getBuffer().setBufferType( static_cast<BufferType>(parameter.as<int>()) );
            }
            // Insert element
            if(elem->second->isEnabled()) {
              enabledElements_.push_back(elem);
            }
          }
          else {
            MELO_DEBUG_STREAM("[Signal logger] Could not load " << name << " from config file. Var not logged.");
          }
        }
        else {
          MELO_DEBUG_STREAM("[Signal logger] Could not load get name from config file. Ignore entry nr: " << i << "!");
        }
      }
    }
    else {
      MELO_DEBUG_STREAM("[Signal logger] Parameter file is ill-formatted. Log elements is no sequence.");
    }
  }
  catch(YAML::Exception & e) {
    MELO_ERROR_STREAM("[Signal logger] Could not load config file, because exception occurred: "<< e.what());
    return false;
  }

  // Noisily add all elements that were not in the configuration file
  for(auto iteratorOffset : iteratorOffsets) {
    LogElementMapIterator elem = std::next(logElements_.begin(), iteratorOffset);
    elem->second->setIsEnabled(true);
    enabledElements_.push_back(elem);
    MELO_DEBUG("[Signal logger] Enable logger element %s. It was not specified in the logger file!", elem->first.c_str() );
  }

  return true;
}

bool SignalLoggerBase::saveDataCollectScript(const std::string & scriptName)
{
  if(!isInitialized_ || isSavingData_)
  {
    MELO_WARN("[Signal logger] Could not save data collect script! %s%s", isCollectingData_?" Collecting data!":"",
        !isInitialized_?" Not initialized!":"");
    return false;
  }

  // Check filename size and ending
  std::string ending = ".yaml";
  if ( ( (ending.size() + 1) > scriptName.size() ) || !std::equal(ending.rbegin(), ending.rend(), scriptName.rbegin()) ) {
    MELO_ERROR_STREAM("[Signal logger] Script must be a yaml file : *.yaml");
    return false;
  }

  // Push back all data to node
  YAML::Node node;
  std::size_t j = 0;

  // Update logger owns a unique lock for the log element map -> thread-safe
  for (auto & elem : logElements_)
  {
    node["log_elements"][j]["name"] = elem.second->getOptions().getName();
    node["log_elements"][j]["enabled"] = elem.second->isEnabled();
    node["log_elements"][j]["divider"] = elem.second->getOptions().getDivider();
    node["log_elements"][j]["action"] = static_cast<int>(elem.second->getOptions().getAction());
    node["log_elements"][j]["buffer"]["size"] = elem.second->getBuffer().getBufferSize();
    node["log_elements"][j]["buffer"]["type"] = static_cast<int>(elem.second->getBuffer().getBufferType());
    j++;
  }

  // If there are logged elements save them to file
  if(j != 0) {
    std::ofstream outfile(scriptName);
    yaml_helper::writeYamlOrderedMaps(outfile, node);
    outfile.close();
  }

  return (j != 0);
}

void SignalLoggerBase::initTimeLogElement() {
  // Reset time element
  LogElementOptions options(options_.loggerPrefix_ + std::string{"/time"}, "[s/ns]", 1, LogElementAction::SAVE);
  timeElement_.reset( new LogElementBase<TimestampPair>( &logTime_, BufferType::FIXED_SIZE, 0, std::move(options) ) );
}

signal_logger::TimestampPair SignalLoggerBase::getCurrentTime() {
  signal_logger::TimestampPair timeStamp;

  // Get time in seconds and nanoseconds
  auto duration = std::chrono::system_clock::now().time_since_epoch();
  auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);
  timeStamp.first = seconds.count();
  timeStamp.second = std::chrono::duration_cast<std::chrono::nanoseconds>(duration-seconds).count();

  return timeStamp;
}

bool SignalLoggerBase::workerSaveDataWrapper(const LogFileTypeSet & logfileTypes) {
  //-- Produce file name, this can be done in series to every other process

  // Read suffix number from file
  int suffixNumber = 0;
  std::ifstream ifs(".last_data", std::ifstream::in);
  if(ifs.is_open()) { ifs >> suffixNumber; }
  ifs.close();
  ++suffixNumber;

  // Write next suffix number to file
  std::ofstream ofs(".last_data", std::ofstream::out | std::ofstream::trunc);
  if(ofs.is_open()) { ofs << suffixNumber; }
  ofs.close();

  // To string with format 00000 (e.g 00223)
  std::string suffixString = std::to_string(suffixNumber);
  while(suffixString.length() != 5 ) { suffixString.insert(0, "0"); }

  // Get local time
  char dateTime[21];
  std::time_t now = std::chrono::system_clock::to_time_t ( std::chrono::system_clock::now() );
  strftime(dateTime, sizeof dateTime, "%Y%m%d_%H-%M-%S_", std::localtime(&now));

  // Filename format (e.g. silo_13Sep2016_12-13-49_00011)
  std::string filename = std::string{"silo_"} + std::string{dateTime} + suffixString;

  {
    // Lock the logger (blocking!)
    boost::unique_lock<boost::shared_mutex> workerSaveDataWrapperLock(loggerMutex_);

    // Copy general stuff
    noCollectDataCallsCopy_ = noCollectDataCalls_.load();

    // Copy data from buffer
    for(auto & elem : enabledElements_)
    {
      if(elem->second->getOptions().isSaved())
      {
        elem->second->copy();
      }
    }

    // Copy time
    timeElement_->copy();

    // Reset buffers and counters
    noCollectDataCalls_ = 0;

    // Clear buffer for log elements
    for(auto & elem : enabledElements_) {
      elem->second->reset();
    }

    // Clear buffer for time elements
    timeElement_->reset();

    // Set flag -> collection can restart
    isCopyingBuffer_ = false;
  }

  // Start saving copy to file
  bool success = this->workerSaveData(filename, logfileTypes);

  // Set flag, notify user
  isSavingData_ = false;

  MELO_INFO_STREAM( "[Signal logger] All done, captain! Stored logging data to file " << filename );

  return success;
}

bool SignalLoggerBase::workerStartLogger() {
  isStarting_ = true;

  while(true) {
    if(!isCopyingBuffer_) {
      isStarting_ = false;
      MELO_INFO("[Signal logger] Delayed logger start!");
      return startLogger();
    }
    // Sleep for one timestep (init is only allowed once. access of updateFrequency is ok)
    usleep( (1.0/options_.updateFrequency_) * 1e6 );
  }
}

}
