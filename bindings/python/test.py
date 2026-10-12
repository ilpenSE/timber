#!/usr/bin/env python3
from timber import Timber, TimberMode

t = Timber()
t.set_format("$T $L: $M").add_stdout()
t.info("Selam arkadaşlar!")
