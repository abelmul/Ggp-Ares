# Ares

Ares is a general game playing agent,that is capable of playing any game given
to it in a formal description (specifically GDL).

## Modules

### Gdl Parser

This module is responsible for both parsing gdl and transforming some rules (rules containing `or`)
into suitable rules not containing `or`.

### Reasoner

This module (found in the directory reasoner) implements the sldnf resolution algorithm.

### Threading

This module is responsible for providing an exclusively owned thread pool.

## Launch

(make sure boost is installed.)

`make ares`

`./ares`       (Currently just infers and prints the initial state of the game, `chess.gdl`)

### The configuration file

In the config file `ares.cfg.json`

`parser_threads` : number of threads to use while transforming the input gdl (apparentlly one is sufficient.)

`prover_threads` : number of threads to use for exploring the sld-tree, while answering queries.

`neg_threads`    : number of threads to use for exploring the sld-tree, while answering negative subgoals.

`gdl`            : the path to the gdl file.

`file`           : `bool`, wether or not the gdl is obtained from a file(offline) or using the httpHandler(during games)

## Dependencies

It depends on boost.
