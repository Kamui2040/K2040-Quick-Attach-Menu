# Third-party notices

## BaseNPCSwapper MNAM resolver

The OMOD record-walking approach in `src/OmodTargetResolver.cpp` is adapted
from the MNAM resolver in
[mraggi/BaseNPCSwapper](https://github.com/mraggi/BaseNPCSwapper), reviewed at
commit `6a5d0351123333ee013d800531b500a6ef0d6c77`.

BaseNPCSwapper is licensed under GPL-3.0. K2040's Quick Attach Menu is also
licensed under GPL-3.0. The adapted implementation has been changed for this
project to cache complete MNAM arrays, honor winning override records, and
expose fail-closed target metadata to the generated-menu runtime.
