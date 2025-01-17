/*  _ _ _   _       _
 * | |_| |_| |_ _ _| |_
 * | | | . |   | | |  _|
 * |_|_|___|_|_|___|_|
 * Hobby graphics and GUI library under the MIT License (MIT)
 *
 * Copyright (c) 2014 Jean-Baptiste Lepesme github.com/jiboo/libhut
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <algorithm>
#include <cstring>
#include <iostream>

#include "hut/buffer.hpp"

using namespace hut;

buffer::buffer(display &_display, uint32_t _size, VkMemoryPropertyFlags _type, VkBufferUsageFlagBits _usage)
    : display_(_display), size_(_size), usage_(_usage) {
  ranges_.insert(range_t{0, _size, false});
  init(_size, _type, _usage);
}

buffer::~buffer() {
  if (memory_ != 0)
    vkFreeMemory(display_.device_, memory_, nullptr);
  if (buffer_ != 0)
    vkDestroyBuffer(display_.device_, buffer_, nullptr);
}


void buffer::init(uint32_t _size, VkMemoryPropertyFlags _type, VkBufferUsageFlagBits _usage) {
  VkBufferCreateInfo bufferInfo = {};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = _size;
  bufferInfo.usage = _usage;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateBuffer(display_.device_, &bufferInfo, nullptr, &buffer_) != VK_SUCCESS) {
    throw std::runtime_error("failed to create vertex buffer!");
  }

  VkMemoryRequirements memRequirements;
  vkGetBufferMemoryRequirements(display_.device_, buffer_, &memRequirements);

  auto type = display_.find_memory_type(memRequirements.memoryTypeBits, _type);

  VkMemoryAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex = type.first;
  type_ = type.second;

  if (vkAllocateMemory(display_.device_, &allocInfo, nullptr, &memory_) != VK_SUCCESS) {
    vkDestroyBuffer(display_.device_, buffer_, nullptr);
    throw std::runtime_error("failed to allocate vertex buffer memory!");
  }
  vkBindBufferMemory(display_.device_, buffer_, memory_, 0);
}

void buffer::update(uint32_t _offset, uint32_t _size, const void *_data) {
  if (type_ & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
    void *target;
    vkMapMemory(display_.device_, memory_, _offset, _size, 0, &target);
    memcpy(target, _data, _size);
    vkUnmapMemory(display_.device_, memory_);
  } else {
    buffer::range_t staging = display_.staging_->do_alloc(_size);
    VkBufferCopy copy;
    copy.size = _size;
    copy.srcOffset = staging.offset_;
    copy.dstOffset = _offset;

    display_.post([copy, this](auto) { display_.stage_copy(buffer_, &copy); });

    void *target;
    vkMapMemory(display_.device_, display_.staging_->memory_, staging.offset_, _size, 0, &target);
    memcpy(target, _data, _size);
    vkUnmapMemory(display_.device_, display_.staging_->memory_);
  }
}

void buffer::copy_from(VkBuffer _other, uint32_t _other_offset, uint32_t _this_offset, uint32_t _size) {
  VkBufferCopy copyRegion = {};
  copyRegion.srcOffset = _other_offset;
  copyRegion.dstOffset = _this_offset;
  copyRegion.size = _size;

  display_.stage_copy(_other, buffer_, &copyRegion);
}

void buffer::copy_from(buffer &_other, uint32_t _other_offset, uint32_t _this_offset, uint32_t _size) {
  copy_from(_other.buffer_, _other_offset, _this_offset, _size);
}

void buffer::grow(uint32_t new_size) {
  assert(new_size > size_);

  VkBuffer old_buff = buffer_;
  VkDeviceMemory old_mem = memory_;
  uint32_t old_size = size_;

  init(new_size, type_, usage_);
  copy_from(old_buff, 0, 0, size_);

  VkDevice dev = display_.device_;
  display_.on_staged.once([dev, old_buff, old_mem] {
    vkFreeMemory(dev, old_mem, nullptr);
    vkDestroyBuffer(dev, old_buff, nullptr);
    return true;
  });

  auto last = ranges_.rbegin();
  if (last->allocated_) {
    range_t nrange;
    nrange.allocated_ = false;
    nrange.size_ = (new_size - size_);
    nrange.offset_ = last->offset_ + last->size_;
    ranges_.insert(nrange);
  } else {
    range_t nrange = *last;
    nrange.size_ += (new_size - size_);
    ranges_.erase(std::next(last).base());
    ranges_.insert(nrange);
  }
}

buffer::range_t buffer::do_alloc(uint32_t _size) {
  auto it = ranges_.cbegin();
  for (; it != ranges_.cend(); it++) {
    if (!it->allocated_ && it->size_ >= _size)
      break;
  }

  if (it == ranges_.cend()) {
    grow(_size < size_ ? size_ * 2 : _size * 2);
    return do_alloc(_size);
  }

  range_t result;
  result.offset_ = it->offset_;
  result.size_ = _size;
  result.allocated_ = true;

  range_t free_block = *it;
  ranges_.erase(it);

  free_block.offset_ += _size;
  free_block.size_ -= _size;

  ranges_.insert(result);
  ranges_.insert(free_block);

  return result;
}

void buffer::do_free(uint32_t _offset, uint32_t _size) {
  std::set<range_t>::iterator it = ranges_.find(range_t{_offset, _size, true});
  if (it == ranges_.end())
    throw std::out_of_range("couldn't find range, probably already deleted");

  range_t result = *it;
  result.allocated_ = false;
  it = ranges_.erase(it);
  ranges_.insert(it, result);
  merge();
}

void buffer::debug_ranges() {
  for (auto &range : ranges_)
    std::cout << "\trange " << range.offset_ << " to " << (range.offset_ + range.size_) << " " << range.allocated_
              << std::endl;
}

void buffer::merge() {
  if (ranges_.size() < 2)
    return;

  auto prev = ranges_.begin();
  auto it = ranges_.begin();
  it++;
  while (it != ranges_.end()) {
    if (!it->allocated_ && !prev->allocated_) {
      range_t m = *prev;
      m.size_ += it->size_;
      ranges_.erase(prev);
      it = ranges_.erase(it);
      prev = ranges_.insert(it, m);
      it = prev;
      it++;
    } else {
      it++;
      prev++;
    }
  }
}

void buffer::clear_ranges() {
  ranges_.clear();
  ranges_.insert(range_t{0, size_, false});
}
