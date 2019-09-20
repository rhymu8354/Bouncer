/**
 * @file Bouncer.i
 *
 * This is a SWIG interface file used to construct a facade for
 * accessing Bouncer from other languages such as C#.
 *
 * Copyright (c) 2019 by Richard Walters
 */

%module(directors="1") Module

%{
#include <Bouncer/Main.hpp>
%}

%include <stdint.i>
%include <std_map.i>
%include <std_shared_ptr.i>
%include <std_string.i>
%include <std_vector.i>
%include <typemaps.i>

// Ignore move operators because SWIG doesn't support them.
%ignore Bouncer::Main::operator=(Main&&);

%include "Bouncer/include/Bouncer/Main.hpp"
