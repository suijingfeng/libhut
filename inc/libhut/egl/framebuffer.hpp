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

#include <memory>

#include <GLES2/gl2.h>

#include "libhut/framebuffer.hpp"

namespace hut {

    class framebuffer : public base_framebuffer {
    public:
        framebuffer(std::shared_ptr<texture> t) : base_framebuffer(t) {
            glGenFramebuffers(1, &name);
            glBindFramebuffer(GL_FRAMEBUFFER, name);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, t->name, 0);

            glClearColor(0, 0, 0, 0);
            glClear(GL_COLOR_BUFFER_BIT);

            glViewport(0, 0, t->real_size[0], t->real_size[1]);

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        virtual ~framebuffer() {
            glDeleteFramebuffers(1, &name);
        }

        virtual void draw(std::shared_ptr<drawable> d) {
            glBindFramebuffer(GL_FRAMEBUFFER, name);
            d->draw(target->real_size);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        virtual void draw(std::shared_ptr<batch> b) {

        }

        virtual uivec2 size() const {
            return target->real_size;
        }

        virtual unsigned short density() const {
            return target->density;
        }

    protected:
        GLuint name;
    };

} // namespace hut
