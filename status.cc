#include "status.h"

namespace entdb {

std::string Status::ToString() const {
  if (message1_ == "") {
    return "OK";
  } else {
    char tmp[30];
    const char* type;
    switch (code()) {
      case kOK:
        type = "OK";
        break;
      case kNotFound:
        type = "NotFound: ";
        break;
      case kInvalidArgument:
        type = "Invalid argument: ";
        break;
      case kIOError:
        type = "IO error: ";
        break;
      default:
        snprintf(tmp, sizeof(tmp), "Unknown code(%d): ",
                 static_cast<int>(code()));
        type = tmp;
        break;
    }
    std::string result(type);
    result.append(message1_);
    if (message2_.size() > 0) {
      result.append(" - ");
      result.append(message2_);
    }
    return result;
  }
}

};
