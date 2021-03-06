/**
 * Copyright 2019 Huawei Technologies Co., Ltd
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef MINDSPORE_INFERENCE_LOG_H_
#define MINDSPORE_INFERENCE_LOG_H_

#include <stdarg.h>
#include <stdint.h>
#include <string>
#include <sstream>
#include <memory>
#include <iostream>

#ifndef ENABLE_ACL
#include "mindspore/core/utils/log_adapter.h"
namespace mindspore::inference {
#define MSI_LOG(level) MS_LOG(level)

#define MSI_LOG_DEBUG MSI_LOG(DEBUG)
#define MSI_LOG_INFO MSI_LOG(INFO)
#define MSI_LOG_WARNING MSI_LOG(WARNING)
#define MSI_LOG_ERROR MSI_LOG(ERROR)

#define MSI_ASSERT(item) MS_ASSERT(item)
}  // namespace mindspore::inference

#else  // ENABLE_ACL
#include "acl/acl.h"
namespace mindspore::inference {

class LogStream {
 public:
  LogStream() { sstream_ = std::make_shared<std::stringstream>(); }
  ~LogStream() = default;

  template <typename T>
  LogStream &operator<<(const T &val) noexcept {
    (*sstream_) << val;
    return *this;
  }

  LogStream &operator<<(std::ostream &func(std::ostream &os)) noexcept {
    (*sstream_) << func;
    return *this;
  }

  friend class LogWriter;

 private:
  std::shared_ptr<std::stringstream> sstream_;
};

template <class T, typename std::enable_if<std::is_enum<T>::value, int>::type = 0>
constexpr std::ostream &operator<<(std::ostream &stream, const T &value) {
  return stream << static_cast<typename std::underlying_type<T>::type>(value);
}

class LogWriter {
 public:
  LogWriter(const char *file, int line, const char *func, aclLogLevel log_level)
      : file_(file), line_(line), func_(func), log_level_(log_level) {}
  ~LogWriter() = default;

  void operator<(const LogStream &stream) const noexcept __attribute__((visibility("default"))) {
    std::ostringstream msg;
    msg << stream.sstream_->rdbuf();
    OutputLog(msg);
  }

 private:
  void OutputLog(const std::ostringstream &msg) const { aclAppLog(log_level_, func_, file_, line_, msg.str().c_str()); }

  const char *file_;
  int line_;
  const char *func_;
  aclLogLevel log_level_;
};

#define MSILOG_IF(level) inference::LogWriter(__FILE__, __LINE__, __FUNCTION__, ACL_##level) < inference::LogStream()

#define MSI_LOG(level) MSI_LOG_##level

#define MSI_LOG_DEBUG MSILOG_IF(DEBUG)
#define MSI_LOG_INFO MSILOG_IF(INFO)
#define MSI_LOG_WARNING MSILOG_IF(WARNING)
#define MSI_LOG_ERROR MSILOG_IF(ERROR)

#define MSI_ASSERT(item)

}  // namespace mindspore::inference

#endif  // ENABLE_ACL

#endif  // MINDSPORE_INFERENCE_LOG_H_
