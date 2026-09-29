#!/usr/bin/env python3
from timber import Timber

t = Timber()
t.set_format("$T $L: $M")
t.add_stdout()
t.info("Selam arkadaşlar!")
