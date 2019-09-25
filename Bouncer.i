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
#include <Bouncer/Configuration.hpp>
#include <Bouncer/Host.hpp>
#include <Bouncer/Main.hpp>
#include <Bouncer/Stats.hpp>
#include <Bouncer/User.hpp>
%}

%include <stdint.i>
%include <std_map.i>
%include <std_shared_ptr.i>
%include <std_string.i>
%include <std_vector.i>
%include <typemaps.i>

%shared_ptr(Bouncer::Host)

// Ignore move operators because SWIG doesn't support them.
%ignore Bouncer::Main::operator=(Main&&);

%feature("director") Bouncer::Host;

// The %include directives below need to be done in order
// of dependencies (if A depends on B, list B first and then A).
// -------
%include "Bouncer/include/Bouncer/Stats.hpp"
%include "Bouncer/include/Bouncer/User.hpp"

%include "Bouncer/include/Bouncer/Configuration.hpp"
%include "Bouncer/include/Bouncer/Host.hpp"

%include "Bouncer/include/Bouncer/Main.hpp"
// -------

%template(StdVectorString) std::vector< std::string >;
%template(Users) std::vector< Bouncer::User >;
