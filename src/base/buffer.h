/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2016 The OpenSAF Foundation
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. This file and program are licensed
 * under the GNU Lesser General Public License Version 2.1, February 1999.
 * The complete license can be accessed from the following location:
 * http://opensource.org/licenses/lgpl-license.php
 * See the Copying file included with the OpenSAF distribution for full
 * licensing terms.
 *
 * Author(s): Ericsson AB
 *
 */

#ifndef BASE_BUFFER_H_
#define BASE_BUFFER_H_

#include <cstddef>
#include <cstdint>
#include <cstring>
#include "base/macros.h"

namespace base {

// An output buffer that can be used to build e.g. a message to be sent over a
// network.
class Buffer {
 public:
  // Allocate a buffer with enough space to store @a capacity bytes of data.
  explicit Buffer(size_t capacity) :
      buffer_{new char[capacity]},
      capacity_{capacity},
      size_{0} {
  }
  ~Buffer() {
    delete [] buffer_;
  }
  // Reset the write position to the start of the buffer.
  void clear() { size_ = 0; }
  // Returns true if the buffer is empty.
  bool empty() const { return size_ == 0; }
  // Returns a pointer to the start of the buffer.
  const char* data() const { return buffer_; }
  // Returns a pointer to the end of the buffer where new data can be appended.
  char* end() const { return buffer_ + size_; }
  // Returns the number of bytes that have been written to the buffer.
  size_t size() const { return size_; }
  // Set new size of the buffer (e.g. after manually adding data at the end).
  void set_size(size_t s) { size_ = s; }
  // Returns the maximum number of bytes that can be stored in this buffer.
  size_t capacity() const { return capacity_; }
  // Append a single character to the end of the buffer.
  void AppendChar(char c) {
    if (size_ != capacity_) buffer_[size_++] = c;
  }
  // This function is similar to AppendNumber(), except that leading zeros will
  // be printed - i.e. this method implements a fixed field width.
  void AppendFixedWidthNumber(uint32_t number, uint32_t power) {
    for (;;) {
      uint32_t digit = number / power;
      AppendChar(digit + '0');
      if (power == 1) break;
      number -= digit * power;
      power /= 10;
    }
  }
  // Append decimal @a number to the end of the buffer. The @a power parameter
  // must be a power of ten, pointing to the highest possible digit in the
  // decimal number. E.g. if @a power is 1000, then the @a number must not be
  // greater than 9999.
  void AppendNumber(uint32_t number, uint32_t power) {
    while (power != 1 && power > number) power /= 10;
    AppendFixedWidthNumber(number, power);
  }
  // Append a string of @a size characters to the end of the buffer.
  void AppendString(const char* str, size_t size) {
    size_t bytes_to_copy = capacity_ - size_;
    if (size < bytes_to_copy) bytes_to_copy = size;
    memcpy(buffer_ + size_, str, bytes_to_copy);
    size_ += bytes_to_copy;
  }
  // Append a NUL-terminated string to the end of the buffer.
  void AppendString(const char* str) {
    AppendString(str, strlen(str));
  }

 private:
  char* buffer_;
  size_t capacity_;
  size_t size_;

  DELETE_COPY_AND_MOVE_OPERATORS(Buffer);
};

}  // namespace base

#endif  // BASE_BUFFER_H_
