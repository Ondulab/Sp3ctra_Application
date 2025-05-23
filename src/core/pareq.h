// ----------------------------------------------------------------------------
//
//  Copyright (C) 2009-2017 Fons Adriaensen <fons@linuxaudio.org>
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
// ----------------------------------------------------------------------------

#ifndef __PAREQ_H
#define __PAREQ_H

#include "global.h"
#include <math.h>
#include <stdint.h>

class Pareq {
public:
  Pareq(void);
  ~Pareq(void);

  void setfsamp(float fsamp);
  void setparam(float f, float g) {
    _f0 = f;
    _g0 = powf(10.0f, 0.05f * g);
    _touch0++;
  }
  void reset(void);
  void prepare(int nsamp);
  void process(int nsamp, int nchan, float *data[]) {
    if (_state != BYPASS)
      process1(nsamp, nchan, data);
  }

private:
  enum { BYPASS, STATIC, SMOOTH, MAXCH = 4 };

  void calcpar1(int nsamp, float g, float f);
  void process1(int nsamp, int nchan, float *data[]);

  volatile int16_t _touch0;
  volatile int16_t _touch1;
  // bool              _bypass; // Unused private field
  int _state;
  float _fsamp;

  float _g0, _g1;
  float _f0, _f1;
  float _c1, _dc1;
  float _c2, _dc2;
  float _gg, _dgg;

  float _z1[MAXCH];
  float _z2[MAXCH];
};

#endif
