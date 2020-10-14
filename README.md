- [Ares](#ares)
  - [Build and Launch](#build-and-launch)
    - [Build](#build)
    - [Launch](#launch)
  - [The config file](#the-config-file)
  - [Modules](#modules)
    - [Gdl Parser](#gdl-parser)
    - [Reasoner](#reasoner)
    - [GameAnalyzer](#gameanalyzer)
    - [Strategy](#strategy)
    - [HttpHandler](#httphandler)
  - [Dependencies](#dependencies)
  - [Tests](#tests)
    - [Run Unit Tests](#run-unit-tests)
    - [Run Stress Tests](#run-stress-tests)

# Ares

Ares is a general game playing agent,that is capable of playing any game given
to it in a formal description (specifically GDL).


## Build and Launch

### Build
(make sure boost is installed.)
~~~
make setup
make ares
~~~

### Launch
~~~
make run
~~~

## The config file

This section briefly explains the options one might tweak with ares.

~~~
{
    "strategy": string, could be one of Montecarlo or Random, or any other class
    that inherits from strategy and RegistrarBase<T>.  

    "mct_threads": int, the number of threads to use for the montecarlo tree search.
    
    "url":string, the http address ares will listen on.
    
    "gdl":string, some tests might depend on this.

    "simulations":int, number of simulations a game to run during stress tests.

    "steps":int, number of steps of a game to run during stress tests.

    "ansSample":int, how many answers to sample if proving randomly.

    "uct_c":int, the C parameter of montecarlo uct

    "delta_milli":int, the amount milliseconds to deduct from start and play clock,
    so that http replies get to the game manager on time.
    
    "stateDumpF":"./visualization/data.json", strategies dump thier state for visualization
     and debugging purposes. Don't change this.
}

~~~

## Modules

### Gdl Parser

This module is responsible for parsing gdl.

### Reasoner

This module (found in the directory reasoner) implements the oldt resolution algorithm, as well as GGP related interfaces.

### GameAnalyzer

This module does simple game analysis, such as wether or not a game is an alternating zero-sum game. It does so by doing multiple random simulations.

### Strategy

Contains a set of strategies used to decide which move to make at each step of a play.

### HttpHandler

Handles the HTTP communication with a game manager inorder to play games.

## Dependencies

1. Boost, install using:
   
   On debian based systems.

   `sudo apt-get install libboost-all-dev`

   On Fedora.

   ` sudo yum install boost-devel`

   Or follow the instructions on the official site [here](http://www.boost.org/doc/libs/1_42_0/more/getting_started/unix-variants.html#easy-build-and-install)
   
2. cpprestsdk is installed during `make setup`

## Tests

There are two directories found in the `test` directory, named `stress` and `unit` containing stress and unit tests. The directories contain several tests. 

### Run Unit Tests
~~~
make unit_tests
make run_unit_tests
~~~

### Run Stress Tests

~~~
make stress_tests
make run_stress_tests
~~~

