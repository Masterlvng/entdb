#ifndef ENTDB_STATUS_H
#define ENTDB_STATUS_H

#include <string>
#include <stdio.h>

namespace entdb {

class Status {
 public:
  Status() { code_ = kOK; message1_ = ""; }
  ~Status() {}
  Status(int code, std::string message1, std::string message2)
  {
    code_ = code;
    message1_ = message1;
    message2_ = message2;
  }

  static Status OK() { return Status(); }

  static Status NotFound(const std::string& message1, const std::string& message2="") {
    return Status(kNotFound, message1, message2);
  }
  static Status InvalidArgument(const std::string& message1, const std::string& message2="") {
    return Status(kInvalidArgument, message1, message2);
  }
  static Status IOError(const std::string& message1, const std::string& message2="") {
    return Status(kIOError, message1, message2);
  }

  bool IsOK() const { return (code() == kOK); }
  bool IsNotFound() const { return code() == kNotFound; }
  bool IsInvalidArgument() const { return code() == kInvalidArgument; }
  bool IsIOError() const { return code() == kIOError; }

  std::string ToString() const;


 private:
  int code_;
  std::string message1_;
  std::string message2_;

  int code() const { return code_; };

  enum Code {
    kOK = 0, 
    kNotFound = 1,
    kInvalidArgument = 2,
    kIOError = 3
  };
};

}; 

#endif 
