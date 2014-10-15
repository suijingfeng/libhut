/*  _ _ _   _       _
 * | |_| |_| |_ _ _| |_
 * | | | . |   | | |  _|
 * |_|_|___|_|_|___|_|
 * Hobby graphics and GUI library under the MIT License (MIT)
 *
 * Copyright (c) 2014 Jean-Baptiste "Jiboo" Lepesme
 * github.com/jiboo/libhut @lepesmejb +JeanBaptisteLepesme
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include <vector>
#include <functional>

namespace hut {

    template<typename T>
    class property {
    protected:
        using getter_t = std::function<const T&(void)>;
        using setter_t = std::function<void(const T&)>;

        getter_t getter;
        setter_t setter;

    public:
        property(getter_t getter, setter_t setter) : getter(getter), setter(setter) { // Wow! Ugly.

        }

        const T& operator=(const T& data) {
            setter(data);
            return data;
        }

        operator const T&() {
            return getter();
        }
    };

    template<typename T>
    class buffed {
    protected:
        using setter_t = std::function<void(const T&)>;

        T data;
        setter_t setter;

    public:
        buffed(setter_t setter) : setter(setter) { // Wow! Ugly.

        }

        const T& operator=(const T& _data) {
            data = _data;
            setter(data);
            return _data;
        }

        operator const T&() const {
            return data;
        }
    };

} // namespace hut