#!/usr/bin/env python3
import timber

t = timber._timber.timber_alloc()
timber._timber.timber_add_stdout_sink(t)
timber._timber.timber_init(t)
timber._timber.timber_log(t, timber._timber.TIMBER_INFO, b"selam")
timber._timber.timber_destroy(t)
