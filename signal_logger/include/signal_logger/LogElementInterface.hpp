/*
 * BufferInterface.hpp
 *
 *  Created on: Sep 22, 2016
 *      Author: gabrielhottiger
 */

#pragma once

// Signal logger
#include <signal_logger/BufferInterface.hpp>

namespace signal_logger {

class LogElementInterface
{
 public:
  LogElementInterface():
    type_(typeid(void)) {

  }

  virtual ~LogElementInterface() {

  }

 protected:
  LogElementInterface(const std::type_index& type, const std::string& name = std::string()) :
    type_(type),
    pBuffer_(nullptr),
    name_(name)
  {
  }

 public:
  template<typename ValueType_>
  void push_front(boost::call_traits<ValueType_>::param_type item) {
    if (type_ == typeid(ValueType_)) {
      return std::static_pointer_cast<Buffer<ValueType_> >(pBuffer_)->push_front(item);
    }
    else {
      throw std::runtime_error("Buffer value type mismatch");
    }
  }

  template<typename ValueType_>
  void pop_back(ValueType_* pItem) {
    if (type_ == typeid(ValueType_)) {
      return std::static_pointer_cast<Buffer<ValueType_> >(pBuffer_)->pop_back(pItem);
    }
    else {
      throw std::runtime_error("Buffer value type mismatch");
    }
  }

  virtual void collect() = 0;


 protected:
  std::type_index type_;
  internal::BufferInterfacePtr pBuffer_;
  std::string name_;

};

} /* namespace signal_logger */
