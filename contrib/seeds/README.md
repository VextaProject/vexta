# Seeds

Utility to generate the seeds.txt list that is compiled into the client
(see [src/chainparamsseeds.h](/src/chainparamsseeds.h) and other utilities in [contrib/seeds](/contrib/seeds)).

Be sure to update `PATTERN_AGENT` in `makeseeds.py` to include the current version,
and remove old versions as necessary (at a minimum when GetDesirableServiceFlags
changes its default return value, as those are the services which seeds are added
to addrman with).

The seeds compiled into a release should be created from verified Vexta crawler data:

    python3 makeseeds.py < seeds_main.txt > nodes_main.txt
    python3 generate-seeds.py . > ../../src/chainparamsseeds.h

Do not generate fixed seeds until public Vexta nodes and a trusted crawler are available.

## Dependencies

Ubuntu, Debian:

    sudo apt-get install python3-dnspython

and/or for other operating systems:

    pip install dnspython

See https://dnspython.readthedocs.io/en/latest/installation.html for more information.
