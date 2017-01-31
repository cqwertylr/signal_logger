/*!
 * @file	  typedefs.hpp
 * @author	Gabriel Hottiger
 * @date	  Jan 30, 2017
 */

#pragma once

#include <cstddef>

namespace signal_logger {

//! Enum containing possible buffer types
enum class BufferType: unsigned int
{
  FIXED_SIZE = 0,/*!< 0 */
  LOOPING = 1,/*!< 1 */
  EXPONENTIALLY_GROWING = 2/*!< 2 */
};

//! Enum containing possible logging actions
enum class LogElementAction: unsigned int
{
  SAVE_AND_PUBLISH = 0,/*!< 0 */
  SAVE = 1,/*!< 1 */
  PUBLISH = 2/*!< 2 */
};

//! Enum containing possible log file types
enum class LogFileType: unsigned int
{
  BINARY = 0,/*!< 0 */
  BAG = 1, /*!< 1 */
  BINARY_AND_BAG = 2/*!< 2 */
};

//! Default log element group
static constexpr const char* LOG_ELEMENT_DEFAULT_GROUP_NAME   = "/log/";
//! Default log element unit
static constexpr const char* LOG_ELEMENT_DEFAULT_UNIT         = "-";
//! Default log element divider
static constexpr std::size_t LOG_ELEMENT_DEFAULT_DIVIDER      = 1;
//! Default log element action
static constexpr LogElementAction LOG_ELEMENT_DEFAULT_ACTION  = LogElementAction::SAVE_AND_PUBLISH;
//! Default log element buffer size
static constexpr std::size_t LOG_ELEMENT_DEFAULT_BUFFER_SIZE  = 1000;
//! Default log element buffer type
static constexpr BufferType LOG_ELEMENT_DEFAULT_BUFFER_TYPE   = BufferType::LOOPING;

//! Default maximal logging time
static constexpr const double LOGGER_DEFAULT_MAXIMUM_LOG_TIME     = 10.0;
//! Default logging script file name
static constexpr const char*  LOGGER_DEFAULT_SCRIPT_FILENAME      = "logger.yaml";
//! Default logger prefix
static constexpr const char*  LOGGER_DEFAULT_PREFIX               = "/log";
//! Default log time for exponentially growing time buffer
static constexpr const double LOGGER_EXP_GROWING_MAXIMUM_LOG_TIME = 10.0;

//! Helper for subclass add functions
struct both_slashes {
  bool operator()(char a, char b) const {
    return a == '/' && b == '/';
  }
};

}
