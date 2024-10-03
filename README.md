# GenDB

[![Actions Status](https://github.com/probcomp/hierarchical-irm/workflows/Python%20package/badge.svg)](https://github.com/probcomp/hierarchical-irm/actions)
[![pypi](https://img.shields.io/pypi/v/hirm.svg)](https://pypi.org/project/hirm/)

This repository contains an implementation of GenDB, a Bayesian method for
automatic structure discovery in tabular and relational data.

A paper describing this method is in preparation.  Briefly, this codebase
builds on the [Hierarchical Infinite Relations
Model](https://proceedings.mlr.press/v161/saad21a.html) implemented
in <https://github.com/probsys/hierarchical-irm> and
adds:

1. Many more distribution types, including a normal distribution for floating
point data, categorical and Skellam distributions for integer data, and a
bigram distribution for string data.

2. Hyperparameter inference.

3. Support for distributions like Skellam without conjugate priors.

4. Support for "noisy relations", which add noise to the output of another
relation.

5. A library of emission classes for noisy relations, including a gaussian
emission for adding gaussian noise to real data, a bitflip emission for
adding noise to boolean data, and a bigram emission for adding insertions,
substitutions and deletions to string data.

In addition, we use these capabilities to build an entirely new interface
to the HIRM model:  the pclean binary (named after the
[PClean paper](https://proceedings.mlr.press/v130/lew21a/lew21a.pdf) on which
it is heavily based) reads in tabular data and a schema describing the entities
that generated it, and converts the data and schema into their HIRM equivalents.


## Installation

First obtain a GNU C++ compiler, version 7.5.0 or higher.
The underlying code can be built and installed via bazel. To install bazel:

    $ sudo apt-get install bazel

If bazel is not available, first add the Bazel distribution URI as discussed
on this [page](https://bazel.build/install/ubuntu).

The binaries can be generated by first cloning this repository and then writing

    $ cd cxx
    $ bazel build -c opt ...

The unit tests can be invoked via

    $ bazel test ...

And the integrate tests with

    $ ./integration_tests.sh

## Usage: HIRM

Here is an example usage of the HIRM binary:

    $ cd cxx
    $ bazel build -c opt :hirm
    $ ./bazel-bin/hirm assets/animals.unary
    setting seed to 10
    loading schema from assets/animals.unary.schema
    loading observations from assets/animals.unary.obs
    selected model is HIRM
    incorporating observations
    inferring 10 iters; timeout 0
    saving to assets/animals.unary.10.hirm

Let's take a look at the two input files.

The first, which must have the name <path>.schema, looks like this:

```
$ head assets/animals.unary.schema
black ~ bernoulli(animal)
white ~ bernoulli(animal)
blue ~ bernoulli(animal)
brown ~ bernoulli(animal)
gray ~ bernoulli(animal)
orange ~ bernoulli(animal)
red ~ bernoulli(animal)
yellow ~ bernoulli(animal)
patches ~ bernoulli(animal)
spots ~ bernoulli(animal)
```

Each line specifies the signature of a relation in the system:

- The first token is the name of the relation; all the relation names must be
  unique.
- The second token is the `~` character.
- The third token is the observation type.  See the `DistributionSpec`
  definition for supported values for clean relations and the `EmissionSpec`
  definition for supported values for noisy relations.
- Inside the parenthesis is the list of domains of the relation.  In this
  example, the only domain is `animal`.

Thus, for this schema, we have a list of unary relations that each specify
whether an `animal` has a given attribute.

Note that, in general a given relational system can be encoded in multiple
ways. See `assets/animals.binary.schema` for an encoding of this system using
a single higher-order relation with signature:
`has ~ bernoulli(feature, animal)`.

The second input, which must be at <path>.obs, contains the observations
of the relations.  Observation files are comma separated values (CSV) and
look like

```
$ head assets/animals.unary.obs
0,black,antelope
1,black,grizzlybear
1,black,killerwhale
0,black,beaver
1,black,dalmatian
0,black,persiancat
1,black,horse
1,black,germanshepherd
0,black,bluewhale
1,black,siamesecat
```

Each line specifies a single observation:

- The first entry is 0 or 1.  This is because all of the relations in this
  example are bernoulli, which is boolean valued.  If we had used a ``normal``
  distribution instead, the first entry would be real valued.  If we had
  used a ``bigram`` distribution, the first entry would be string valued.
- The second entry is the relation name.  There must be a corresponding
  relation with the same name in the schema file.
- The third and subsequent entries are the names of domain entities; e.g,
  `antelope`, `grizzlybear`, etc., are entities in the `animals` domain.
  The number of domain entities on a line must correspond to the arity of the
  corresponding relation in the schema file. Since all the relations in this
  example are unary, there is only one entity after each relation name.

Thus, for this observation file, we have observations `black(antelope) = 0`,
`black(grizzlybear) = 1`, and so on.

Finally, let's take a look at the output file, `assets/animals.unary.10.hirm`.
It contains the clusterings of the relations and domain entities that the
system learned during inference.  The output is comprised of multiple sections,
each delimited by a single blank line.

```
$ cat assets/animals.unary.10.hirm
0 oldworld black insects skimmer chewteeth agility bulbous fast lean orange inactive slow stripes tail red active
1 quadrapedal paws strainteeth pads meatteeth hooves longneck ocean coastal hunter hairless smart group nocturnal meat buckteeth plankton plains timid horns hibernate forager ground grazer furry fields brown solitary stalker toughskin water arctic blue smelly claws swims vegetation fish flippers walks
5 mountains jungle forest bipedal cave desert fierce nestspot tree tusks yellow hands scavenger flys
6 muscle longleg domestic tunnels newworld bush big gray spots strong weak patches white hops small

irm=0
animal 0 giraffe seal horse bat rabbit chimpanzee killerwhale dalmatian mole chihuahua zebra deer lion mouse raccoon dolphin collie bobcat tiger siamesecat germanshepherd otter weasel spidermonkey beaver leopard antelope gorilla fox hamster squirrel wolf rat
animal 1 skunk persiancat giantpanda polarbear moose pig buffalo elephant cow sheep grizzlybear ox humpbackwhale walrus rhinoceros bluewhale hippopotamus

irm=1
animal 0 mouse rabbit zebra moose antelope horse buffalo deer ox cow gorilla pig rhinoceros chimpanzee giraffe sheep spidermonkey elephant
animal 1 collie germanshepherd siamesecat giantpanda chihuahua lion raccoon squirrel grizzlybear dalmatian rat persiancat weasel leopard skunk bobcat mole tiger hamster fox wolf
animal 3 otter walrus humpbackwhale killerwhale bluewhale dolphin seal
animal 4 polarbear bat
animal 5 hippopotamus beaver

irm=5
animal 0 antelope germanshepherd elephant hippopotamus tiger rhinoceros zebra giraffe killerwhale sheep humpbackwhale mole hamster persiancat horse siamesecat chihuahua cow dolphin walrus collie polarbear mouse pig deer moose skunk bluewhale buffalo dalmatian rat beaver ox fox seal rabbit wolf weasel otter
animal 1 squirrel raccoon giantpanda gorilla lion bat spidermonkey chimpanzee grizzlybear bobcat leopard

irm=6
animal 0 horse killerwhale spidermonkey deer giraffe germanshepherd rhinoceros leopard moose fox wolf buffalo dolphin bluewhale grizzlybear chimpanzee walrus lion bobcat zebra beaver elephant ox antelope gorilla hippopotamus humpbackwhale polarbear tiger
animal 1 collie squirrel raccoon chihuahua sheep hamster rabbit rat mouse skunk persiancat weasel mole bat otter siamesecat
animal 2 dalmatian giantpanda cow pig
animal 3 seal
```

The first section of the file shows how the relations were clustered.  In
this example, the system found four clusters and gave them the labels 0, 1,
5 and 6.  The subsequent sections show the sub-clustering of entities within
a given relation cluster.  For example, the `irm=6` section shows the clustering
within the `6` relation cluster and the line

```
animal 2 dalmatian giantpanda cow pig
```

says that those four animals get clustered together within that relation
cluster.  This example only has one domain (animal), but if there were
multiple domains, their entities would get assigned to clusters and be
listed in this section as well.

Please see the file `cxx/integration_tests.sh` for more examples of using
the hirm binary, and run `./bazel-bin/hirm --help` for more information
about its options.

# Visualizing HIRM's output

The clusters created by HIRM can be visualized using the `make_plots.py`
program in the `/py` subdirectory.

To use this program, you will first need to install numpy and matplotlib.
On Debian systems, this can be done by running `./py/install.sh`.

Then simply run `make_plots.py` and pass it the locations of your observations,
schema, and HIRM clusters:

    $ cd py
    $ ./make_plots.py --schema=../cxx/assets/animals.unary.schema --observations=../cxx/assets/animals.unary.obs --clusters=../cxx/assets/animals.unary.hirm --output=/tmp/vis.html

The visualization is written to the HTML file `/tmp/vis.html`, which can be
viewed by pointing your web browser to `file:///tmp/vis.html`.

Currently, `make_plots` only produces visualizations for unary and binary
relations, and only for numeric-valued relations (including boolean
valued relations like `bernoulli` and categorical relations like
`categorical`).

## Usage: PClean

Here is an example usage of the pclean binary:

    $ cd cxx
    $ bazel build -c opt pclean:pclean
    $ ./bazel-bin/pclean/pclean --schema=assets/flights.schema --obs=assets/flights_dirty.10.csv --iters=5 --output=/tmp/flights.out
    Setting seed to 10
    Reading plcean schema ...
    Reading schema file from assets/flights.schema
    Making schema helper ...
    Translating schema ...
    Reading observations ...
    Reading observations file from assets/flights_dirty.10.csv
    Creating hirm ...
    Translating observations ...
    Schema does not contain tuple_id, skipping ...
    Encoding observations ...
    Incorporating observations ...
    Running inference ...
    Starting iteration 1, model score = inf
    Starting iteration 2, model score = -30344.088796
    Starting iteration 3, model score = -37593.201854
    Starting iteration 4, model score = -37557.513713
    Starting iteration 5, model score = -35773.000439

Again, we have two input files, a schema and an observation file.
Let's look at the schema file:

```
$ cat assets/flights.schema
class TrackingWebsite
  name ~ stringcat(strings="aa airtravelcenter allegiantair boston businesstravellogue CO den dfw flightarrival flightaware flightexplorer flights flightstats flightview flightwise flylouisville flytecomm foxbusiness gofox helloflight iad ifly mco mia myrateplan mytripandmore orbitz ord panynj phl quicktrip sfo src travelocity ua usatoday weather world-flight-tracker wunderground")

class Time
  time ~ string(maxlength=40)

class Flight
  flight_id ~ string(maxlength=20)
  # These are all abbreviations for "scheduled/actual arrival/depature time"
  sdt ~ Time
  sat ~ Time
  adt ~ Time
  aat ~ Time

class Obs
  flight ~ Flight
  src ~ TrackingWebsite

observe
  src.name as src
  flight.flight_id as flight
  flight.sdt.time as sched_dep_time
  flight.sat.time as sched_arr_time
  flight.adt.time as act_dep_time
  flight.aat.time as act_arr_time
  from Obs
```

A pclean schema consists of one or more classes (separated by blank lines)
followed by an observe block.  Within a class, one or more variables can be
declared.  Variables come in two types:  class variables and scalar variables.
Class variables have the name of a class on the right of the `~`, and all
class names must start with an uppercase character.

Scalar variables have the name of an attribute on the right of the `~`, and
the attribute can optionally take parameters (like `maxlength`) inside of
parentheses.  Currently the following attribute names are supported:

* bool (boolean data)
* categorical (integer data)
* real (floating point data)
* string (string data)
* stringcat (string data)
* typo\_int (integer data possibly corrupted by typos)
* typo\_nat (natural number data possibly corrupted by typos)
* typo\_real (floating point data possibly corrupted by typos)

The observe block consists of one or more queries.  Each query starts with a
period-delimitted path specifier.  The final component of the path specifier
should be a scalar variable and all the other components must be class
variables.  The path specifier is interpreted relative to the class given
in the `from` clause of the observe block.

After the path specifier comes the word "as" followed by the name of the
query.  This name should match the name of a column in the CSV file of
observations, which looks like this:

```
$ cat assets/flights_dirty.10.csv
tuple_id,src,flight,sched_dep_time,act_dep_time,sched_arr_time,act_arr_time
1,aa,AA-3859-IAH-ORD,7:10 a.m.,7:16 a.m.,9:40 a.m.,9:32 a.m.
2,aa,AA-1733-ORD-PHX,7:45 p.m.,7:58 p.m.,10:30 p.m.,
3,aa,AA-1640-MIA-MCO,6:30 p.m.,,7:25 p.m.,
4,aa,AA-518-MIA-JFK,6:40 a.m.,6:54 a.m.,9:25 a.m.,9:28 a.m.
5,aa,AA-3756-ORD-SLC,12:15 p.m.,12:41 p.m.,2:45 p.m.,2:50 p.m.
6,aa,AA-204-LAX-MCO,11:25 p.m.,,12/02/2011 6:55 a.m.,
7,aa,AA-3468-CVG-MIA,7:00 a.m.,7:25 a.m.,9:55 a.m.,9:45 a.m.
8,aa,AA-484-DFW-MIA,4:15 p.m.,4:29 p.m.,7:55 p.m.,7:39 p.m.
9,aa,AA-446-DFW-PHL,11:50 a.m.,12:12 p.m.,3:50 p.m.,4:09 p.m.
```

It is okay if the CSV file has columns that don't correspond to queries
(like `tuple_id` in this example), but currently we don't support schemas
with queries that don't appear as columns.

Finally, the output of `pclean` is in the same format as `hirm`.  If we look
at it

```
$ head /tmp/flights.out
3 Time:time src
4 TrackingWebsite:name
6 Flight:flight_id
9 sched_dep_time
10 act_dep_time flight act_arr_time
12 sched_arr_time

```

we can see that it clustered the relations into six clusters.  If we look
at the cluster named `10` in the output file:

```
irm=10
TrackingWebsite 1 1 3 6 7 8
TrackingWebsite 2 0 2 4 5
Obs 1 0 1 3 4 5 6 7 8
Obs 2 2
Flight 0 0 2 3 4 6 7 8
Flight 1 1 5
Time 0 0 1 2 3 4 5 6 7 8
```

we can see that it clustered the underlying entities into three clusters
(named 0, 1, and 2).  All of the rows of the CSV file (which correspond
to the `Obs` domain, because of the `from Obs` in the schema) were put
into cluster 1 except for the line

```
3,aa,AA-1640-MIA-MCO,6:30 p.m.,,7:25 p.m.,
```

which was put into cluster 2, probably because it was missing two fields.
All of the flights were put into cluster 0, except for "AA-1733-ORD-PHX"
and "AA-204-LAX-MCO", which were put into cluster 1.

Run `./bazel-bin/pclean/pclean --help` for more information about the
other options that it supports.

## License

Copyright (c) 2024

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
